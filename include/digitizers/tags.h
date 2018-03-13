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


#ifndef INCLUDED_DIGITIZERS_TAGS_H
#define INCLUDED_DIGITIZERS_TAGS_H

#include <digitizers/api.h>
#include <gnuradio/tags.h>
#include <cassert>

namespace gr {
  namespace digitizers {


    /*!
     * \brief A convenience structure holding information about the measurement.
     * \ingroup digitizers
     *
     *
     * user delay                 |--->
     * realignment delay          |--------->
     * actual delay               |------------>
     *
     * Note, user delay might be a negative number.
     */
    struct DIGITIZERS_API acq_info_t
    {
      int64_t timestamp;         // timestamp (UTC nanoseconds) of the first sample in the collection
      double timebase;           // distance between samples in seconds
      double user_delay;
      double actual_delay;
      uint32_t samples;
      uint32_t status;

      uint64_t offset;

      // triggered data specifics
      uint32_t pre_samples;
      int64_t trigger_timestamp; // timestamp of the trigger (UTC nanoseconds) without realignment applied
      int64_t last_beam_in_timestamp; // last actual beam-in timestamp
      bool triggered_data;
    };

    inline gr::tag_t
    make_acq_info_tag(const acq_info_t &acq_info)
    {
      gr::tag_t tag;
      tag.key = pmt::intern("acq_info");
      tag.value =  pmt::make_tuple(
              pmt::from_uint64(static_cast<uint64_t>(acq_info.timestamp)),
              pmt::from_double(acq_info.timebase),
              pmt::from_double(acq_info.user_delay),
              pmt::from_double(acq_info.actual_delay),
              pmt::from_long(static_cast<long>(acq_info.samples)),
              pmt::from_long(static_cast<long>(acq_info.status)),
              pmt::from_long(static_cast<long>(acq_info.pre_samples)),
              pmt::from_uint64(static_cast<uint64_t>(acq_info.trigger_timestamp)),
              pmt::from_uint64(static_cast<uint64_t>(acq_info.last_beam_in_timestamp)),
              pmt::from_bool(acq_info.triggered_data)
              );
      tag.offset = acq_info.offset;
      return tag;
    }

    inline gr::tag_t
    make_trigger_tag(uint32_t pre_trigger_samples, uint32_t post_trigger_samples,
            uint32_t status, double timebase, int64_t timestamp=-1)
    {
        acq_info_t info;
        info.timestamp = timestamp;
        info.trigger_timestamp = -1;
        info.timebase = timebase;
        info.actual_delay = 0.0;
        info.user_delay = 0.0;
        info.pre_samples = pre_trigger_samples;
        info.samples = post_trigger_samples;
        info.status = status;
        info.offset = 0;
        info.triggered_data = true;
        info.last_beam_in_timestamp = -1;

        return make_acq_info_tag(info);
    }

    inline acq_info_t
    decode_acq_info_tag(const gr::tag_t &tag)
    {
      assert(pmt::symbol_to_string(tag.key) == "acq_info");
      assert(pmt::is_tuple(tag.value));

      acq_info_t acq_info;
      acq_info.offset = tag.offset;

      auto tag_tuple = pmt::to_tuple(tag.value);
      acq_info.timestamp = static_cast<int64_t>(pmt::to_uint64(tuple_ref(tag_tuple, 0)));
      acq_info.timebase = pmt::to_double(tuple_ref(tag_tuple, 1));
      acq_info.user_delay = pmt::to_double(tuple_ref(tag_tuple, 2));
      acq_info.actual_delay = pmt::to_double(tuple_ref(tag_tuple, 3));
      acq_info.samples = static_cast<uint32_t>(pmt::to_long(tuple_ref(tag_tuple, 4)));
      acq_info.status = static_cast<uint32_t>(pmt::to_long(tuple_ref(tag_tuple, 5)));
      acq_info.pre_samples = static_cast<uint32_t>(pmt::to_long(tuple_ref(tag_tuple, 6)));
      acq_info.trigger_timestamp = static_cast<int64_t>(pmt::to_uint64(tuple_ref(tag_tuple, 7)));
      acq_info.last_beam_in_timestamp = static_cast<int64_t>(pmt::to_uint64(tuple_ref(tag_tuple, 8)));
      acq_info.triggered_data = pmt::to_bool(tuple_ref(tag_tuple, 9));

      return acq_info;
    }

    inline bool
    contains_triggered_data(const gr::tag_t &tag)
    {
      assert(pmt::symbol_to_string(tag.key) == "acq_info");
      assert(pmt::is_tuple(tag.value));

      auto tag_tuple = pmt::to_tuple(tag.value);
      return pmt::to_bool(tuple_ref(tag_tuple, 9));
    }

    inline gr::tag_t
    make_timebase_info_tag(double timebase)
    {
      gr::tag_t tag;
      tag.key = pmt::intern("timebase_info");
      tag.value = pmt::from_double(timebase);
      return tag;
    }

    inline double
    decode_timebase_info_tag(const gr::tag_t &tag)
    {
      assert(pmt::symbol_to_string(tag.key) == "timebase_info");
      return pmt::to_double(tag.value);
    }


  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TAGS_H */

