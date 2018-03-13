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

#ifndef INCLUDED_DIGITIZERS_BLOCK_CUSTOM_FILTER_IMPL_H
#define INCLUDED_DIGITIZERS_BLOCK_CUSTOM_FILTER_IMPL_H

#include <digitizers/block_custom_filter.h>
#include <boost/shared_ptr.hpp>
#include <gnuradio/blocks/keep_one_in_n.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/single_pole_iir_filter_ff.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/filter/fir_filter.h>
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
    boost::shared_ptr<gr::filter::fir_filter_fff> fir_filter;
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
    boost::shared_ptr<gr::filter::fir_filter_fff> fir_filter;
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
      boost::shared_ptr<gr::filter::fft_filter_fff> fft_filter;
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
    boost::shared_ptr<gr::filter::single_pole_iir_filter_ff> iir_filter1, iir_filter2;
    boost::shared_ptr<gr::blocks::keep_one_in_n> keep_one;

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
    boost::shared_ptr<gr::filter::iir_filter_ffd> iir_filter;
    boost::shared_ptr<gr::blocks::keep_one_in_n> keep_one;
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
    boost::shared_ptr<gr::filter::iir_filter_ffd> iir_filter;
    boost::shared_ptr<gr::blocks::keep_one_in_n> keep_one;
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
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_CUSTOM_FILTER_IMPL_H */

