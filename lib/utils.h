/* -*- c++ -*- */
/*
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


#ifndef INCLUDED_DIGITIZERS_UTILS_H
#define INCLUDED_DIGITIZERS_UTILS_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <system_error>
#include <boost/circular_buffer.hpp>
#include <pmt/pmt.h>
#include <gnuradio/tags.h>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <queue>
#include <list>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Converts an integer value to hex (string).
     */
    inline std::string
    to_hex_string(int value)
    {
      std::stringstream sstream;
      sstream << "0x"
              << std::setfill('0') << std::setw(2)
              << std::hex << (int)value;
      return sstream.str();
    }

    /*!
     * \brief Converts error code to string message.
     */
    inline std::string
    to_string(std::error_code ec)
    {
      return ec.message();
    }

    // TODO: replace with boost mutex/cond_variable in order to allow for thread interruption.
    template <typename T>
    class concurrent_queue
    {
      private:
        mutable std::mutex mut;
        std::queue<T> data_queue;
        std::condition_variable data_cond;

      public:
        concurrent_queue(){}

        void push(T new_value)
        {
            {
                std::lock_guard<std::mutex> lg(mut);
                data_queue.push(new_value);
            }
            data_cond.notify_one();

            // TODO: jgolob remove this
            int64_t microseconds_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            std::cerr << "push timestamp " << microseconds_since_epoch << std::endl;
        }

        /*!
         *\returns false if no data is available in time, otherwiese true.
         */
        bool wait_and_pop(T& value, std::chrono::milliseconds timeout)
        {
            std::unique_lock<std::mutex> lk(mut);
            auto retval = data_cond.wait_for(lk, timeout, [this]{return !data_queue.empty();});
            if (retval) { // true is returned if condition evaluates to true...
                value = data_queue.front();
                data_queue.pop();
            }

            // TODO: jgolob and this
            int64_t microseconds_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            std::cerr << "pop timestamp " << microseconds_since_epoch << " " << retval <<  std::endl;

            return retval;
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> lk(mut);
            return data_queue.empty();
        }

        void clear()
        {
            std::lock_guard<std::mutex> lk(mut);
            while(!data_queue.empty()) {
                data_queue.pop();
            }
        }

        size_t size() const
        {
           std::lock_guard<std::mutex> lk(mut);
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

      explicit circular_buffer(size_type capacity)
        : m_missed(0), m_container(capacity)  {}

      size_type size() const {
        return m_container.size();
      }

      size_type capacity() const {
        return m_container.capacity();
      }

      uint64_t missed_count() const {
        return m_missed;
      }

      const_iterator begin()
      {
          return m_container.begin();
      }

      const_iterator end()
      {
          return m_container.end();
      }

      void push_back(const value_type *items, size_type nitems) {
        for (size_type i=0; i<nitems; i++) {
          if (m_container.full()) {
            m_missed++;
          }

          m_container.push_back(items[i]);
        }
      }

      void pop_front(value_type* pItem, size_type nitems) {
        assert(m_container.size() >= nitems);

        for (size_type i=0; i<nitems; i++) {
            pItem[i] = m_container.front();
            m_container.pop_front();
        }
      }

      void pop_front()
      {
          m_container.pop_front();
      }

    private:
      circular_buffer(const circular_buffer&);              // Disabled copy constructor.
      circular_buffer& operator = (const circular_buffer&); // Disabled assign operator.

      uint64_t m_missed;
      container_type m_container;
    };

    inline gr::tag_t
    make_peak_info_tag(double frequency, double stdev)
    {
      gr::tag_t tag;
      tag.key = pmt::intern("peak_info");
      tag.value = pmt::make_tuple(
          pmt::from_double(frequency),
          pmt::from_double(stdev));
      return tag;
    }

    inline void
    decode_peak_info_tag(const gr::tag_t &tag, double &frequency, double &stdev)
    {
      assert(pmt::symbol_to_string(tag.key) == "peak_info");

      auto values = pmt::to_tuple(tag.value);
      frequency = pmt::to_double(tuple_ref(values, 0));
      stdev = pmt::to_double(tuple_ref(values, 1));
    }

    static const double fwhm2stdev = 0.5/sqrt(2*log(2));
    static const double whm2stdev = 2.0*fwhm2stdev;
  }



  template<typename T>
  class median_filter
  {
  private:
    std::queue<T> vals;
    std::list<T> ord_vals;
    int num;
    int middle;

  public:
    median_filter(int n):
    num(n),middle(n/2)
    {
      //fill wit zeroes as starter.
      for(int i = 0; i < num; i++) {
        vals.push(0.0);
        ord_vals.push_back(0.0);
      }
    }

    T add(T new_el)
    {
      //track sample chronological order and remove oldest one
      vals.push(new_el);
      float oldest = vals.front();
      vals.pop();

      //remove from ordered list the oldest value
      for(auto it = ord_vals.begin(); it != ord_vals.end(); ++it){
        if(*it == oldest){
          ord_vals.erase(it);
          break;
        }
      }
      //add to the ordered list by insertion
      bool added_in = false;
      for(auto it = ord_vals.begin(); it != ord_vals.end(); ++it) {
        if(*it <= new_el) {
          ord_vals.insert(it, new_el);
          added_in = true;
          break;
        }
      }
      //the value hasn't been inserted into the list.
      //add it to end, i.e. biggest sample in this window.
      if(!added_in) { ord_vals.push_back(new_el); }

      //middle value of the new ordered list is
      //the median of the last n samples
      auto mean_val = ord_vals.begin();
      std::advance(mean_val, middle);
      return *mean_val;
    }
  };

  template<typename T>
  class average_filter
  {
  private:
    std::queue<T> vals;
    int num;
    T running_avg;
    int iterations_to_fixing;

    //fixes small errors that may occur in average estimation
    bool fix_runinng_average()
    {
      iterations_to_fixing++;
      if(iterations_to_fixing == 100000){
        //calculate a fresh average
        running_avg =0.0;
        for(int i = 0; i < num; i++) {
          //iteration through a queue.
          double val = vals.front();
          vals.pop();
          vals.push(val);

          //sum up all elements in queue
          running_avg += val;
        }
        //average the sum
        running_avg /= num;

        //prepare for new estimations
        iterations_to_fixing = 0;
        return true;
      }
      else {
        return false;
      }
    }
  public:
    average_filter(int n):
      num(n),
      running_avg(0.0),
      iterations_to_fixing(0)
    {
      for(int i = 0; i < num; i++) {
        vals.push(0.0);
      }
      fix_runinng_average();
    }

    T add(T val)
    {
      float old_el = vals.front();
      vals.pop();
      vals.push(val);

      //if running average has not been freshly calculated,
      //estimate it
      if(!fix_runinng_average()) {
        running_avg = (num * running_avg -old_el + val)/num;
      }

      return running_avg;
    }
  };
}

#endif



