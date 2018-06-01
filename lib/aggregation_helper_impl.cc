/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cmath>
#include <gnuradio/io_signature.h>
#include "aggregation_helper_impl.h"

namespace gr {
  namespace digitizers {

    aggregation_helper::sptr
    aggregation_helper::make(int decim, float sigmaMult)
    {
      return gnuradio::get_initial_sptr
        (new aggregation_helper_impl(decim, sigmaMult));
    }

    /*
     * The private constructor
     */
    aggregation_helper_impl::aggregation_helper_impl(int decim, float sigma_mult)
      : gr::block("aggregation_helper",
              gr::io_signature::make(3, 3, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float))),
        d_sigma_mult_fact(sigma_mult),
        d_count(decim),
        d_decimation(decim)
    {
      set_relative_rate(1.0 / decim);
      set_tag_propagation_policy(tag_propagation_policy_t::TPP_DONT);
    }

    /*
     * Our virtual destructor.
     */
    aggregation_helper_impl::~aggregation_helper_impl()
    {
    }

    void
    aggregation_helper_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
    {
      unsigned ninputs = ninput_items_required.size();
      for(unsigned i = 0; i < ninputs; i++)
        ninput_items_required[i] = noutput_items * d_decimation + history() - 1;
    }

    int
    aggregation_helper_impl::general_work(int noutput_items,
        gr_vector_int &ninput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *a = (const float *) input_items[0];
      const float *b = (const float *) input_items[1];
      const float *c = (const float *) input_items[2];
      float *out = (float *) output_items[0];

      // Do <+signal processing+>
      int nr_input_items = *std::min_element(ninput_items.begin(), ninput_items.end());

      int i, j;
      for(i = 0, j = 0; i < nr_input_items && j < noutput_items; i++) {
        d_count--;
        if(d_count <= 0) {
          out[j] = std::sqrt(std::fabs(a[i] - b[i]*b[i]) + (d_sigma_mult_fact * c[i]*c[i]));
          d_count = d_decimation;
          j++;
        }
      }

      //Sigma is not supposed to include tags so they are ignored.
      consume_each(i);
      return j;
    }

    void
    aggregation_helper_impl::update_design(float sigma_mult)
    {
      d_sigma_mult_fact = sigma_mult;
    }
  } /* namespace digitizers */
} /* namespace gr */

