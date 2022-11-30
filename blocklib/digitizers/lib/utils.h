#ifndef INCLUDED_DIGITIZERS_UTILS_H
#define INCLUDED_DIGITIZERS_UTILS_H

#include <gnuradio/digitizers/tags.h>
#include <gnuradio/tag.h>

#include <boost/call_traits.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/thread/pthread/condition_variable.hpp>
#include <boost/thread/pthread/mutex.hpp>

#include <system_error>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <list>
#include <queue>
#include <vector>

namespace gr {
namespace digitizers {

inline uint64_t get_timestamp_nano_utc()
{
    timespec start_time;
    clock_gettime(CLOCK_REALTIME, &start_time);
    return (start_time.tv_sec * 1000000000) + (start_time.tv_nsec);
}

inline uint64_t get_timestamp_milli_utc()
{
    return uint64_t(get_timestamp_nano_utc() / 1000000);
}

inline std::vector<tag_t> filter_tags(std::vector<tag_t>&& tags, const std::string& key)
{
    // TODO(port) check if we can use ranges here instead of modifying the container
    auto has_not_key = [&key](const auto& tag) { return tag.map().count(key) == 0; };

    std::erase_if(tags, has_not_key);
    return tags;
}

/*!
 * \brief Converts an integer value to hex (string).
 */
inline std::string to_hex_string(int value)
{
    std::stringstream sstream;
    sstream << "0x" << std::setfill('0') << std::setw(2) << std::hex << (int)value;
    return sstream.str();
}

/*!
 * \brief Converts error code to string message.
 */
inline std::string to_string(std::error_code ec) { return ec.message(); }

template <typename T>
class concurrent_queue
{
private:
    mutable boost::mutex mut;
    std::queue<T> data_queue;
    boost::condition_variable data_cond;

public:
    concurrent_queue() {}

    void push(T new_value)
    {
        {
            boost::lock_guard<boost::mutex> lg(mut);
            data_queue.push(new_value);
        }
        data_cond.notify_one();
    }

    /*!
     *\returns false if no data is available in time, otherwiese true.
     */
    bool wait_and_pop(T& value, boost::chrono::milliseconds timeout)
    {
        boost::unique_lock<boost::mutex> lk(mut);
        auto retval =
            data_cond.wait_for(lk, timeout, [this] { return !data_queue.empty(); });
        if (retval) { // true is returned if condition evaluates to true...
            value = data_queue.front();
            data_queue.pop();
        }

        return retval;
    }

    /*!
     *\returns false if no data is available, otherwiese true.
     */
    bool pop(T& value)
    {
        boost::unique_lock<boost::mutex> lk(mut);

        if (data_queue.size() == 0) {
            return false;
        }

        value = data_queue.front();
        data_queue.pop();

        return true;
    }

    bool empty() const
    {
        boost::lock_guard<boost::mutex> lk(mut);
        return data_queue.empty();
    }

    void clear()
    {
        boost::lock_guard<boost::mutex> lk(mut);
        while (!data_queue.empty()) {
            data_queue.pop();
        }
    }

    size_t size() const
    {
        boost::lock_guard<boost::mutex> lk(mut);
        return data_queue.size();
    }
};

template <class T>
class circular_buffer
{
public:
    typedef boost::circular_buffer<T> container_type;
    typedef typename container_type::size_type size_type;
    typedef typename container_type::value_type value_type;
    typedef typename container_type::const_iterator const_iterator;
    typedef typename boost::call_traits<value_type>::param_type param_type;

    explicit circular_buffer(size_type capacity) : m_missed(0), m_container(capacity) {}

    size_type size() const { return m_container.size(); }

    size_type capacity() const { return m_container.capacity(); }

    uint64_t missed_count() const { return m_missed; }

    const_iterator begin() { return m_container.begin(); }

    const_iterator end() { return m_container.end(); }

    void push_back(const value_type* items, size_type nitems)
    {
        for (size_type i = 0; i < nitems; i++) {
            if (m_container.full()) {
                m_missed++;
            }

            m_container.push_back(items[i]);
        }
    }

    void pop_front(value_type* pItem, size_type nitems)
    {
        assert(m_container.size() >= nitems);

        for (size_type i = 0; i < nitems; i++) {
            pItem[i] = m_container.front();
            m_container.pop_front();
        }
    }

    void pop_front() { m_container.pop_front(); }

private:
    circular_buffer(const circular_buffer&);            // Disabled copy constructor.
    circular_buffer& operator=(const circular_buffer&); // Disabled assign operator.

    uint64_t m_missed;
    container_type m_container;
};

inline gr::tag_t make_peak_info_tag(double frequency, double stdev)
{
    return { 0, { { "peak_info", std::vector<pmtf::pmt>(frequency, stdev) } } };
}

inline void decode_peak_info_tag(const gr::tag_t& tag, double& frequency, double& stdev)
{
    const auto tag_value = tag.get("peak_info");
    assert(tag_value);
    const auto tag_vector = pmtf::get_as<std::vector<pmtf::pmt>>(*tag_value);
    assert(tag_vector.size() == 2);
    frequency = pmtf::get_as<double>(tag_vector[0]);
    stdev = pmtf::get_as<double>(tag_vector[1]);
}

static const double fwhm2stdev = 0.5 / sqrt(2 * log(2));
static const double whm2stdev = 2.0 * fwhm2stdev;

template <typename T>
class median_filter
{
private:
    std::queue<T> vals;
    std::list<T> ord_vals;
    int num;
    int middle;

public:
    median_filter(int n) : num(n), middle(n / 2)
    {
        // fill wit zeroes as starter.
        for (int i = 0; i < num; i++) {
            vals.push(0.0);
            ord_vals.push_back(0.0);
        }
    }

