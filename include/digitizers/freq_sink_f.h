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
      int64_t  timestamp;            // nanoseconds UTC
      int64_t  trigger_timestamp;    // nanoseconds UTC
      uint32_t status;
      uint32_t number_of_bins;       // number of bins
    };

    /*!
     * \brief Frequency sink mode
     * \ingroup digitizers
     */
    enum DIGITIZERS_API freq_sink_mode_t
    {
      FREQ_SINK_MODE_TRIGGERED     = 0,
      FREQ_SINK_MODE_STREAMING     = 1,
      FREQ_SINK_MODE_SEQUENCE      = 2,
      FREQ_SINK_MODE_SNAPSHOT      = 3
    };

    /*!
     * \brief <+description of block+>
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API freq_sink_f : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<freq_sink_f> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::freq_sink_f.
       *
       * To avoid accidental use of raw pointers, digitizers::freq_sink_f's
       * constructor is in a private implementation
       * class. digitizers::freq_sink_f::make is the public interface for
       * creating new instances.
       */
      static sptr make(std::string name, std::string unit, int nbins, int nmeasurements, freq_sink_mode_t mode);

      /*!
       * \brief Specifies snapshot times within the beam-in-to-beam-out window.
       *
       * Relevant in snapshot acquisition mode only. Snapshot times are specified in seconds relative
       * to the actual beam-in event.
       *
       * \param points snapshot points
       */
      virtual void set_snapshot_points(const std::vector<float> &points) = 0;

      /*!
       * \brief Get signal metadata (signal name and unit).
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
       * Note those properties are exported in the form of vectors (arrays) in order for the
       * interface to be consistent with the FESA class interface.
       *
       * User is required to read out all the available measurements at once, else the data is
       * lost, manning the three vectors (frequency, magnitude and phase) should be of sufficient
       * size. Note no vector resizing is NOT performed by this block. Initially all the vectors
       * are cleared.
       *
       * \param nr_measurements number of measurements to read
       * \param measurements measurement info
       * \param frequency frequency bins
       * \param magnitude magnitude spectra
       * \param phase phase spectra
       * \returns number of actual measurements read
       */
      virtual size_t get_measurements(size_t nr_measurements,
              std::vector<spectra_measurement_t> &measurements,
              std::vector<float> &frequency,
              std::vector<float> &magnitude,
              std::vector<float> &phase) = 0;

      /*!
       * \brief Register a callable, called whenever a predefined number of measurements is available.
       *
       * Note, only one callable can be registered for a given sink.
       *
       * \param callback callback to be called once the buffer is full
       * \param ptr a void pinter that is passed to the ::freq_data_available_cb_t function
       */
      virtual void set_measurements_available_callback(data_available_cb_t callback, void *ptr) = 0;

      virtual int get_nbins() = 0;
      virtual int get_nmeasurements() = 0;

      /*!
       * \brief Gets mode.
       * \returns frequency sink mode
       */
      virtual freq_sink_mode_t get_mode() = 0;

      /*!
       * \brief Stops collecting measurements and wait data readout.
       *
       * Applicable in sequence acquisition mode only.
       *
       */
      virtual void notify_data_ready() = 0;

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_FREQ_SINK_F_H */

