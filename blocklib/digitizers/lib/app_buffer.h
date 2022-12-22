#ifndef INCLUDED_DIGITIZERS_APP_BUFFER_H
#define INCLUDED_DIGITIZERS_APP_BUFFER_H

#include <boost/lockfree/spsc_queue.hpp>

#include <error.h>

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include <fmt/format.h>

#include <system_error>
#include <cstring>
#include <memory>
#include <sstream>

namespace gr::digitizers {

/*!
 * \brief A helper class holding application buffer. An application buffer consists of a
 * number of small buffers (data chunks). This class takes care of managing the free data
 * chunk poll.
 *
 * Details are given in the form of inline comments below.
 */
class app_buffer_t
{
public:
    static const unsigned MAX_NR_BUFFERS = 8192;

    app_buffer_t()
        : d_free_data_chunks(MAX_NR_BUFFERS),
          d_data_chunks(MAX_NR_BUFFERS),
          d_nr_channels(0),
          d_nr_ports(0),
          d_chunk_size_bytes(0),
          d_chunk_size(0),
          d_nr_chunks(0),
          d_data_rdy_errc()
    {
    }

    /*!
     * \brief A data structure holding data of all ENABLED (and only enabled) analog &
     * digital channels/ports.
     *
     * Memory organization:
     *  <chan 1 values>
     *  <chan 1 errors>
     *  <chan 2 values>
     *  ...
     *  <port 1 values>
     *  <port 2 values>
     *  ...
     */
    struct data_chunk_t {
        data_chunk_t() = delete;

        data_chunk_t(size_t mem_size_bytes)
            : d_data(mem_size_bytes), d_status(), d_local_timestamp(0), d_lost_count(0)
        {
        }

        std::vector<uint8_t> d_data;
        std::vector<uint32_t> d_status; // see channel_status_t enum definition
        std::chrono::nanoseconds d_local_timestamp; // UTC nanoseconds
        int d_lost_count;                           // number of buffers lost
    };

    using data_chunk_ptr =
        std::unique_ptr<data_chunk_t, std::function<void(data_chunk_t*)>>;

private:
    using data_chunk_sptr = std::shared_ptr<data_chunk_t>;

    // To avoid manual memory management
    std::vector<data_chunk_sptr> d_chunks;

    // For simplify we use circular buffers for managing free pool of data chunks
    boost::lockfree::spsc_queue<data_chunk_t*> d_free_data_chunks;
    boost::lockfree::spsc_queue<data_chunk_t*> d_data_chunks;

    // Static buffer configuration
    int d_nr_channels;      // number of enabled analog channels
    int d_nr_ports;         // number of enabled digital ports
    int d_chunk_size_bytes; // size of data chunk in bytes

    size_t d_chunk_size; // number of samples per data chunk (or buffer)
    size_t d_nr_chunks;  // number of data chunks the application buffer support

    // Mutex is used only in combination with the conditional variable
    boost::mutex d_mutex;
    boost::condition_variable d_data_rdy_cv;

    // For communicating errors to worker function
    std::error_code d_data_rdy_errc;

public:
    /*!
     * \brief Initialize application buffer.
     */
    void initialize(int nr_enabled_channels,
                    int nr_enabled_ports,
                    size_t chunk_size,
                    size_t nr_chunks)
    {
        // in order to use lock-free containers we need to use static-sized data
        // structures
        if (nr_chunks > MAX_NR_BUFFERS) {
            nr_chunks = MAX_NR_BUFFERS;
        }
        boost::mutex::scoped_lock lock(d_mutex);

        d_nr_channels = nr_enabled_channels;
        d_nr_ports = nr_enabled_ports;
        d_chunk_size = chunk_size;
        d_nr_chunks = nr_chunks;

        d_chunk_size_bytes =
            (d_nr_ports * d_chunk_size)                           // digital data
            + (d_nr_channels * d_chunk_size * sizeof(float) * 2); // analog data

        // To support re-initialization, delete all data chunks
        d_chunks.clear();
        d_data_chunks.reset();
        d_free_data_chunks.reset();

        // Make individual chunks (or buffers)
        for (size_t i = 0; i < d_nr_chunks; i++) {
            auto sptr = std::make_shared<data_chunk_t>(d_chunk_size_bytes);
            d_chunks.push_back(sptr);
            d_free_data_chunks.push(sptr.get());
        }

        // Reset error code...
        d_data_rdy_errc = std::error_code{};
    }

    /*!
     * \brief Adds/inserts data into the application buffer (or data chunk). Driver
     * implementors must make sure to organize memory as expected by the data_chunk_t
     * structure.
     */
    void add_full_data_chunk(data_chunk_t* data_chunk)
    {
        d_data_chunks.push(data_chunk);

        // notify clients (i.e. work thread) about new data chunk
        d_data_rdy_cv.notify_one();
    }

    std::error_code wait_data_ready()
    {
        boost::unique_lock<boost::mutex> lock(d_mutex);
        d_data_rdy_cv.wait(lock,
                           [this] { return !d_data_chunks.empty() || d_data_rdy_errc; });
        return d_data_rdy_errc;
    }

    /*!
     * \brief This method is meant to be used by driver-level code to communicate error
     * conditions to the work thread.
     *
     * If an error is returned from the wait_data_ready then the work method should exit
     * or rearm the driver.
     */
    void notify_data_ready(std::error_code ec)
    {
        {
            boost::mutex::scoped_lock guard(d_mutex);
            d_data_rdy_errc = ec;
        }

        d_data_rdy_cv.notify_one();
    }

    data_chunk_ptr get_data_chunk()
    {
        if (d_data_rdy_errc || d_data_chunks.empty()) {
            // by the contract the work method must wait data being ready before calling
            // this method
            throw std::runtime_error(fmt::format(
                "Exception in {}:{}:  Use wait_data_ready!!!", __FILE__, __LINE__));
        }

        auto releaser = [this](data_chunk_t* chunk) {
            // instead of delete, return chunk to the pool for reuse
            if (chunk) {
                d_free_data_chunks.push(chunk);
            }
        };

        // get the oldest data chunk
        data_chunk_t* data_chunk = nullptr;
        d_data_chunks.pop(data_chunk);
        assert(data_chunk != nullptr);
        return data_chunk_ptr{ data_chunk, releaser };
    }

    /*!
     * \brief Get free application buffer. If none available it returns nullptr.
     */
    data_chunk_t* get_free_data_chunk()
    {
        if (d_free_data_chunks.empty()) {
            return nullptr;
        }

        data_chunk_t* ptr = nullptr;
        d_free_data_chunks.pop(ptr);
        return ptr;
    }
};

} // namespace gr::digitizers

#endif /* INCLUDED_DIGITIZERS_APP_BUFFER_H */
