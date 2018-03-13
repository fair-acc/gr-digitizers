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


#ifndef INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_DECIMATED_H
#define INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_DECIMATED_H

#include <digitizers/api.h>
#include <gnuradio/hier_block2.h>

namespace gr {
  namespace digitizers {

  /*!
       * \brief The block returns the response from lower frequency to the upper frequency.
       * Where:
       * lower frequency = center_fq - 0.5 * width_f,
       * upper frequency = center_fq + 0.5 * width_fq,
       * for each given pair of frequencies. The user also needs to specify the
       * time_of_transition, which tells the block how many seconds each transition from the
       * user specified windows should take. The block holds the last frequency window,
       * until a beam in tag arrives. When it does, it starts changing its frequency window,
       * corresponding to the parameters set.
       * Graphical representation for this:
       *
       *  \verbatim
       *     ^
       * Y(f)|
       *     |         ^
       *     |         X
       *     |         X'beam in'
       *     |         X
       *     |         ˇ
       *     |          ___
       *     |         |   \
       *     |         |    \
       *     |_ _ _ _ _|     \______ _ _ _ _ _
       *     |
       *     |                                 ...
       *     |_ _ _ _ _          ___ _ _ _ _ _
       *     |         |_______-/
       *     L---------*--*--*--*------------------------->
       *               S  T  T  T  E                    f(t)
       *  \endverbatim
       *
       *  where S denotes the start of the function, T the transitions,
       *  and E the end of the function. Transitions are evenly spaced apart,
       *  which is set by the time_of_transition variable. Important to note,
       *  the signal is chunked up into vectors, of size window_size, and
       *  each of these gets analysed with the goertzl transform blocks, that
       *  are evenly spaced apart, from the lower frequency, to the upper
       *  frequency.
       *
       *  It also decimates the input, by the delta_t.
       * \ingroup digitizers
       *
       */
    class DIGITIZERS_API stft_goertzl_dynamic_decimated : virtual public gr::hier_block2
    {
     public:
      typedef boost::shared_ptr<stft_goertzl_dynamic_decimated> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::stft_goertzl_dynamic.
       *
       * \param samp_rate sampling rate of the signal.
       * \param delta_t time between each analysis of signal
       * \param window_size size of the stft window
       * \param nbins number of bins for the goertzel basded f-response
       * \param time_of_transition length in seconds of the window effectiveness
       * \param center_fq Central frequency of the corresponding frequency window
       * \param width_fq Width of the corresponding frequency window
       */
      static sptr make(double samp_rate,
        double delta_t,
        int window_size,
        int nbins,
        double time_of_transition,
        std::vector<double> center_fq,
        std::vector<double> width_fq);

      /**
       * \brief Set a new sample rate.
       *
       * Interrupts the block, and fixes all the blocks
       *
       * \param samp_rate The desired new sample rate
       */
      virtual void set_samp_rate(double samp_rate) = 0;

      /*!
       * \brief update frequency function definitions
       *
       * \param time_of_transition How long the transition lasts in seconds,
       * from one pair of f_window bounds to the next.
       * \param center_fq Central frequency of the corresponding f_window bounds
       * \param width_fq Width of the corresponding f_window bounds
       */
      virtual void update_bounds(double time_of_transition,
        std::vector<double> center_fq,
        std::vector<double> width_fq) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_STFT_GOERTZL_DYNAMIC_DECIMATED_H */

