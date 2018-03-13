/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
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


#ifndef INCLUDED_DIGITIZERS_POST_MORTEM_SINK_H
#define INCLUDED_DIGITIZERS_POST_MORTEM_SINK_H

#include <digitizers/api.h>
#include <digitizers/sink_common.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Post-mortem sink
     *
     * This is a sync data block (or pass trough data block) allowing the users to
     * expose post-mortem data to the FESA control system. All incoming samples are
     * copied to the outputs and at the same time stored in a circular buffer.
     *
     * The circular buffer can be frozen by using the post_mortem_sink::freeze_buffer
     * method and the data can be obtained by using the post_mortem_sink::get_items
     * method.
     *
     * It is important to notice that calling the post_mortem_sink::get_data method
     * unfreezes the buffer meaning the clients must make sure that all the data is
     * read out in a single call to the post_mortem_sink::get_data method.
     *
     * This sink should be used for streaming data only because internally
     * it is assumed that items are evenly spaced between each other (important for
     * calculating timestamps).
     *
     * Incoming samples are passed trough while the buffer is frozen.
     *
     * \ingroup digitizers
     */
    class DIGITIZERS_API post_mortem_sink : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<post_mortem_sink> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::post_mortem_sink.
       *
       * \param name signal name
       * \param unit signal unit
       * \param buffer_size circular buffer size (or number of samples to buffer)
       *
       * \returns shared_ptr to a new instance
       */
      static sptr make(std::string name, std::string unit, size_t buffer_size);

      /*!
       * \brief Get signal metadata, such as signal name, timebase and unit.
       * \returns metadata
       */
      virtual signal_metadata_t get_metadata() = 0;

      /*!
       * \brief Returns the size of the circular buffer.
       * \returns buffer size in samples
       */
      virtual size_t get_buffer_size() = 0;

      /*!
       * \brief Freeze the post-mortem buffer.
       */
      virtual void freeze_buffer() = 0;

      /*!
       * \brief Read from a post-mortem buffer (circular buffer).
       *
       * The read function attempts to read nr_items_to_read samples from a circular buffer.
       * Note it is the user responsibility to read-out all the samples at once. A call to
       * this method unlocks the circular buffer.
       *
       * \param nr_items_to_read number of items to read
       * \param values values
       * \param errors error estimates
       * \param info measurement timestamp and status
       * \returns number of actual items read
       */
      virtual size_t get_items(size_t nr_items_to_read, float *values, float *errors, measurement_info_t *info) = 0;

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_POST_MORTEM_SINK_H */

