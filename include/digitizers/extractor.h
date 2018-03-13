/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
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


#ifndef INCLUDED_DIGITIZERS_EXTRACTOR_H
#define INCLUDED_DIGITIZERS_EXTRACTOR_H

#include <digitizers/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Extracts a subset of triggered data.
     *
     * This block uses acq_info tags in order to detect triggered data, namely the triggered_data
     * flag of the acq_info tag. With acq_info tag number of pre- and post-trigger samples is provided.
     *
     * Based on pre-trigger and post-trigger window configuration parameters and user delays provided
     * with the acq_info tag a subset of samples is extracted as shown below:
     *
     *   [ all pre-trigger samples       |   all post-trigger samples                ]
     * step a)
     *   .          [ pre-trigger window |  post-trigger window           ]          .
     * step b)
     *                                   ----> actual delay
     *   .          .    [                   |                                ]      .
     *
     * In step a) a desired pre-trigger and post-trigger window is determined and in step b) a complete
     * window is shifted by actual_delay including: constant delay, user defined delay and realignment delay.
     *
     * \ingroup digitizers
     */
    class DIGITIZERS_API extractor : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<extractor> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::extractor.
       *
       * To avoid accidental use of raw pointers, digitizers::extractor's
       * constructor is in a private implementation
       * class. digitizers::extractor::make is the public interface for
       * creating new instances.
       *
       * \param drop_non_trigger_data Instructs the block to output only samples tagged with a
       * 'triggered_data' tag. Other samples are dropped.
       */
      static sptr make(float post_trigger_window, float pre_trigger_window=0.0);

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_EXTRACTOR_H */

