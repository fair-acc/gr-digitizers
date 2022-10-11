/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_SINK_COMMON_H
#define INCLUDED_DIGITIZERS_SINK_COMMON_H

#include <cstdint>
#include <digitizers/api.h>
#include <gnuradio/tag.h>
#include <string>

namespace gr {
namespace digitizers {

/*!
 * \brief Holds static information about the signal.
 *
 * \ingroup digitizers
 */
struct DIGITIZERS_API signal_metadata_t {
    std::string unit;
    std::string name;
};

/*!
 * \brief Measurement information.
 *
 * Note, trigger_timestamp is the the same timestamp as provided to the time-realignment block
 * (via the add_timing_event method), else it contains the local timestamp of the first
 * post-trigger sample.
 *
 * Member-variable timestamp contains timestamp of the first pre-trigger sample, or if there is
 * none the timestamp of the first sample.
 *
 * \ingroup digitizers
 */
struct DIGITIZERS_API measurement_info_t {
    double   timebase;     // distance between samples (in seconds)
    double   user_delay;   // in seconds
    double   actual_delay; // in seconds (including realignment and user delay)

    int64_t  timestamp;         // nanoseconds UTC
    int64_t  trigger_timestamp; // nanoseconds UTC
    uint32_t status;            // Refer to gr::digitizers::channel_status_t
    uint32_t pre_trigger_samples;
    uint32_t post_trigger_samples;

    uint64_t samples_lost; // samples lost since last readout
};

/*!
 * \brief Holding information about the timestamp (trigger or acquisition) when the callback
 * is called.
 *
 * Note, trigger_timestamp is the the same timestamp as provided to the time-realignment block
 * (via the add_timing_event method), else it contains the local timestamp of the first
 * post-trigger sample.
 *
 * \ingroup digitizers
 */
struct DIGITIZERS_API data_available_event_t {
    int64_t     trigger_timestamp; // trigger timestamp (without any realignment or user delay)
    std::string signal_name;       // signal name
};

/*!
 * \brief Callback deceleration
 */
typedef void (*data_available_cb_t)(const data_available_event_t *evt, void *userdata);
typedef void (*cb_copy_data_t)(const float *values,
        std::size_t                         values_size,
        const float                        *errors,
        std::size_t                         errors_size,
        std::vector<gr::tag_t>             &tags,
        void                               *userdata);

}
} // namespace gr::digitizers

#endif /* INCLUDED_DIGITIZERS_SINK_COMMON_H */
