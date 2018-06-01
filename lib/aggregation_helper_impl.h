/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_AGGREGATION_HELPER_IMPL_H
#define INCLUDED_DIGITIZERS_AGGREGATION_HELPER_IMPL_H

#include <digitizers/aggregation_helper.h>

namespace gr {
  namespace digitizers {

    class aggregation_helper_impl : public aggregation_helper
    {
     private:
      float d_sigma_mult_fact;
      int d_count;
      int d_decimation;

     public:
      aggregation_helper_impl(int decim, float sigma_mult);
      ~aggregation_helper_impl();

      void forecast(int noutput_items, gr_vector_int &ninput_items_required) override;

      int general_work(int noutput_items,
          gr_vector_int &ninput_items,
          gr_vector_const_void_star &input_items,
          gr_vector_void_star &output_items) override;

     protected:
      void update_design(float sigma_mult) override;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_AGGREGATION_HELPER_IMPL_H */

