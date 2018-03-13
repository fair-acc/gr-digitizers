/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
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
#include "block_custom_filter_impl.h"
#include <math.h>
namespace gr {
  namespace digitizers {

    // make a custom implementation.
    block_custom_filter::sptr
    block_custom_filter::make(algorithm_id_t alg_id,
      int decimation,
      const std::vector<float> &fir_taps,
      double low_freq,
      double up_freq,
      double tr_width,
      const std::vector<double> &fb_user_taps,
      const std::vector<double> &fw_user_taps,
      double samp_rate)
    {
      block_custom_filter::sptr return_ptr;
      switch(alg_id){
      case FIR_LP:
        return_ptr = gnuradio::get_initial_sptr
          (new block_custom_filter_fir_lp(decimation,fir_taps, low_freq, up_freq,
             tr_width, fb_user_taps, fw_user_taps, samp_rate));
        break;
      case FIR_BP:
        return_ptr = gnuradio::get_initial_sptr
          (new block_custom_filter_fir_bp(decimation, fir_taps, low_freq, up_freq,
             tr_width, fb_user_taps, fw_user_taps, samp_rate));
        break;
      case FIR_CUSTOM:
        return_ptr = gnuradio::get_initial_sptr
          (new block_custom_filter_fir_custom(decimation, fir_taps, low_freq, up_freq,
             tr_width, fb_user_taps, fw_user_taps, samp_rate));
        break;
      case FIR_CUSTOM_FFT:
        return_ptr = gnuradio::get_initial_sptr
          (new block_custom_filter_fir_fft(decimation, fir_taps, low_freq, up_freq,
             tr_width, fb_user_taps, fw_user_taps, samp_rate));
        break;
      case IIR_LP:
        return_ptr = gnuradio::get_initial_sptr
          (new block_custom_filter_iir_lp(decimation, fir_taps, low_freq, up_freq,
             tr_width, fb_user_taps, fw_user_taps, samp_rate));
        break;
      case IIR_HP:
        return_ptr = gnuradio::get_initial_sptr
          (new block_custom_filter_iir_hp(decimation, fir_taps, low_freq, up_freq,
             tr_width, fb_user_taps, fw_user_taps, samp_rate));
        break;
      case IIR_CUSTOM:
        return_ptr = gnuradio::get_initial_sptr
          (new block_custom_filter_iir_custom(decimation, fir_taps, low_freq, up_freq,
             tr_width, fb_user_taps, fw_user_taps, samp_rate));
        break;
      default:
        throw std::runtime_error("No algorithm with specified ID!");
        break;
      }
      return return_ptr;
    }

    /*!
     * Actual implementations of different algorithm paths follow.
     */

    // ALGORITHM 0 (FIR LOW PASS)
    block_custom_filter_fir_lp::block_custom_filter_fir_lp(int decimation,
      const std::vector<float> &fir_taps,
      double low_freq,
      double up_freq,
      double tr_width,
      const std::vector<double> &fb_user_taps,
      const std::vector<double> &fw_user_taps,
      double samp_rate)
      : gr::hier_block2("block_custom_filter",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(1, 1, sizeof(float)))
    {
      auto filter_design = gr::filter::firdes::low_pass(1,
        samp_rate,up_freq, tr_width);
      d_fir_filter = gr::filter::fir_filter_fff::make(decimation, filter_design);
      connect(self(), 0, d_fir_filter, 0);
      connect(d_fir_filter, 0, self(), 0);
    }

    block_custom_filter_fir_lp::~block_custom_filter_fir_lp()
    {
    }

    void
    block_custom_filter_fir_lp::update_design(
     const std::vector<float> &fir_taps,
     double low_freq,
     double up_freq,
     double tr_width,
     const std::vector<double> &fb_user_taps,
     const std::vector<double> &fw_user_taps,
     double samp_rate)
    {
      auto filter_design = gr::filter::firdes::low_pass(1, samp_rate,
        up_freq, tr_width, gr::filter::firdes::win_type::WIN_HAMMING);
      d_fir_filter->set_taps(filter_design);
    }

    // ALGORITHM 1 (FIR BAND PASS)
    block_custom_filter_fir_bp::block_custom_filter_fir_bp(int decimation,
      const std::vector<float> &fir_taps,
      double low_freq,
      double up_freq,
      double tr_width,
      const std::vector<double> &fb_user_taps,
      const std::vector<double> &fw_user_taps,
      double samp_rate)
      : gr::hier_block2("block_custom_filter",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(1, 1, sizeof(float)))
    {
      auto filter_design = gr::filter::firdes::band_pass(1,
        samp_rate, low_freq, up_freq, tr_width );
      fir_filter = gr::filter::fir_filter_fff::make(decimation, filter_design);
      connect(self(), 0, fir_filter, 0);
      connect(fir_filter, 0, self(), 0);
    }

