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
              d_last_state(false),
              no_low(0), 
              no_high(0),
              prev_no_high(0), 
              prev_no_low(0),
              prev_half(0), 
              current_half(0)
    {
      reset_no_low();
      reset_no_high();
    }

    /*
     * Our virtual destructor.
     */
    mains_frequency_calc_impl::~mains_frequency_calc_impl()
    {
    }

    void mains_frequency_calc_impl::calc_frequency_average_over_period(float* mains_frequency_out, int prev_count, int current_count, int current_position)
    {
      int total_period_length = prev_count + current_count;
      int start = current_position - total_period_length;

      std::ofstream outfile;
      outfile.open("data.txt", std::ios_base::app); // append instead of overwrite

      outfile << "Period Lenght:" << "\n";
      outfile << total_period_length << "\n";

      outfile << "Start:" << "\n";
      outfile << start << "\n";

      // for (int i = start; i < total_period_length; i++)
      // {
      //   mains_frequency_out[i] = (float)((prev_half + current_half) / 2.0);
      // }
    }

    void mains_frequency_calc_impl::calc_frequency_per_halfed_period(int current_count, int noutput_items)
    {
      float seconds_per_halfed_period = (float)((double)current_count / d_expected_sample_rate);

      float mains_frequency_per_half_period = float(1.0 / float(seconds_per_halfed_period + seconds_per_halfed_period));

      current_half = mains_frequency_per_half_period;
    }

    void mains_frequency_calc_impl::mains_threshold(float* mains_frequency_out, const float* frequenzy_in, int noutput_items)
    {
      // outfile << "HI" << "\n";
      // outfile << d_hi << "\n";
      // outfile << "LO" << "\n";
      // outfile << d_lo << "\n";

      for (int i = 0; i < noutput_items; i++)
      {
        if ((float)(frequenzy_in[i]) > d_hi && !d_last_state) 
        {
            calc_frequency_per_halfed_period(no_low, i);

            calc_frequency_average_over_period(mains_frequency_out, prev_no_high, no_low, noutput_items);

            reset_no_low();
            
            prev_half = current_half;

            d_last_state = true;
            no_high++;
        } 
        else if ((float)(frequenzy_in[i]) > d_hi && d_last_state) 
        {
            no_high++;
        } 
        else if ((float)(frequenzy_in[i]) > d_lo &&  (float)(frequenzy_in[i]) < d_hi && d_last_state) 
        {  
            no_high++;
        }
        else if ((float)(frequenzy_in[i]) < d_lo && d_last_state)
        {
            calc_frequency_per_halfed_period(no_high, i);

            calc_frequency_average_over_period(mains_frequency_out, prev_no_low, no_high, noutput_items);

            reset_no_high();

            prev_half = current_half;

            d_last_state = false;
            no_low++;
        }
        else if ((float)(frequenzy_in[i]) < d_lo && !d_last_state)
        {
          no_low++;
        }
        else if ((float)(frequenzy_in[i]) > d_lo &&  (float)(frequenzy_in[i]) < d_hi && !d_last_state)
        {
          no_low++;
        }
        else
        {
          std::cerr << "ERROR: Lower and Upper threshold are asynchronous!" << "\n";
        }
        // outfile << "F: " << (float)(frequenzy_in[i]) << " " << "H: " << no_high << " " << "L: " << no_low << " " << "S: " << d_last_state << "\n";
      }
    }

    void mains_frequency_calc_impl::reset_no_low()
    {
      prev_no_low = no_low;
      no_low = 0;
    }

    void mains_frequency_calc_impl::reset_no_high()
    {
      prev_no_low = no_high;
      no_high = 0;
    }

    void mains_frequency_calc_impl::reset_last_state()
    {
      d_last_state = false;
    }

    int
    mains_frequency_calc_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float* samples_in =  (const float*)input_items[0];
      float* mains_frequency_out = (float*)output_items[0];

      float current_half_frequency = 0.0;

      mains_threshold(mains_frequency_out, samples_in, noutput_items);

      // memset(mains_frequency_out, 0.0, noutput_items * sizeof(*mains_frequency_out));

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

