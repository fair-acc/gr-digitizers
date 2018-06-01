/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_AMPLITUDE_AND_PHASE_HELPER_IMPL_H
#define INCLUDED_DIGITIZERS_AMPLITUDE_AND_PHASE_HELPER_IMPL_H

#include <digitizers/amplitude_and_phase_helper.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief this block helps calculate the phase and amplitude
     * with a reference signal repsonse.
     */
    class amplitude_and_phase_helper_impl : public amplitude_and_phase_helper
    {
     public:
      amplitude_and_phase_helper_impl();
      ~amplitude_and_phase_helper_impl();

      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_AMPLITUDE_AND_PHASE_HELPER_IMPL_H */

