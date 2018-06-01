/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_BLOCK_AGGREGATION_H
#define INCLUDED_DIGITIZERS_BLOCK_AGGREGATION_H

#include <digitizers/api.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/firdes.h>

namespace gr {
  namespace digitizers {


  /**
     * \brief Block aggregation. User can choose different algorithms to apply to the data.
     *
     * The blocks inside this block are connected as they are in the diagram provided,
     * with the exception of the helper circuit, which is implemented in a single block.
     * User can adjust the parameters in runtime, but the algorithmID has to be chosen
     * beforehand.
     * \ingroup digitizers
     */
    class DIGITIZERS_API block_aggregation : virtual public gr::hier_block2
    {
     public:
      typedef boost::shared_ptr<block_aggregation> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::block_aggregation.
       *
       * \param alg_id Chosen algorithm of the circuit that later maps to an enum(valid:0-6).
       * \param decim decimation factor.
       * \param delay The delay of the samples on output.
       * \param fir_taps user defined FIR-filter taps.
       * \param low_freq lower frequency boundary.
       * \param up_freq upper frequency boundary.
       * \param tr_width transition width for frequency boundaries.
       * \param fb_user_taps feed backward user taps.
       * \param fw_user_taps feed forward user taps.
       * \param samp_rate Sampling rate of the whole circuit.
       */
      static sptr make(int alg_id,
          int decim,
          int delay,
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate);

      /**
       * \brief Updates the circuit when any of the parameters change.
       *
       * \param delay The delay of the samples on output.
       * \param fir_taps user defined FIR-filter taps.
       * \param low_freq lower frequency boundary.
       * \param up_freq upper frequency boundary.
       * \param tr_width transition width for frequency boundaries.
       * \param fb_user_taps feed backward user taps.
       * \param fw_user_taps feed forward user taps.
       * \param samp_rate Sampling rate of the whole circuit.
       */
      virtual void update_design(int delay,
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate) = 0;

      /*!
       * \brief Update the firdes taps.
       *
       * See gr::filter::firdes::low_pass and gr::filter::firdes::band_pass methods for more details
       * (those two methods are used internally). Note, sampling frequency is static and cannot be changed
       * by using this method.
       */
      virtual void update_design(float delay,
        const std::vector<double> &fb_user_taps,
        const std::vector<double> &fw_user_taps,
        double gain,
        double low_cutoff_freq,     // Hz center of transition band
        double high_cutoff_freq,    // Hz center of transition band
        double transition_width,    // Hz width of transition band
        gr::filter::firdes::win_type,
        double beta = 6.76) = 0;        // used only with Kaiser
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_AGGREGATION_H */

