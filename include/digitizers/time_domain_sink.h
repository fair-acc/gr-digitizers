/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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
      TIME_SINK_MODE_STREAMING     = 1
    };

    /*!
     * \brief GNU Radio sink for exporting time-domain data into FESA control system.
     *
     * This sink support two modes, a) triggered b) streaming mode. Implementation wise both
     * modes are implemented the same, that is received data is stored in data buffers until
     * read-out by the FESA. Sink mode is only used to relay this information to the FESA
     * side.
     *
     * Note buffer size can be set to one to achieve sample-by-sample acquisition or it can be
     * set to something bigger, e.g. 16k to publish triggered data. In streaming mode it is a
     * good practice to set number of buffers large enough to cover for a readout delay, e.g.
     * 200ms.
     *
     * It should be noted, that this block is still consuming input samples if all the buffers
     * are full, meaning the samples are lost.
     *
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
       * number of trigger samples to be consumed at once. Similar is valid for other two
       * acquisition modes as well.
       *
       * In order to allow for e.g. double-buffering strategy, parameter number of buffers is
       * provided allowing the users to specify a measurement history depth where each measurement
       * consists of a single buffer.
       *
       * \param name signal name
       * \param unit signal unit
       * \param samp_rate expected sample rate in Hz
       * \param buffer_size buffer size
       * \param nr_buffers number of buffers
       * \param mode time sink mode which is mostly used by FESA to determine how to handle the sink
       *
       * \returns shared_ptr to a new instance
       */
      static sptr make(std::string name, std::string unit, float samp_rate, size_t buffer_size,
              size_t nr_buffers, time_sink_mode_t mode);

      /*!
       * \brief Get signal metadata, such as signal name, timebase and unit.
       * \returns metadata
       */
      virtual signal_metadata_t get_metadata() = 0;

      /*!
       * \brief Read time domain data.
       *
       * User is required to read out all the available samples at once, else the data
       * is lost. Essentially this means that the value and error buffers should be large enough
       * to hold a complete buffer.
       *
       * \param nr_items_to_read number of items to read
       * \param values values
       * \param errors error estimates
       * \param info meassurement timestamp and status
       * \returns number of actual items read
       */
      virtual size_t get_items(size_t nr_items_to_read, float *values, float *errors, measurement_info_t *info) = 0;

      /*!
       * \brief Register a callable, called whenever a predefined number of samples is available.
       *
       * Note, only one callable can be registered for a given sink.
       *
       * \param callback callback to be called once the buffer is full
       * \param ptr a void pinter that is passed to the gr::digitizers::data_available_cb_t function
       */
      virtual void set_callback(data_available_cb_t callback, void *ptr) = 0;

      /*!
       * \brief Gets buffer size.
       * \returns buffer size in samples
       */
      virtual size_t get_buffer_size() = 0;

      /*!
       * \brief Returns expected sample rate in Hz.
       */
      virtual float get_sample_rate() = 0;

      /*!
       * \brief Gets mode.
       * \returns time sink mode
       */
      virtual time_sink_mode_t get_sink_mode() = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TIME_DOMAIN_SINK_H */

