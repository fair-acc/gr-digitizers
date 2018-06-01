/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_BLOCK_CUSTOM_FILTER_H
#define INCLUDED_DIGITIZERS_BLOCK_CUSTOM_FILTER_H

#include <digitizers/api.h>
#include <gnuradio/hier_block2.h>
#include "digitizers/status.h"

namespace gr {
  namespace digitizers {

    /*!
     * \brief This block implements the algorithm paths defined in its diagram
     * as seperate implementations.
     * \ingroup digitizers
     *
     * The user can choose an algorithmID that corresponds to the algorithm path.
     * The parameters that the block takes are a union of all of the parameters the
     * different algorithms need.
     *
     * The algorithm has to be chosen beforehand, while the parameters of the algorithms
     * can be changed runtime.
     */
    class DIGITIZERS_API block_custom_filter : virtual public gr::hier_block2
    {
     public:
      typedef boost::shared_ptr<block_custom_filter> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of testBlocks::block_custom_filter.
       *
       * To avoid accidental use of raw pointers, testBlocks::block_custom_filter's
       * constructor is in a private implementation
       * class. testBlocks::block_custom_filter::make is the public interface for
       * creating new instances.
       */
      static sptr make(algorithm_id_t alg_id,
          int decimation,
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate);

      /*!
       * \brief Sets the new aruments to the algorithms on one of the lines.
       * Has to be implemented by the algorithm path.
       *
       *
       * \param fir_taps user defined FIR-filter taps.
       * \param low_freq lower frequency boundary.
       * \param up_freq upper frequency boundary.
       * \param tr_width transition width for frequency boundaries.
       * \param fb_user_taps feed backward user taps.
       * \param fw_user_taps feed forward user taps.
       * \param samp_rate Sampling rate of the whole circuit.
       */
      virtual void update_design(
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate) = 0;

      /*!
       * \brief returns a delay approximation that each of the algorithm paths should
       * implement. The delay corresponds to the number of samples for which the
       * output signal is behind. this number is always a positive decimal value.
       *
       *   ^
       *   |            ___________
       *   |___________|
       *   L-----------x---------->
       *               |         INPUT
       *   ^           |--->
       *   |             delay = 4;
       *   |                _______
       *   |_______________/
       *   L---------------x------>
       *                         SIGNAL
       */
      virtual double get_delay_approximation() = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_CUSTOM_FILTER_H */

