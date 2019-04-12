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
     * This sink support two modes, a) triggered b) streaming mode.
     * Implementation wise both modes are implemented the same. "Sink mode" is only of interest for the host Application
     *
     * On each incoming data package a callback is called which alows the host application to copy the data to its internal buffers
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
       * For streaming mode
       * Note in streaming acquisition mode the output_package_size argument determines the number
       * of samples to be consumed by the user at once. E.g. if sample-by-sample acquisition
       * is desired then the output_package_size should be set to 1.
       *
       * \param name signal name
       * \param unit signal unit
       * \param samp_rate expected sample rate in Hz
       * \param mode time sink mode which is mostly used by FESA to determine how to handle the sink
       * \param output_package_size output_package_size
       *
       * \returns shared_ptr to a new instance
       */
      static sptr make(std::string name, std::string unit, float samp_rate, time_sink_mode_t mode, size_t output_package_size);

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::time_domain_sink.
       *
       * For triggered mode
       *
       * \param name signal name
       * \param unit signal unit
       * \param samp_rate expected sample rate in Hz
       * \param mode time sink mode which is mostly used by FESA to determine how to handle the sink
       * \param pre_samples pre trigger samples
       * \param post_samples pre trigger samples
       *
       * \returns shared_ptr to a new instance
       */
      static sptr make(std::string name, std::string unit, float samp_rate, time_sink_mode_t mode, int pre_samples, int post_samples);

      /*!
       * \brief Get signal metadata, such as signal name, timebase and unit.
       * \returns metadata
       */
      virtual signal_metadata_t get_metadata() = 0;

      /*!
       * \brief Registers a callback which is called whenever a predefined number of samples is available.
       *
       * \param cb_copy_data callback in which the host application can copy the data
       */
      virtual void set_callback(cb_copy_data_t cb_copy_data, void* userdata) = 0;

      /*!
       * \brief Gets output package size
       * \returns output package size in samples
       */
      virtual size_t get_output_package_size() = 0;

      /*!
       * \brief Returns expected sample rate in Hz.
       */
      virtual float get_sample_rate() = 0;

      /*!
       * \brief Gets mode.
       * \returns time sink mode
       */
      virtual time_sink_mode_t get_sink_mode() = 0;

      virtual uint32_t get_pre_samples() = 0;

      virtual uint32_t get_post_samples() = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_TIME_DOMAIN_SINK_H */

