/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_FUNCTION_FF_IMPL_H
#define INCLUDED_DIGITIZERS_FUNCTION_FF_IMPL_H

#include <digitizers/function_ff.h>
#include <digitizers/tags.h>
#include <boost/thread/mutex.hpp>

namespace gr {
  namespace digitizers {

    class function_ff_impl : public function_ff
    {
     private:
      acq_info_t d_acq_info;

      std::vector<float> d_ref, d_min, d_max;
      std::vector<int64_t> d_timing;

      boost::mutex d_mutex;

     public:
      function_ff_impl(int decimation);
      ~function_ff_impl();

      bool start() override;

      void set_function(const std::vector<float> &timing,
              const std::vector<float> &ref,
              const std::vector<float> &min,
              const std::vector<float> &max) override;

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items) override;

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_FUNCTION_FF_IMPL_H */