    block_custom_filter_fir_bp::~block_custom_filter_fir_bp()
    {
    }

    void
    block_custom_filter_fir_bp::update_design(
      const std::vector<float> &fir_taps,
      double low_freq,
      double up_freq,
      double tr_width,
      const std::vector<double> &fb_user_taps,
      const std::vector<double> &fw_user_taps,
      double samp_rate)
    {
      auto filter_design = gr::filter::firdes::band_pass(1,
        samp_rate, low_freq, up_freq, tr_width);
      fir_filter->set_taps(filter_design);
    }

    // ALGORITHM 2 (FIR CUSTOM)
    block_custom_filter_fir_custom::block_custom_filter_fir_custom(int decimation,
      const std::vector<float> &fir_taps,
      double low_freq,
      double up_freq,
      double tr_width,
      const std::vector<double> &fb_user_taps,
      const std::vector<double> &fw_user_taps,
      double samp_rate)
      : gr::hier_block2("block_custom_filter",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(1, 1, sizeof(float)))
    {
      fir_filter = gr::filter::fir_filter_fff::make(decimation, fir_taps);
      connect(self(), 0, fir_filter, 0);
      connect(fir_filter, 0, self(), 0);
    }

    block_custom_filter_fir_custom::~block_custom_filter_fir_custom()
    {
    }

    void
    block_custom_filter_fir_custom::update_design(
      const std::vector<float> &fir_taps,
      double low_freq,
      double up_freq,
      double tr_width,
      const std::vector<double> &fb_user_taps,
      const std::vector<double> &fw_user_taps,
      double samp_rate)
    {
      fir_filter->set_taps(fir_taps);
    }

    //    ALGORITHM 3 (FIR FFT)
    block_custom_filter_fir_fft::block_custom_filter_fir_fft(int decimation,
      const std::vector<float> &fir_taps,
      double low_freq,
      double up_freq,
      double tr_width,
      const std::vector<double> &fb_user_taps,
      const std::vector<double> &fw_user_taps,
      double samp_rate)
      : gr::hier_block2("block_custom_filter",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(1, 1, sizeof(float)))
    {
      fft_filter = gr::filter::fft_filter_fff::make(decimation, fir_taps);
      connect(self(), 0, fft_filter, 0);
      connect(fft_filter, 0, self(), 0);
    }

    block_custom_filter_fir_fft::~block_custom_filter_fir_fft()
    {
    }

    void
    block_custom_filter_fir_fft::update_design(
      const std::vector<float> &fir_taps,
      double low_freq,
      double up_freq,
      double tr_width,
      const std::vector<double> &fb_user_taps,
      const std::vector<double> &fw_user_taps,
      double samp_rate)
    {
      fft_filter->set_taps(fir_taps);
    }

    // ALGORITHM 4 (IIR LOW PASS)
    block_custom_filter_iir_lp::block_custom_filter_iir_lp(int decimation,
      const std::vector<float> &fir_taps,
      double low_freq,
      double up_freq,
      double tr_width,
      const std::vector<double> &fb_user_taps,
      const std::vector<double> &fw_user_taps,
      double samp_rate)
      : gr::hier_block2("block_custom_filter",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(1, 1, sizeof(float)))
    {
      iir_filter1 = gr::filter::single_pole_iir_filter_ff::make(calculate_alpha(samp_rate, up_freq), 1);
      iir_filter2 = gr::filter::single_pole_iir_filter_ff::make(calculate_alpha(samp_rate, up_freq), 1);
      keep_one = gr::blocks::keep_one_in_n::make(sizeof(float), decimation);
      connect(self(), 0, iir_filter1, 0);
      connect(iir_filter1, 0, iir_filter2, 0);
      connect(iir_filter2, 0, keep_one, 0);
      connect(keep_one, 0, self(), 0);
    }

    block_custom_filter_iir_lp::~block_custom_filter_iir_lp()
    {
    }

