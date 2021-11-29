/* -*- c++ -*- */
/*
 * Copyright 2021 fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/io_signature.h>
#include "mains_frequency_calc_impl.h"

namespace gr {
  namespace digitizers_39 {

    mains_frequency_calc::sptr
    mains_frequency_calc::make(double expected_sample_rate, float low_threshold, float high_threshold)
    {
      return gnuradio::make_block_sptr<mains_frequency_calc_impl>(
        expected_sample_rate, low_threshold, high_threshold);
    }


    /*
     * The private constructor
     */
    mains_frequency_calc_impl::mains_frequency_calc_impl(double expected_sample_rate, float low_threshold, float high_threshold)
      : gr::sync_block("mains_frequency_calc",
              gr::io_signature::make(1 /* min inputs */, 1 /* max inputs */, sizeof(float)),
              gr::io_signature::make(1 /* min outputs */, 1 /*max outputs */, sizeof(float))),
              d_lo(low_threshold),
              d_hi(high_threshold),
              d_last_state(0.0),
              no_low(0), 
              no_high(0)
    {
      reset_no_low();
      reset_no_hight();
    }

    /*
     * Our virtual destructor.
     */
    mains_frequency_calc_impl::~mains_frequency_calc_impl()
    {
    }

    void mains_frequency_calc_impl::calc_average_ms_of_period(float* ms_low ,float* ms_high, bool* is_threshold, int noutput_items)
    {
      for (int i = 0; i < noutput_items; i++) 
      {
        if (is_threshold[i] == 0.0 && counter_low[i] != 0)
        {
          ms_low[i] = ;
        }
        else if (is_threshold[i] == 0.0 && counter_high[i] != 0)
        {
          ms_high[i] = ;
        }
      }
    }

    void mains_frequency_calc_impl::mains_threshold(float* is_threshold, int* counter_low, int* counter_high, const float* frequenzy_in, int noutput_items)
    {
      for (int i = 0; i < noutput_items; i++) 
      {
        if (frequenzy_in[i] > d_hi) 
        {
            // save prior state
            if (d_last_state == 0.0)
            {
              counter_low[i] = no_low;
              reset_no_low();
            }
            is_threshold[i] = 1.0;
            d_last_state = 1.0;
            no_high++;
        } 
        else if (frequenzy_in[i] < d_lo) 
        {
            // save prior state
            if (d_last_state == 1.0)
            {
              counter_high[i] = no_high;
              reset_no_hight();
            }
            is_threshold[i] = 0.0;
            d_last_state = 0.0;
            no_low++;
        } 
        else
            if (d_last_state == 0.0)
            {
              reset_no_hight();
              //no_low++;
            }
            else
            {
              reset_no_low();
              //no_high++;
            }
        }
    }

    void reset_no_low()
    {
      int no_low = 0;
    }

    void reset_no_hight()
    {
      int no_high = 0;
    }

    int
    mains_frequency_calc_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float* f_in =  (const float*)input_items[0];
      float* nf_out = (float*)output_items[0];

      bool* is_mains_threshold = (bool*)malloc(noutput_items*sizeof(bool));

      int* counter_low = (int*)malloc(noutput_items*sizeof(float));
      memset(counter_low, 0, noutput_items * sizeof(*counter_low));
      int* counter_high = (int*)malloc(noutput_items*sizeof(float));
      memset(counter_high, 0, noutput_items * sizeof(*counter_high));

      float* ms_low = (float*)malloc(noutput_items*sizeof(float));
      memset(ms_low, 0.0, noutput_items * sizeof(*ms_low));
      float* ms_high = (float*)malloc(noutput_items*sizeof(float));
      memset(ms_high, 0.0, noutput_items * sizeof(*ms_high));
      
      // Threshold
      mains_threshold(is_mains_threshold, f_in, noutput_items);



      return noutput_items;
    }

  } /* namespace digitizers_39 */
} /* namespace gr */

