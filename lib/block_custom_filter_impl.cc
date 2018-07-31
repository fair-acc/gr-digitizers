/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__ << ": No algorithm with specified ID: " << alg_id;
        throw std::runtime_error(message.str());
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
          gr::io_signature::make(1, 1, sizeof(float))),
          d_samp_rate(samp_rate)
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

    double
    block_custom_filter_fir_lp::get_delay_approximation()
    {
      double length = 1.0 * d_fir_filter->taps().size();
      double delay_approx =  length / (2.0 * d_samp_rate);
      return delay_approx;
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
          gr::io_signature::make(1, 1, sizeof(float))),
          d_samp_rate(samp_rate)
    {
      auto filter_design = gr::filter::firdes::band_pass(1,
        samp_rate, low_freq, up_freq, tr_width );
      d_fir_filter = gr::filter::fir_filter_fff::make(decimation, filter_design);
      connect(self(), 0, d_fir_filter, 0);
      connect(d_fir_filter, 0, self(), 0);
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
      d_fir_filter->set_taps(filter_design);
    }

    double
    block_custom_filter_fir_bp::get_delay_approximation()
    {
      double length = 1.0 * d_fir_filter->taps().size();
      double delay_approx =  length / (2.0 * d_samp_rate);
      return delay_approx;
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
          gr::io_signature::make(1, 1, sizeof(float))),
          d_samp_rate(samp_rate)
    {
      d_fir_filter = gr::filter::fir_filter_fff::make(decimation, fir_taps);
      connect(self(), 0, d_fir_filter, 0);
      connect(d_fir_filter, 0, self(), 0);
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
      d_fir_filter->set_taps(fir_taps);
    }

    double
    block_custom_filter_fir_custom::get_delay_approximation()
    {
      double length = 1.0 * d_fir_filter->taps().size();
      double delay_approx =  length / (2.0 * d_samp_rate);
      return delay_approx;
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
          gr::io_signature::make(1, 1, sizeof(float))),
          d_samp_rate(samp_rate)
    {
      d_fft_filter = gr::filter::fft_filter_fff::make(decimation, fir_taps);
      connect(self(), 0, d_fft_filter, 0);
      connect(d_fft_filter, 0, self(), 0);
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
      d_fft_filter->set_taps(fir_taps);
    }

    double
    block_custom_filter_fir_fft::get_delay_approximation()
    {
      double length = 1.0 * d_fft_filter->taps().size();
      return length / (2.0 * d_samp_rate);
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
          gr::io_signature::make(1, 1, sizeof(float))),
          d_up_freq(up_freq)
    {
      d_iir_filter1 = gr::filter::single_pole_iir_filter_ff::make(calculate_alpha(samp_rate, d_up_freq), 1);
      d_iir_filter2 = gr::filter::single_pole_iir_filter_ff::make(calculate_alpha(samp_rate, d_up_freq), 1);
      d_keep_one = gr::blocks::keep_one_in_n::make(sizeof(float), decimation);
      connect(self(), 0, d_iir_filter1, 0);
      connect(d_iir_filter1, 0, d_iir_filter2, 0);
      connect(d_iir_filter2, 0, d_keep_one, 0);
      connect(d_keep_one, 0, self(), 0);
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
      d_up_freq = up_freq;
      d_iir_filter1->set_taps(calculate_alpha(samp_rate, d_up_freq));
      d_iir_filter2->set_taps(calculate_alpha(samp_rate, d_up_freq));
    }

    double
    block_custom_filter_iir_lp::get_delay_approximation()
    {
      double delay_approx =  1.0 / (2.0 * d_up_freq);
      return delay_approx;
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
          gr::io_signature::make(1, 1, sizeof(float))),
          d_low_freq(low_freq),
          d_samp_rate(samp_rate)
    {
      double alphaHp = calc_alphaHP(samp_rate, d_low_freq);
      std::vector<double> fw_user_taps_double;
      std::vector<double> fb_user_taps_double;
      fw_user_taps_double.push_back(0.0);
      fw_user_taps_double.push_back(alphaHp);
      fw_user_taps_double.push_back(-alphaHp);
      fb_user_taps_double.push_back(1.0);
      fb_user_taps_double.push_back(alphaHp);

      d_iir_filter = gr::filter::iir_filter_ffd::make(fw_user_taps_double, fb_user_taps_double, true);
      d_keep_one = gr::blocks::keep_one_in_n::make(sizeof(float), decimation);
      connect(self(), 0, d_iir_filter, 0);
      connect(d_iir_filter, 0, d_keep_one, 0);
      connect(d_keep_one, 0, self(), 0);
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
      d_samp_rate = samp_rate;
      d_low_freq = low_freq;
      double alphaHp = calc_alphaHP(samp_rate, low_freq);
      std::vector<double> fw_user_taps_double;
      std::vector<double> fb_user_taps_double;
      fw_user_taps_double.push_back(0.0);
      fw_user_taps_double.push_back(alphaHp);
      fw_user_taps_double.push_back(-alphaHp);
      fb_user_taps_double.push_back(1.0);
      fb_user_taps_double.push_back(alphaHp);
      d_iir_filter->set_taps(fw_user_taps_double, fb_user_taps_double);
    }

    double
    block_custom_filter_iir_hp::get_delay_approximation()
    {
      double delay_approx = 1.0 / (2.0 * ((d_samp_rate / 2.0) - d_low_freq ) );
      return delay_approx;
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
          gr::io_signature::make(1, 1, sizeof(float))),
        d_low_freq(low_freq),
        d_samp_rate(samp_rate)
    {
      d_iir_filter = gr::filter::iir_filter_ffd::make(fw_user_taps, fb_user_taps, false);
      d_keep_one = gr::blocks::keep_one_in_n::make(sizeof(float), decimation);
      connect(self(), 0, d_iir_filter, 0);
      connect(d_iir_filter, 0, d_keep_one, 0);
      connect(d_keep_one, 0, self(), 0);
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
      d_iir_filter->set_taps(fw_user_taps, fb_user_taps);
    }

    double
    block_custom_filter_iir_custom::get_delay_approximation()
    {
      return 0.0;
    }

  } /* namespace digitizers */
} /* namespace gr */