    double
    block_custom_filter_iir_lp::calculate_alpha(double sample_rate, double upper_frequency)
    {
      double freq_max_corr = upper_frequency / (2.0 * (sqrt(2.0) - 1.0));
      double ts = 1.0 / sample_rate;
      return (2.0 * M_PI * ts * freq_max_corr) / ((2.0 * M_PI * ts * freq_max_corr) + 1.0);
    }

    void
    block_custom_filter_iir_lp::update_design(
      const std::vector<float> &fir_taps,
      double low_freq,
      double up_freq,
      double tr_width,
      const std::vector<double> &fb_user_taps,
      const std::vector<double> &fw_user_taps,
      double samp_rate)
    {
      iir_filter1->set_taps(calculate_alpha(samp_rate, up_freq));
      iir_filter2->set_taps(calculate_alpha(samp_rate, up_freq));
    }

    // ALGORITHM 5 (IIR HIGH PASS)
    block_custom_filter_iir_hp::block_custom_filter_iir_hp(int decimation,
      const std::vector<float> &fir_taps,
      double low_freq,
      double up_freq,
      double tr_width,
      const std::vector<double> &fb_user_taps,
      const std::vector<double> &fw_user_taps,
      double samp_rate)
      : gr::hier_block2("block_custom_filter",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(1, 1, sizeof(float)))
    {
      double alphaHp = calc_alphaHP(samp_rate, low_freq);
      std::vector<double> fw_user_taps_double;
      std::vector<double> fb_user_taps_double;
      fw_user_taps_double.push_back(0.0);
      fw_user_taps_double.push_back(alphaHp);
      fw_user_taps_double.push_back(-alphaHp);
      fb_user_taps_double.push_back(1.0);
      fb_user_taps_double.push_back(alphaHp);

      iir_filter = gr::filter::iir_filter_ffd::make(fw_user_taps_double, fb_user_taps_double, true);
      keep_one = gr::blocks::keep_one_in_n::make(sizeof(float), decimation);
      connect(self(), 0, iir_filter, 0);
      connect(iir_filter, 0, keep_one, 0);
      connect(keep_one, 0, self(), 0);
    }

    double
    block_custom_filter_iir_hp::calc_alphaHP(double sample_rate, double lower_frequency)
    {
      double ts = 1.0 / sample_rate;
      return 1.0 / ((2.0 * M_PI * ts * lower_frequency) + 1.0);
    }

    block_custom_filter_iir_hp::~block_custom_filter_iir_hp()
    {
    }

    void
    block_custom_filter_iir_hp::update_design(
      const std::vector<float> &fir_taps,
      double low_freq,
      double up_freq,
      double tr_width,
      const std::vector<double> &fb_user_taps,
      const std::vector<double> &fw_user_taps,
      double samp_rate)
    {
      double alphaHp = calc_alphaHP(samp_rate, low_freq);
      std::vector<double> fw_user_taps_double;
      std::vector<double> fb_user_taps_double;
      fw_user_taps_double.push_back(0.0);
      fw_user_taps_double.push_back(alphaHp);
      fw_user_taps_double.push_back(-alphaHp);
      fb_user_taps_double.push_back(1.0);
      fb_user_taps_double.push_back(alphaHp);
      iir_filter->set_taps(fw_user_taps_double, fb_user_taps_double);
    }

    // ALGORITHM 6 (IIR CUSTOM)
    block_custom_filter_iir_custom::block_custom_filter_iir_custom(int decimation,
      const std::vector<float> &fir_taps,
      double low_freq,
      double up_freq,
      double tr_width,
      const std::vector<double> &fb_user_taps,
      const std::vector<double> &fw_user_taps,
      double samp_rate)
      : gr::hier_block2("block_custom_filter",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(1, 1, sizeof(float)))
    {
      iir_filter = gr::filter::iir_filter_ffd::make(fw_user_taps, fb_user_taps, false);
      keep_one = gr::blocks::keep_one_in_n::make(sizeof(float), decimation);
      connect(self(), 0, iir_filter, 0);
      connect(iir_filter, 0, keep_one, 0);
      connect(keep_one, 0, self(), 0);
    }

    block_custom_filter_iir_custom::~block_custom_filter_iir_custom()
    {
    }

    void
    block_custom_filter_iir_custom::update_design(
      const std::vector<float> &fir_taps,
      double low_freq,
      double up_freq,
      double tr_width,
      const std::vector<double> &fb_user_taps,
      const std::vector<double> &fw_user_taps,
      double samp_rate)
    {
      iir_filter->set_taps(fw_user_taps, fb_user_taps);
    }

  } /* namespace digitizers */
} /* namespace gr */