    T add(T new_el)
    {
        // track sample chronological order and remove oldest one
        vals.push(new_el);
        float oldest = vals.front();
        vals.pop();

        // remove from ordered list the oldest value
        for (auto it = ord_vals.begin(); it != ord_vals.end(); ++it) {
            if (*it == oldest) {
                ord_vals.erase(it);
                break;
            }
        }
        // add to the ordered list by insertion
        bool added_in = false;
        for (auto it = ord_vals.begin(); it != ord_vals.end(); ++it) {
            if (*it <= new_el) {
                ord_vals.insert(it, new_el);
                added_in = true;
                break;
            }
        }
        // the value hasn't been inserted into the list.
        // add it to end, i.e. biggest sample in this window.
        if (!added_in) {
            ord_vals.push_back(new_el);
        }

        // middle value of the new ordered list is
        // the median of the last n samples
        auto mean_val = ord_vals.begin();
        std::advance(mean_val, middle);
        return *mean_val;
    }
};

template <typename T>
class average_filter
{
private:
    std::queue<T> vals;
    int num;
    T running_avg;
    int iterations_to_fixing;

    // fixes small errors that may occur in average estimation
    bool fix_runinng_average()
    {
        iterations_to_fixing++;
        if (iterations_to_fixing == 100000) {
            // calculate a fresh average
            running_avg = 0.0;
            for (int i = 0; i < num; i++) {
                // iteration through a queue.
                double val = vals.front();
                vals.pop();
                vals.push(val);

                // sum up all elements in queue
                running_avg += val;
            }
            // average the sum
            running_avg /= num;

            // prepare for new estimations
            iterations_to_fixing = 0;
            return true;
        }
        else {
            return false;
        }
    }

public:
    average_filter(int n) : num(n), running_avg(0.0), iterations_to_fixing(0)
    {
        for (int i = 0; i < num; i++) {
            vals.push(0.0);
        }
        fix_runinng_average();
    }

    T add(T val)
    {
        float old_el = vals.front();
        vals.pop();
        vals.push(val);

        // if running average has not been freshly calculated,
        // estimate it
        if (!fix_runinng_average()) {
            running_avg = (num * running_avg - old_el + val) / num;
        }

        return running_avg;
    }

    T get_avg_value() const { return running_avg; }

    size_t size() const { return vals.size(); }
};

template <typename T>
class measurement_buffer_t
{
public:
    using data_chunk_sptr = std::shared_ptr<T>;

    measurement_buffer_t(std::size_t num_buffers)
        : d_free_data_chunks(num_buffers), d_data_chunks(num_buffers)
    {
    }

    void add_measurement(const data_chunk_sptr& data_chunk)
    {
        boost::mutex::scoped_lock lock(d_mutex);
        d_data_chunks.push_back(data_chunk);
    }

    void return_free_buffer(const data_chunk_sptr& data_chunk)
    {
        boost::mutex::scoped_lock lock(d_mutex);
        d_free_data_chunks.push_back(data_chunk);
    }

    data_chunk_sptr get_free_buffer()
    {
        boost::mutex::scoped_lock lock(d_mutex);

        if (d_free_data_chunks.empty()) {
            return nullptr;
        }
        else {
            auto ptr = d_free_data_chunks.back();
            d_free_data_chunks.pop_back();
            return ptr;
        }
    }

    data_chunk_sptr get_measurement()
    {
        boost::unique_lock<boost::mutex> lock(d_mutex);
        if (d_data_chunks.empty()) {
            return nullptr;
        }
        // get the oldest data chunk
        auto sptr = d_data_chunks.front();
        d_data_chunks.pop_front();

        return sptr;
    }

private:
    // For simplify we use circular buffers for managing free pool of data chunks
    boost::circular_buffer<data_chunk_sptr> d_free_data_chunks;
    boost::circular_buffer<data_chunk_sptr> d_data_chunks;
    boost::mutex d_mutex;
};

/*!
 * Find acq_info tags belonging to a given range and merge them together. The only thing
 * that needs to be merged is in fact status bitfield.
 */
/*template <typename AcqInfoContainer>
inline acq_info_t
calculate_acq_info_for_range(uint64_t start, uint64_t end, const AcqInfoContainer &
container, float samp_rate)
{
  acq_info_t result {};
  result.timestamp = -1;
  result.last_beam_in_timestamp = -1;
  result.trigger_timestamp = -1;

  bool taken = false;

  // keep replacing until the first in the range is found
  for (const auto & info : container)
  {
    if (info.offset <= start)
    {
      result = info;
      taken = true;
    }
    else if (info.offset > start && info.offset < end)
    {
      if (taken)
      {
        result.status |= info.status;  // merge status
      }
      else
      {
        result = info;
        taken = true;
      }
    }
  }

  // update timestamp
  if (result.timestamp != -1) {
    auto delta_samples = start >= result.offset
            ? static_cast<float>(start - result.offset)
            : -static_cast<float>(result.offset - start);
    auto delta_ns = static_cast<int64_t>((delta_samples / samp_rate) * 1000000000.0f);
    result.timestamp += delta_ns;
  }

  // we take our sample rate for granted
  result.timebase = 1.0f / samp_rate;

  return result;
}*/

} // namespace digitizers
} // namespace gr

#endif
