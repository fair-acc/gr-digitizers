/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <gnuradio/math.h>
#include "stft_goertzl_dynamic_impl.h"
#include "digitizers/tags.h"

namespace gr {
  namespace digitizers {

    stft_goertzl_dynamic::sptr
    stft_goertzl_dynamic::make(double samp_rate,
        double delta_t,
        int window_size,
        int nbins,
        double window_effectiveness,
        std::vector<double> center_fq,
        std::vector<double> width_fq)
    {
      return gnuradio::get_initial_sptr
        (new stft_goertzl_dynamic_impl(samp_rate,
            delta_t,
            window_size,
            nbins,
            window_effectiveness,
            center_fq,
            width_fq));
    }

    stft_goertzl_dynamic_impl::stft_goertzl_dynamic_impl(double samp_rate,
        double delta_t,
        int window_size,
        int nbins,
        double window_effectiveness,
        std::vector<double> center_fq,
        std::vector<double> width_fq)
      : gr::sync_decimator("stft_goertzl_dynamic",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(3, 3, sizeof(float) * nbins),
              window_size),
              d_samp_length(1.0/samp_rate),
              d_delta_t(delta_t),
              d_nbins(nbins),
              d_last_tag_offset(0),
              d_vectors_passed(0),
              d_lows(0),
              d_highs(0),
              d_times(0),
              d_fq_access()
    {
      update_bounds(window_effectiveness, center_fq, width_fq);

      //start sequence at the end
      d_last_tag_offset = -(d_times.back() * samp_rate * 1.1);
    }

    stft_goertzl_dynamic_impl::~stft_goertzl_dynamic_impl()
    {
    }



    std::vector<double>
    stft_goertzl_dynamic_impl::calc_bin_freqs()
    {
      std::vector<tag_t> tags;
      get_tags_in_range(tags, 0, nitems_read(0), nitems_read(0) + decimation(),  pmt::string_to_symbol("acq_info"));
      d_vectors_passed++;
      for(auto tag : tags) {
        auto acq_tag = decode_acq_info_tag(tag);
        if (acq_tag.triggered_data) {
          double beam_in_offset_secs = (acq_tag.last_beam_in_timestamp - acq_tag.timestamp)/1000000000;
          d_last_tag_offset = acq_tag.offset - beam_in_offset_secs / d_samp_length;
          d_vectors_passed = 0;
          break;
        }
      }

      //relative time of current samples from the last tag
      double relative_time = (nitems_read(0) - d_last_tag_offset) * d_samp_length
          + (d_delta_t - (d_samp_length * decimation())) * d_vectors_passed;

      //find index of the last window in sequence
      size_t i = 0;
      for(; i < d_times.size(); i++) {
        if(d_times.at(i)>relative_time) {
          break;
        }
      }

      //get lower and upper bound of the window
      double lower_agg = d_lows.back();
      double upper_agg = d_highs.back();
      if(i < d_times.size()) {
        // in sequence
        double prev_time = 0.0;
        if(i != 0) {
          //not start of sequence
          prev_time = d_times.at(i-1);
        }
        //linear interpolation
        double window_offset = (relative_time - prev_time) / (d_times.at(i) - prev_time);
        lower_agg = d_lows.at(i) * (1.0 - window_offset) + d_lows.at(i+1) * window_offset;
        upper_agg = d_highs.at(i) * (1.0 - window_offset) + d_highs.at(i+1) * window_offset;
      }

      //calculate frequencies for each bin
      std::vector<double> bin_fqs;
      double fq_step = (upper_agg - lower_agg) / (1.0 * d_nbins);
      for(int i = 0; i < d_nbins; i++) {
        bin_fqs.push_back(lower_agg + (i * fq_step));
      }
      return bin_fqs;
    }

    int
    stft_goertzl_dynamic_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *in = (const float *) input_items[0];
      float *mag = (float *) output_items[0];
      float *phs = (float *) output_items[1];
      float *fqs = (float *) output_items[2];

      std::vector<double> freqs;
      {
        //set a new spectral window
        std::unique_lock<std::mutex> lk(d_fq_access);
        freqs = calc_bin_freqs();
      }
      int win_size = decimation();

      //do frequency analysis for each bin
      for(int i = 0; i < d_nbins; i++){
        float w = 2.0 * M_PI * freqs.at(i) * d_samp_length;
        double wr = 2.0 * std::cos(w);
        double wi = std::sin(w);
        double d1 = 0.0;
        double d2 = 0.0;

        //goertzel magic
        for(int j = 0; j < win_size; j++) {
          double y = in[j] + wr * d1 - d2;
          d2 = d1;
          d1 = y;
        }
        double re = (0.5*wr*d1-d2)/win_size;
        double im = (wi*d1)/win_size;

        //transform from carthesian to polar components
        mag[i] = std::sqrt((re * re) + (im * im));
        phs[i] = gr::fast_atan2f(re, im);
        //post frequency of each bin
        fqs[i] = freqs.at(i);
      }
      return 1;
    }

    //update interface

    void
    stft_goertzl_dynamic_impl::set_samp_rate(double samp_rate)
    {
      d_samp_length = 1.0/samp_rate;
    }

    void
    stft_goertzl_dynamic_impl::update_bounds(double window_effectiveness,
        std::vector<double> center_fq,
        std::vector<double> width_fq)
    {
      std::unique_lock<std::mutex> lk(d_fq_access);
      d_lows.clear();
      d_highs.clear();
      d_times.clear();
      for(size_t i = 0; i < center_fq.size(); i++) {
        double half_width = std::abs(width_fq.at(i))/2.0;
        d_lows.push_back(center_fq.at(i) - half_width);
        d_highs.push_back(center_fq.at(i) + half_width);
      }
      double relative_absolute_effectiveness = 0.0;
      for(size_t i = 0; i < center_fq.size()-1; i++) {
        relative_absolute_effectiveness += window_effectiveness;
        d_times.push_back(relative_absolute_effectiveness);
      }
    }


  } /* namespace digitizers */
} /* namespace gr */

