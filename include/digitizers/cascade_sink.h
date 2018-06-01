/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_CASCADE_SINK_H
#define INCLUDED_DIGITIZERS_CASCADE_SINK_H

#include <digitizers/api.h>
#include <gnuradio/hier_block2.h>
#include <digitizers/time_domain_sink.h>
#include <digitizers/post_mortem_sink.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Receives a signal and error estimation, and publishes it to FESA at 10Hz and 1Hz. The
     * signal_name is used for all four exposed signals, where each signal gets a correspondent signal
     * post-fix, i.e.:
     * signal_name@1000Hz
     * signal_name@100Hz
     * signal_name@10Hz
     * signal_name@1Hz
     *
     * Note signal with sample rate of 1000Hz is published to FESA at 10Hz, with number of elements of
     * 100. Similar is valid for the 100Hz signal.
     *
     * This block also includes two different post mortem sinks, one connected to the raw input, and the
     * other connected to the 1kHz decimated input. FESA signal names:
     * signal_namePMRAW
     * signal_namePM1000Hz
     *
     * \ingroup digitizers
     */
    class DIGITIZERS_API cascade_sink : virtual public gr::hier_block2
    {
     public:
      typedef boost::shared_ptr<cascade_sink> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::cascade_sink.
       *
       * \param alg_id Chosen algorithm of the circuit that later maps to an enum(valid:0-6).
       * \param delay The delay of the samples on output.
       * \param fir_taps user defined FIR-filter taps.
       * \param low_freq lower frequency boundary.
       * \param up_freq upper frequency boundary.
       * \param tr_width transition width for frequency boundaries.
       * \param fb_user_taps feed backward user taps.
       * \param fw_user_taps feed forward user taps.
       * \param samp_rate Sampling rate of the whole circuit.
       * \param pm_buffer Post mortem buffer size in seconds
       * \param signal_name signal name
       * \param unit_name signal unit
       */
      static sptr make(int alg_id,
          int delay,
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate,
          float pm_buffer,
          std::string signal_name,
          std::string unit_name);

      /*!
       * \brief Returns all time-domain sinks contained by this module.
       */
      virtual std::vector<time_domain_sink::sptr> get_time_domain_sinks() = 0;

      /*!
       * \brief Returns post-mortem sinks contained by this module.
       */
      virtual std::vector<post_mortem_sink::sptr> get_post_mortem_sinks() = 0;

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_CASCADE_SINK_H */

