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

    /**
     * @brief Construct a new mains frequency calc::make object
     * 
     * @param expected_sample_rate The sample rate per second relaied by the signal source
     * @param low_threshold The thresold determining the begining and duration of the negative curve
     * @param high_threshold The thresold determining the begining and duration of the positive curve
     */
    mains_frequency_calc::make(double expected_sample_rate, float low_threshold, float high_threshold)
    {
      return gnuradio::make_block_sptr<mains_frequency_calc_impl>(
        expected_sample_rate, low_threshold, high_threshold);
    }

    /**
     * @brief Construct a new mains frequency calc impl::mains frequency calc impl object
     * 
     * @param expected_sample_rate The sample rate per second relaied by the signal source
     * @param low_threshold The thresold determining the begining and duration of the negative curve
     * @param high_threshold The thresold determining the begining and duration of the positive curve
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
              current_half_frequency(0),
              average_frequency(25.0), 
              d_alpha(0.007)
    {
      reset_no_low();
      reset_no_high();
    }

    /**
     * @brief Destroy the mains frequency calc impl::mains frequency calc impl object
     * 
     */
    mains_frequency_calc_impl::~mains_frequency_calc_impl()
    {
    }

    /**
     * @brief Calcualtes the mains frequency over half of a period with the expected sample rate
     * 
     * @param current_count The length of the detected lower or upper part of the period, in number of samples
     */
    void mains_frequency_calc_impl::calc_frequency_per_halfed_period(int current_count)
    {
      float seconds_per_halfed_period = (float)((double)current_count / d_expected_sample_rate);

      current_half_frequency = float(1.0 / float(seconds_per_halfed_period + seconds_per_halfed_period));
    }

    /**
     * @brief Calculates a mains frequency average over a portion of the current value and the previous average
     * 
     */
    void mains_frequency_calc_impl::calc_current_average()
    {
      average_frequency = d_alpha * current_half_frequency + (1 - d_alpha)  * average_frequency;
    }

    /**
     * @brief Detects the flanks in order to messure the lenght of the upcoming period, calculating the previous period if if the flank falls or rises
     * 
     * @param mains_frequency_out The average mains frequency value buffer
     * @param samples_in The incoming samples of the signal which is observed
     * @param noutput_items The samples currently available for cumputation
     */
    void mains_frequency_calc_impl::detect_mains_frequency_over_half_period(float* mains_frequency_out, const float* samples_in, int noutput_items)
    {
      for (int i = 0; i < noutput_items; i++)
      {
        if ((float)(samples_in[i]) > d_hi && !d_last_state) 
        {
          calc_frequency_per_halfed_period(no_low);

          calc_current_average();

          reset_no_low();

          d_last_state = true;
          no_high++;
        } 
        else if ((float)(samples_in[i]) > d_hi && d_last_state) 
        {
          no_high++;
        } 
        else if ((float)(samples_in[i]) > d_lo &&  (float)(samples_in[i]) < d_hi && d_last_state) 
        {  
          no_high++;
        }
        else if ((float)(samples_in[i]) < d_lo && d_last_state)
        {
          calc_frequency_per_halfed_period(no_high);

          calc_current_average();

          reset_no_high();

          d_last_state = false;
          no_low++;
        }
        else if ((float)(samples_in[i]) < d_lo && !d_last_state)
        {
          no_low++;
        }
        else if ((float)(samples_in[i]) > d_lo &&  (float)(samples_in[i]) < d_hi && !d_last_state)
        {
          no_low++;
        }
        else
        {
          std::cerr << "ERROR: No matching condition!" << "\n";
        }

        mains_frequency_out[i] = average_frequency;
      }
    }

    /**
     * @brief Setting the count of samples which are representing the negative curve to zero
     * 
     */
    void mains_frequency_calc_impl::reset_no_low()
    {
      no_low = 0;
    }

    /**
     * @brief Setting the count of samples which are representing the positive curve to zero
     * 
     */
    void mains_frequency_calc_impl::reset_no_high()
    {
      no_high = 0;
    }

    /**
     * @brief Main | Core block routine
     * 
     * @param noutput_items The samples currently available for cumputation
     * @param input_items The item vector containing the input items
     * @param output_items  The item vector that will contain the output items
     * @return number of output items
     */
    int mains_frequency_calc_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float* samples_in =  (const float*)input_items[0];
      float* mains_frequency_out = (float*)output_items[0];

      detect_mains_frequency_over_half_period(mains_frequency_out, samples_in, noutput_items);

      return noutput_items;
    }

  } /* namespace digitizers_39 */
} /* namespace gr */

