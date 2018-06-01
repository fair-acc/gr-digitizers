/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_INTERLOCK_GENERATION_FF_IMPL_H
#define INCLUDED_DIGITIZERS_INTERLOCK_GENERATION_FF_IMPL_H

#include <digitizers/interlock_generation_ff.h>
#include <digitizers/tags.h>

namespace gr {
  namespace digitizers {

    class interlock_generation_ff_impl : public interlock_generation_ff
    {
     private:
      float d_max_max; // max value to be used for upper bounds evaluation
      float d_max_min;

      //callback stuff
      bool d_interlock_issued;
      interlock_cb_t d_callback;
      void *d_user_data;

      // timing
      acq_info_t d_acq_info;

     public:
      interlock_generation_ff_impl(float max_min, float max_max);

      ~interlock_generation_ff_impl();

      bool start() override;

      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items) override;

      void set_callback(interlock_cb_t callback, void *ptr);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_INTERLOCK_GENERATION_FF_IMPL_H */

