/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <gnuradio/math.h>
#include "stft_goertzl_dynamic_impl.h"
#include "digitizers/tags.h"

#include <gnuradio/fft/window.h>

namespace gr {
  namespace digitizers {

    stft_goertzl_dynamic::sptr
    stft_goertzl_dynamic::make(double samp_rate, int winsize, int nbins)
    {
      std::vector<int> signature;
      signature.push_back(sizeof(float) * winsize);
      signature.push_back(sizeof(float));
      signature.push_back(sizeof(float));
      return gnuradio::get_initial_sptr
        (new stft_goertzl_dynamic_impl(samp_rate, winsize, nbins, signature));
    }

    stft_goertzl_dynamic_impl::stft_goertzl_dynamic_impl(double samp_rate, int winsize, int nbins, std::vector<int> in_sig)
      : gr::sync_block("stft_goertzl_dynamic",
              gr::io_signature::makev(3, 3, in_sig),
              gr::io_signature::make(3, 3, sizeof(float) * nbins)),
              d_samp_length(1.0/samp_rate),
              d_winsize(winsize),
              d_nbins(nbins),
              d_window_function(fft::window::build(fft::window::win_type::WIN_HANN, winsize, 1.0))
    {
      set_tag_propagation_policy(TPP_DONT);
    }

    stft_goertzl_dynamic_impl::~stft_goertzl_dynamic_impl()
    {
    }

    
    void stft_goertzl_dynamic_impl::goertzel(const float* data, const long data_len, float Ts, float frequency, int filter_size, float &real, float &imag)
    {
        // https://github.com/NaleRaphael/goertzel-ffrequency/blob/master/src/dsp.c
        float k; // Related to frequency bins
        float omega;
        float sine, cosine, coeff, sf;
        float q0, q1, q2;
        long int i;
        
        k = (0.5f + ((float)(filter_size*frequency) * Ts));    
        
        omega = 2.0f*M_PI*k/(float)filter_size;
        sine = sin(omega);
        cosine = cos(omega);
        coeff = 2.0f*cosine;
        sf = (float)data_len/2.0f;		// scale factor: for normalization
        
        q0 = 0.0f;
        q1 = 0.0f;
        q2 = 0.0f;
        
        long int dlen = data_len - data_len%3;
        for (i = 0; i < dlen; i+=3) {
            q0 = coeff*q1 - q2 + data[i]*d_window_function[i];
            q2 = coeff*q0 - q1 + data[i+1]*d_window_function[i+1];
            q1 = coeff*q2 - q0 + data[i+2]*d_window_function[i+2];
        }
        
        for (; i < data_len; i++) {
            q0 = coeff*q1 - q2 + data[i];
            q2 = q1;
            q1 = q0;
        }
        
        real = (q1 - q2*cosine)/sf;
        imag = (q2*sine)/sf;	
    }
    
    
    void stft_goertzl_dynamic_impl::dft(const float* data, const long data_len, float Ts, float frequency, float& real, float& imag) {
        // legacy implementation - mathematically most correct, but numerically expensive due to sine and cosine computations        
        real = 0.0;
        imag = 0.0;
        float omega0 = 2.0 * M_PI * frequency * Ts;
        for (int i = 0; i < data_len; i++) {
            float in = data[i] * d_window_function[i];
            real += cos(omega0 * i) * in;
            imag += sin(omega0 * i) * in;
        }
        real /= 0.5*data_len;
        imag /= 0.5*data_len;    
    }


    int
    stft_goertzl_dynamic_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        const float *in = (const float *) input_items[0];
        const float *f_min = (const float *) input_items[1];
        const float *f_max = (const float *) input_items[2];
        float *mag = (float *) output_items[0];
        float *phs = (float *) output_items[1];
        float *fqs = (float *) output_items[2];
        
        double f_range = f_max[0] - f_min[0];
        //printf("noutput_items = %i, d_winsize =%i\n", noutput_items, d_winsize);
        
        //do frequency analysis for each bin
        for(int i = 0; i < d_nbins; i++){
            double bin_f_range_factor = static_cast<double>(i) / static_cast<double>(d_nbins-1);
            double freq = f_min[0] + (bin_f_range_factor * f_range);
            
            // Goertzel vs DFT
            if (1) {
                float w = 2.0 * M_PI * freq * d_samp_length;
                double wr = 2.0 * std::cos(w);
                double wi = std::sin(w);
                double d1 = 0.0; //resets for each iteration
                double d2 = 0.0; //resets for each iteration
                
                //goertzel magic
                for(int j = 0; j < d_winsize; j++) {
                    //double y = in[j] + wr * d1 - d2;
                    double y = in[j]*d_window_function[j] + wr * d1 - d2;
                    d2 = d1;
                    d1 = y;
                }
                double re = (0.5*wr*d1-d2)/d_winsize;
                double im = (wi*d1)/d_winsize;
                
                //transform from carthesian to polar components
                mag[i] = std::sqrt((re * re) + (im * im)); // this usually should be hypotf(re,im) /over-/under-flow  protected/performance
                mag[i] = std::hypotf(re,im); // faster and over-/under-flow  protected
                phs[i] = gr::fast_atan2f(re, im);
                phs[i] = 0.0;
            } else {
                float re;
                float im;
                if (1) {
                    goertzel(in, d_winsize, d_samp_length, freq, d_winsize, re, im);
                } else {
                    dft(in, d_winsize, d_samp_length, freq, re, im);
                }
                mag[i] = std::hypotf(re,im); // faster and over-/under-flow  protected
                phs[i] = gr::fast_atan2f(re, im);
            }
            
            
            //post frequency of each bin
            fqs[i] = freq;
        }

      std::vector<tag_t> tags;
      get_tags_in_range(tags, 0, nitems_read(0), nitems_read(0)+1);
      for(auto tag : tags) {
        tag.offset = nitems_written(0);
        add_item_tag(0, tag);
      }
      return 1;
    }

    //update interface

    void
    stft_goertzl_dynamic_impl::set_samp_rate(double samp_rate)
    {
      d_samp_length = 1.0/samp_rate;
    }
  } /* namespace digitizers */
} /* namespace gr */

