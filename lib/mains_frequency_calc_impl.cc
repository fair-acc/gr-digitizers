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
              d_expected_sample_rate(expected_sample_rate),
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

    void mains_frequency_calc_impl::calc_frequency_per_halfed_period(float* f_out, int count, int noutput_items)
    {
      int start = noutput_items - count;
      int end = noutput_items - 1;

      std::cout << "COUNT" << "\n" ;
      std::cout << count << "\n" ;
      std::cout << "----------------" << "\n" ;

      float seconds_per_halfed_period = (float)((double)count / d_expected_sample_rate);
      std::cout << "MS per halfed period" << "\n" ;
      std::cout << seconds_per_halfed_period << "\n" ;
      std::cout << "----------------" << "\n" ;

      float mains_frequency_per_half_period = float(1.0 / float(seconds_per_halfed_period + seconds_per_halfed_period));

      std::cout << "Mains Frequency:" << "\n" ;
      std::cout << mains_frequency_per_half_period << "\n" ;
      std::cout << "----------------" << "\n" ;
      // for (int i = start; i < end; i++) 
      // {
      //   f_out[i] = mains_frequency_per_half_period;
      // }
    }

    void mains_frequency_calc_impl::mains_threshold(float* f_out, const float* frequenzy_in, int noutput_items)
    {
      for (int i = 0; i < noutput_items; i++) 
      {
        if (frequenzy_in[i] > d_hi) 
        {
            // save prior state
            if (d_last_state == 0.0)
            {
              // std::cout << "" << "\n" ;
              // std::cout << "Positive Flank" << "\n" ;
              // std::cout << "" << "\n" ;
              // std::cout << "Count LOW" << "\n" ;
              // std::cout << no_low << "\n" ;
              // std::cout << "----------------" << "\n" ;
              calc_frequency_per_halfed_period(f_out, no_low, noutput_items);

              reset_no_low();
            }

            // is_threshold[i] = 1.0;
            d_last_state = 1.0;
            no_high++;
        } 
        else if (frequenzy_in[i] < d_lo) 
        {
            // save prior state
            if (d_last_state == 1.0)
            {
              // std::cout << "" << "\n" ;
              // std::cout << "Negative Flank:" << "\n" ;
              // std::cout << "" << "\n" ;
              // std::cout << "Count HIGH" << "\n" ;
              // std::cout << no_high << "\n" ;
              // std::cout << "----------------" << "\n" ;
              calc_frequency_per_halfed_period(f_out, no_high, noutput_items);

              reset_no_hight();
            }
            // is_threshold[i] = 0.0;
            d_last_state = 0.0;
            no_low++;
        } 
      }
    }

    void mains_frequency_calc_impl::reset_no_low()
    {
      no_low = 0;
    }

    void mains_frequency_calc_impl::reset_no_hight()
    {
      no_high = 0;
    }

    int
    mains_frequency_calc_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float* frequency_in =  (const float*)input_items[0];
      float* mains_frequency_out = (float*)output_items[0];

      mains_threshold(mains_frequency_out, frequency_in, noutput_items);

      memset(mains_frequency_out, 0.0, noutput_items * sizeof(*mains_frequency_out));

      // halfed_period_t* mains_data_pos_neg = (halfed_period_t*)malloc(noutput_items*sizeof(halfed_period_t));

      // int* counter_low = (int*)malloc(noutput_items*sizeof(float));
      // memset(counter_low, 0, noutput_items * sizeof(*counter_low));
      // int* counter_high = (int*)malloc(noutput_items*sizeof(float));
      // memset(counter_high, 0, noutput_items * sizeof(*counter_high));

      // float* ms_low = (float*)malloc(noutput_items*sizeof(float));
      // memset(ms_low, 0.0, noutput_items * sizeof(*ms_low));
      // float* ms_high = (float*)malloc(noutput_items*sizeof(float));
      // memset(ms_high, 0.0, noutput_items * sizeof(*ms_high));
      
      // Threshold

      return noutput_items;
    }

  } /* namespace digitizers_39 */
} /* namespace gr */

