/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_TAGS_H
#define INCLUDED_DIGITIZERS_TAGS_H

#include <digitizers/api.h>
#include <gnuradio/tags.h>
#include <cassert>

namespace gr {
  namespace digitizers {

    char const * const acq_info_tag_name = "acq_info";

    /*!
     * \brief A convenience structure holding information about the measurement.
     * \ingroup digitizers
     *
     * General use:
     *
     * Digitizers are expected to attach the 'acq_info' tag to all the enabled channels including
     * digital ports. In streaming mode this should be done whenever a new chunk of data is obtained
     * from the device.
     *
     * If trigger detection is enabled in streaming mode, then the acq_info tag should be attached to
     * the output streams whenever a trigger is detected. In this case the triggered_data field should
     * be set to True allowing other post-processing modules (i.e. B.2 Demux) to work with triggered data
     * only.
     *
     * Note the acq_info tag should be attached to the first pre-trigger sample and not to a sample
     * where the trigger/edge is actually detected!!!
     *
     * The acq_info tag contains two delays, namely the user_delay and the actual_delay. The following ascii
     * figure depicts relation between different terms:
     *
     * user_delay                 |--->          (3 us)
     * realignment_delay          |--------->    (9 us)
     * actual_delay               |------------> (3 + 9 = 12 us)
     *
     * Those delays are used for two purposes:
     *
     * 1) Time synchronization:
     *    User delay is added to the actual timestamp of a given sample the tag is attached to. Field
     *    timestamp therefore contains timestamp calculated like this:
     *
     *    timestamp = <actual acquisition timestamp> + user_delay
     *
     * 2) Extraction of triggered data
     *    Actual delay, that is user delay and edge-trigger based delay synchronization is accounted
     *    for when extracting triggered data. See B.2 block description for more details (extractor.h).
     */
    struct DIGITIZERS_API acq_info_t
    {
      int64_t timestamp;         // timestamp (UTC nanoseconds) of the first sample in the collection
      double timebase;           // distance between samples in seconds
      double user_delay;         // see description above
      double actual_delay;       // see description above
      uint32_t samples;          // number of samples in the collection
      uint32_t status;           // acquisition status

      uint64_t offset;           // tag offset

      // triggered data specifics
      uint32_t pre_samples;
      int64_t trigger_timestamp; // timestamp of the trigger (UTC nanoseconds) without realignment applied (Either WR Stamp or Trigger from scope )
      int64_t last_beam_in_timestamp; // last actual beam-in timestamp (UTC nanoseconds)
      bool triggered_data;       // Reads true if trigger related fields are relevant
    };

    /*!
     * \brief Factory function for creating acq_info tags.
     */
    inline gr::tag_t
    make_acq_info_tag(const acq_info_t &acq_info)
    {
      gr::tag_t tag;
      tag.key = pmt::intern(acq_info_tag_name);
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

    /*!
     * \brief Factory function for creating acq_info tags.
     */
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

    /*!
     * \brief Converts acq_info tag into acq_info_t struct.
     */
    inline acq_info_t
    decode_acq_info_tag(const gr::tag_t &tag)
    {
      assert(pmt::symbol_to_string(tag.key) == acq_info_tag_name);

      if (!pmt::is_tuple(tag.value) || pmt::length(tag.value) != 10)
      {
          std::ostringstream message;
          message << "Exception in " << __FILE__ << ":" << __LINE__ << ": invalid acq_info tag format";
          throw std::runtime_error(message.str());
      }

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

    /*!
     * \brief Returns true if the acq_info tag is attached to the triggered data.
     */
    inline bool
    contains_triggered_data(const gr::tag_t &tag)
    {
      assert(pmt::symbol_to_string(tag.key) == acq_info_tag_name);
      assert(pmt::is_tuple(tag.value));

      auto tag_tuple = pmt::to_tuple(tag.value);
      return pmt::to_bool(tuple_ref(tag_tuple, 9));
    }

    char const * const timebase_info_tag_name = "timebase_info";

    /*!
     * \brief Factory function for creating timebase_info tags.
     */
    inline gr::tag_t
    make_timebase_info_tag(double timebase)
    {
      gr::tag_t tag;
      tag.key = pmt::intern(timebase_info_tag_name);
      tag.value = pmt::from_double(timebase);
      return tag;
    }

    /*!
     * \brief Returns timebase stored within the timebase_info tag.
     */
    inline double
    decode_timebase_info_tag(const gr::tag_t &tag)
    {
      assert(pmt::symbol_to_string(tag.key) == timebase_info_tag_name);
      return pmt::to_double(tag.value);
    }

    char const * const trigger_tag_name = "trigger";

    /*!
     * \brief Factory function for creating trigger tags.
     */
    inline gr::tag_t
    make_trigger_tag(uint64_t offset=0)
    {
      gr::tag_t tag;
      tag.key = pmt::intern(trigger_tag_name);
      tag.offset = offset;
      return tag;
    }

    /*!
     * \brief Name of the WR event tag.
     */
    char const * const wr_event_tag_name = "wr_event";

    /*!
     * \brief A convenience structure holding information about the WR timing event.
     * \ingroup digitizers

     */
    struct DIGITIZERS_API wr_event_t
    {
      std::string event_id;
      int64_t timestamp;              // timestamp of the event (UTC nanoseconds)
      int64_t last_beam_in_timestamp; // last known actual beam-in timestamp (UTC nanoseconds)

      uint64_t offset;                // tag offset

      bool time_sync_only;            // event should be used for time-synchronization only
      bool realignment_required;      // this event requires realignment
    };

    /*!
     * \brief Factory function for creating wr event tags.
     */
    inline gr::tag_t
    make_wr_event_tag(const wr_event_t &event)
    {
      gr::tag_t tag;
      tag.key = pmt::intern(wr_event_tag_name);
      tag.value =  pmt::make_tuple(
              pmt::string_to_symbol(event.event_id),
              pmt::from_uint64(static_cast<uint64_t>(event.timestamp)),
              pmt::from_uint64(static_cast<uint64_t>(event.last_beam_in_timestamp)),
              pmt::from_bool(event.time_sync_only),
              pmt::from_bool(event.realignment_required)
              );
      tag.offset = event.offset;
      return tag;
    }

    /*!
     * \brief Converts wr event tag into wr_event_t struct.
     */
    inline wr_event_t
    decode_wr_event_tag(const gr::tag_t &tag)
    {
      assert(pmt::symbol_to_string(tag.key) == wr_event_tag_name);

      if (!pmt::is_tuple(tag.value) || pmt::length(tag.value) != 5)
      {
          std::ostringstream message;
          message << "Exception in " << __FILE__ << ":" << __LINE__ << ": invalid wr_event tag format";
          throw std::runtime_error(message.str());
      }

      wr_event_t event;
      event.offset = tag.offset;

      auto tag_tuple = pmt::to_tuple(tag.value);
      event.event_id = pmt::symbol_to_string(tuple_ref(tag_tuple, 0));
      event.timestamp = static_cast<int64_t>(pmt::to_uint64(tuple_ref(tag_tuple, 1)));
      event.last_beam_in_timestamp = static_cast<int64_t>(pmt::to_uint64(tuple_ref(tag_tuple, 2)));
      event.time_sync_only = pmt::to_bool(tuple_ref(tag_tuple, 3));
      event.realignment_required = pmt::to_bool(tuple_ref(tag_tuple, 4));

      return event;
    }

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TAGS_H */

