/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_BLOCK_CUSTOM_FILTER_IMPL_H
#define INCLUDED_DIGITIZERS_BLOCK_CUSTOM_FILTER_IMPL_H

#include <digitizers/block_custom_filter.h>
#include <boost/shared_ptr.hpp>
#include <gnuradio/blocks/keep_one_in_n.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/single_pole_iir_filter_ff.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/filter/fft_filter.h>
#include <gnuradio/filter/iir_filter_ffd.h>

namespace gr {
  namespace digitizers {

  /**
   * \brief Low pass frequency filtering implementation.
   *
   * Has all settings to be compatible with all the other path implementations,
   * yet the only parameters that affect it are up_freq and tr_width.
   *
   * Filter diagram:
   *   ----------\____
   *
   * \param up_freq Upper cutoff frequency
   * \param tr_width filter cutoff transition range
   */
  class block_custom_filter_fir_lp : public block_custom_filter
  {
   private:
    // Nothing to declare in this block.
    gr::filter::fir_filter_fff::sptr d_fir_filter;
    double d_samp_rate;
   public:
    block_custom_filter_fir_lp(int decimation,
        const std::vector<float> &fir_taps,
        double low_freq,
        double up_freq,
        double tr_width,
        const std::vector<double> &fb_user_taps,
        const std::vector<double> &fw_user_taps,
        double samp_rate);

    ~block_custom_filter_fir_lp();

   protected:
    void update_design(
        const std::vector<float> &fir_taps,
        double low_freq,
        double up_freq,
        double tr_width,
        const std::vector<double> &fb_user_taps,
        const std::vector<double> &fw_user_taps,
        double samp_rate);

    double get_delay_approximation();
  };

  /**
   * \brief Band pass filtering implementation.
   *
   * Has all settings to be compatible with all the other path implementations,
   * yet the only parameters that affect it are low_freq, up_freq and tr_width.
   *
   * Filter diagram:
   *   ____/-----\____
   *
   * \param up_freq Upper cutoff frequency
   * \param tr_width filter cutoff transition range
   * \param low_freq Lower cutoff frequency
   */
  class block_custom_filter_fir_bp : public block_custom_filter
  {
   private:
    // Nothing to declare in this block.
    gr::filter::fir_filter_fff::sptr d_fir_filter;
    double d_samp_rate;
   public:
    block_custom_filter_fir_bp(int decimation,
        const std::vector<float> &fir_taps,
        double low_freq,
        double up_freq,
        double tr_width,
        const std::vector<double> &fb_user_taps,
        const std::vector<double> &fw_user_taps,
        double samp_rate);

    ~block_custom_filter_fir_bp();

   protected:
    void update_design(
        const std::vector<float> &fir_taps,
        double low_freq,
        double up_freq,
        double tr_width,
        const std::vector<double> &fb_user_taps,
        const std::vector<double> &fw_user_taps,
        double samp_rate);

    double get_delay_approximation();
  };

  /**
   * \brief Custom band filtering implementation.
   *
   * Has all settings to be compatible with all the other path implementations,
   * yet the only parameter that affects it is fb_user_taps.
   *
   * Filter diagram(User defined):
   *   _/\__/--|_/-\____
   *
   * \param up_freq Upper cutoff frequency
   * \param tr_width filter cutoff transition range
   * \param low_freq Lower cutoff frequency
   */
  class block_custom_filter_fir_custom : public block_custom_filter
  {
   private:
    // Nothing to declare in this block.
    gr::filter::fir_filter_fff::sptr d_fir_filter;
    double d_samp_rate;
   public:
    block_custom_filter_fir_custom(int decimation,
        const std::vector<float> &fir_taps,
        double low_freq,
        double up_freq,
        double tr_width,
        const std::vector<double> &fb_user_taps,
        const std::vector<double> &fw_user_taps,
        double samp_rate);

    ~block_custom_filter_fir_custom();

   protected:
    void update_design(
        const std::vector<float> &fir_taps,
        double low_freq,
        double up_freq,
        double tr_width,
        const std::vector<double> &fb_user_taps,
        const std::vector<double> &fw_user_taps,
        double samp_rate);

