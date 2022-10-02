/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_FREQ_SINK_F_H
#define INCLUDED_DIGITIZERS_FREQ_SINK_F_H

#include <digitizers/sink_common.h>

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {


    /*!
     * \brief Spectra measurement metadata.
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
    struct DIGITIZERS_API spectra_measurement_t
    {
      float timebase;                // distance between measurements (in seconds)

      int64_t  timestamp;            // nanoseconds UTC
      int64_t  trigger_timestamp;    // nanoseconds UTC
      uint32_t status;
      uint32_t number_of_bins;       // number of bins
      uint32_t lost_count;           // number of measurements lost
    };

    /*!
     * \brief Frequency sink mode
     * \ingroup digitizers
     */
    enum DIGITIZERS_API freq_sink_mode_t
    {
      FREQ_SINK_MODE_TRIGGERED     = 0,
      FREQ_SINK_MODE_STREAMING     = 1
    };

    /*!
     * \brief <+description of block+>
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API freq_sink_f : virtual public gr::sync_block
    {
     public:
      typedef std::shared_ptr<freq_sink_f> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::freq_sink_f.
       *
       * At object construction nbuffers is allocated, each holding n measurements. Clients are
       * supposed to retrieve/read a complete buffer at once else the data is lost.
       *
       * \param name Signal name
       * \param samp_rate Expected measurement sample rate in Hz (number of measurements per second)
       * \param nbins Number of bins (or vector size)
       * \param nmeasurements Number of measurements to publish at once
       * \param nbuffers Number of buffers
       * \param mode Sink mode
       */
      static sptr make(std::string name, float samp_rate, size_t nbins,
              size_t nmeasurements, size_t nbuffers, freq_sink_mode_t mode);

      /*!
       * \brief Get signal metadata (note unit name is not used).
       * \returns metadata
       */
      virtual signal_metadata_t get_metadata() = 0;

      /*!
       * \brief Gets measurement data.
       *
       * S single measurement consists of:
       *  - Measurement metadata such as timestamp of the first sample used in spectra analysis,
       *    associated trigger timestamp, status, number of bins etc.
       *  - frequency scale
       *  - magnitude spectra
       *  - phase spectra
       *
       * User is required to read out all the available measurements at once, else the data is
       * lost, manning the three buffers (frequency, magnitude and phase) should be of sufficient
       * size.
       *
       * if nullptr is passed in to any of the output arguments the data is silently dropped.
       *
       * \param nr_measurements number of measurements to read
       * \param metadata measurement info
       * \param frequency frequency bins
       * \param magnitude magnitude spectra
       * \param phase phase spectra
       * \returns number of actual measurements read
       */
      virtual size_t get_measurements(size_t nr_measurements,
              spectra_measurement_t *metadata, float *frequency, float *magnitude, float *phase) = 0;

      /*!
       * \brief Register a callable, called whenever a predefined number of measurements (nmeasurements)
       * is available.
       *
       * Note, only one callable can be registered for a given sink.
       *
       * \param callback callback to be called once the buffer is full
       * \param ptr a void pinter that is passed to the gr::digitizers::freq_data_available_cb_t function
       */
      virtual void set_callback(data_available_cb_t callback, void *ptr) = 0;

      /*!
       * \brief Number of bins of spectra measurement.
       */
      virtual size_t get_nbins() = 0;

      /*!
       * \brief Number of measurement stored in the buffer.
       */
      virtual size_t get_nmeasurements() = 0;

      /*!
       * \brief Estimated measurement sample rate.
       */
      virtual float get_sample_rate() = 0;

      /*!
       * \brief Gets mode.
       * \returns frequency sink mode
       */
      virtual freq_sink_mode_t get_sink_mode() = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_FREQ_SINK_F_H */

