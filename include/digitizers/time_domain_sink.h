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


#ifndef INCLUDED_DIGITIZERS_TIME_DOMAIN_SINK_H
#define INCLUDED_DIGITIZERS_TIME_DOMAIN_SINK_H

#include <digitizers/api.h>
#include <digitizers/sink_common.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Time sink mode
     * \ingroup digitizers
     */
    enum DIGITIZERS_API time_sink_mode_t
    {
      TIME_SINK_MODE_TRIGGERED     = 0,
      TIME_SINK_MODE_STREAMING     = 1,
      TIME_SINK_MODE_STREAMING_SEQ = 2,
      TIME_SINK_MODE_SNAPSHOT = 3
    };

    /*!
     * \brief GNU Radio sink for exporting time-domain data into FESA control system.
     *
     * This sink support three modes, a) triggered b) streaming mode and c) snapshot.
     *
     * For the purpose of triggered mode, the sink stores samples in an internal buffer belonging
     * to the same timing event. Once all the samples are collected a user provided callback is
     * called. This blocks assumes the start of the trigger data to be indicated by using the
     * trigger tag. If buffer size is not sufficient to hold all the trigger data last N samples
     * are dropped and information about this condition is provided to the user when obtaining the
     * data (via status field).
     *
     * In triggered acquisition mode non-trigger data is dropped.
     *
     * In case of streaming acquisition mode, the incoming samples are saved to an internal
     * buffer until the buffer is full. Once the buffer is full a user provided callback is called.
     * Note, buffer size parameter should be set to 1 if sample-by-sample acquisition is desired.
     *
     * It should be noted, that the block is not consuming input samples in buffer is full.

     * \ingroup digitizers
     */
    class DIGITIZERS_API time_domain_sink : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<time_domain_sink> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::time_domain_sink.
       *
       * Note in streaming acquisition mode the buffer size argument determines the number
       * of samples to be consumed by the user at once. E.g. if sample-by-sample acquisition
       * is desired then the buffer size should be set to 1. In case of triggered acquisition
       * the buffer size parameter determines the maximum buffer size or better the maximum
       * number of trigger samples to be consumed at once.
       *
       * \param name signal name
       * \param unit signal unit
       * \param buffer_size buffer size
       * \param fast_data_sink if set to true the sink is consuming fast data
       *
       * \returns shared_ptr to a new instance
       */
      static sptr make(std::string name, std::string unit, size_t buffer_size, time_sink_mode_t mode);

      /*!
       * \brief Specifies snapshot times within the beam-in-to-beam-out window.
       *
       * Relevant in snapshot acquisition mode only.
       *
       * Snapshot times are specified in seconds relative to the actual beam-in event.
       *
       * \param points snapshot points
       */
      virtual void set_snapshot_points(const std::vector<float> &points) = 0;


      /*!
       * \brief Get signal metadata, such as signal name, timebase and unit.
       * \returns metadata
       */
      virtual signal_metadata_t get_metadata() = 0;

      /*!
       * \brief This function copy number of items user wants to read.
       *
       * User is required to read out all the available samples at once, else the data
       * is lost. Essentially this means that values and error buffers should be of buffer size
       * length.
       *
       * \param nr_items_to_read number of items to read
       * \param values values
       * \param errors error estimates
       * \param info meassurement timestamp and status
       * \returns number of actual items read
       */
      virtual size_t get_items(size_t nr_items_to_read, float *values, float *errors, measurement_info_t *info) = 0;

      /*!
       * \brief Returns number of samples stored in the buffer.
       * \returns items in the circular buffer
       */
      virtual size_t get_items_count() = 0;

      /*!
       * \brief Register a callable, called whenever a predefined number of samples
       * is available.
       *
       * Note, only one callable can be registered for a given sink. See gr::feval
       * and time_domain_sink::set_itmes_available_threshold.
       *
       * \param callback callback to be called once the buffer is full
       * \param ptr a void pinter that is passed to the ::data_available_cb_t function
       */
      virtual void set_items_available_callback(data_available_cb_t callback, void *ptr) = 0;

      /*!
       * \brief Gets size of the internal buffer in samples.
       * \returns buffer size in samples
       */
      virtual size_t get_buffer_size() = 0;

      /*!
       * \brief Gets mode.
       * \returns time sink mode
       */
      virtual time_sink_mode_t get_sink_mode() = 0;

      /*!
       * \brief Check if data is ready.
       *
       * TODO jgolob: Note this function has different meaning in different modes.
       *
       * \returns true if some data is available.
       */
      virtual bool is_data_rdy() = 0;

      virtual void notify_data_ready() = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TIME_DOMAIN_SINK_H */