    double get_delay_approximation();
  };

  /**
   * \brief Finite impulse repsonse FFT filtering implementation.
   *
   * Has all settings to be compatible with all the other path implementations,
   * yet the only parameter that affects it is fir_taps.
   *
   * Filter diagram(User defined):
   *   _/\__/--|_/-\____
   *
   * \param fir_taps FIR-filter user defined taps
   */
  class block_custom_filter_fir_fft : public block_custom_filter
    {
     private:
      // Nothing to declare in this block.
      gr::filter::fft_filter_fff::sptr d_fft_filter;
      double d_samp_rate;
     public:
      block_custom_filter_fir_fft(int decimation,
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate);

      ~block_custom_filter_fir_fft();

     protected:
      void update_design(
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate);

      double get_delay_approximation();
    };

  /**
   * \brief Infinite impulse repsonse low pass filtering implementation.
   *
   * Has all settings to be compatible with all the other path implementations,
   * yet the only parameters that affect it are samp_rate, up_freq and decimation.
   *
   * Filter diagram(User defined):
   *   _/\__/--|_/-\____
   *
   * \param up_freq Upper cutoff frequency
   * \param samp_rate Sampling rate
   * \param decimation Decimating factor
   */
  class block_custom_filter_iir_lp : public block_custom_filter
  {
   private:
    // Nothing to declare in this block.
    std::shared_ptr<gr::filter::single_pole_iir_filter_ff> d_iir_filter1, d_iir_filter2;
    std::shared_ptr<gr::blocks::keep_one_in_n> d_keep_one;
    double d_samp_rate;
    double d_up_freq;

    double calculate_alpha(double samp_rate, double upper_frequency);
     public:
      block_custom_filter_iir_lp(int decimation,
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate);

      ~block_custom_filter_iir_lp();

     protected:
      void update_design(
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate);

      double get_delay_approximation();
  };

  /**
   * \brief Infinite impulse repsonse high pass filtering implementation.
   *
   * Has all settings to be compatible with all the other path implementations,
   * yet the only parameters that affect it are samp_rate and up_freq.
   *
   * Filter diagram(User defined):
   *   _______|------
   *
   * \param low_freq Upper cutoff frequency
   * \param samp_rate Sampling rate
   */
  class block_custom_filter_iir_hp : public block_custom_filter
    {
     private:
      // Nothing to declare in this block.
      std::shared_ptr<gr::filter::iir_filter_ffd> d_iir_filter;
      std::shared_ptr<gr::blocks::keep_one_in_n> d_keep_one;
      double d_low_freq;
      double d_samp_rate;

      double calc_alphaHP(double sample_rate, double freq_min);
     public:
      block_custom_filter_iir_hp(int decimation,
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate);

      ~block_custom_filter_iir_hp();

     protected:
      void update_design(
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate);

      double get_delay_approximation();
    };

  /**
     * \brief Infinite impulse repsonse custom filtering implementation.
     *
     * Has all settings to be compatible with all the other path implementations,
     * yet the only parameters that affect it are fw_user_taps, fb_user_taps and decimation.
     *
     * Filter diagram(User defined):
     *   _/\__/--|_/-\____
     *
     * \param fw_user_taps Feed forward user defined taps
     * \param fb_user_taps Feed backward user defined taps
     * \param decimation Decimating factor
     */
  class block_custom_filter_iir_custom : public block_custom_filter
    {
     private:
      // Nothing to declare in this block.
    std::shared_ptr<gr::filter::iir_filter_ffd> d_iir_filter;
    std::shared_ptr<gr::blocks::keep_one_in_n> d_keep_one;
    double d_low_freq;
    double d_samp_rate;
     public:
      block_custom_filter_iir_custom(int decimation,
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate);

      ~block_custom_filter_iir_custom();

     protected:
      void update_design(
          const std::vector<float> &fir_taps,
          double low_freq,
          double up_freq,
          double tr_width,
          const std::vector<double> &fb_user_taps,
          const std::vector<double> &fw_user_taps,
          double samp_rate);

      double get_delay_approximation();
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_CUSTOM_FILTER_IMPL_H */

