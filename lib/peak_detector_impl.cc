/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "peak_detector_impl.h"
#include "utils.h"

namespace gr {
  namespace digitizers {

    peak_detector::sptr
    peak_detector::make(double samp_rate, int vec_len, int proximity)
    {
      return gnuradio::get_initial_sptr
        (new peak_detector_impl(samp_rate, vec_len, proximity));
    }

    /*
     * The private constructor
     */
    peak_detector_impl::peak_detector_impl(double samp_rate,
        int vec_len,
        int proximity)
      : gr::block("peak_detector",
              gr::io_signature::makev(4, 4, std::vector<int> {
                    static_cast<int>(sizeof(float) * vec_len),
                    static_cast<int>(sizeof(float) * vec_len),
                    static_cast<int>(sizeof(float)),
                    static_cast<int>(sizeof(float))
              }),
              gr::io_signature::make(2, 2, sizeof(float))),
        d_vec_len(vec_len),
        d_prox(proximity),
        d_freq(samp_rate)
    {
    }

    /*
     * Our virtual destructor.
     */
    peak_detector_impl::~peak_detector_impl()
    {
    }

    void
    peak_detector_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      //ninput_items_required[0] = 1;
       ninput_items_required[0] = noutput_items;
    }
    
    /**
     * 
     * interpolation using a Gaussian interpolation
     * 
     * @param data, data array
     * @param index, 0< index < data.length
     * @return location of the to be interpolated peak [bins] 
     */
    float interpolateGaussian(const float* data, const int data_length, const int index) {
        if ((index > 0) && (index < (data_length - 1))) {
            const float left = std::pow(data[index - 1], 1);
            const float center = std::pow(data[index - 0], 1);
            const float right = std::pow(data[index + 1], 1);
            
            float val = index;
            val += 0.5 * std::log(right / left) / std::log(std::pow(center, 2) / (left * right));
            return val;
        } else {
            return ((float)index);
        }
    }
    
    float linearInterpolate(float x0, float x1, float y0, float y1, float y) {
            return x0 + (y-y0) * (x1-x0)/(y1-y0);
    }
    
    /**
     * 
     * compute simple Full-Width-Half-Maximum (no inter-bin interpolation)
     * 
     * @param data, data array
     * @param index, 0< index < data.length
     * @return FWHM estimate [bins]
     */
    float computeFWHM(const float* data, const int data_length, const int index) {        
        if ((index > 0) && (index < (data_length - 1))) {
            float maxHalf = 0.5*data[index];            
            int lowerLimit;
            int upperLimit;
            for (upperLimit = index; upperLimit < data_length && data[upperLimit]>maxHalf; upperLimit++);            
            for (lowerLimit = index; lowerLimit > 0 && data[lowerLimit]>maxHalf; lowerLimit--);
            
            return (upperLimit-lowerLimit);
        } else {
            return 1.0f;
        }
    }
    
    
    /**
     * 
     * compute interpolated Full-Width-Half-Maximum
     * 
     * @param data, data array
     * @param index, 0< index < data.length
     * @return FWHM estimate [bins]
     */
    float computeInterpolatedFWHM(const float* data, const int data_length, const int index) {        
        if ((index > 0) && (index < (data_length - 1))) {
            float maxHalf = 0.5*data[index];            
            int lowerLimit;
            int upperLimit;
            for (upperLimit = index; upperLimit < data_length && data[upperLimit]>maxHalf; upperLimit++);            
            for (lowerLimit = index; lowerLimit > 0 && data[lowerLimit]>maxHalf; lowerLimit--);            
                        
            float lowerLimitRefined = linearInterpolate(lowerLimit, lowerLimit+1, data[lowerLimit], data[lowerLimit+1], maxHalf);
            float upperLimitRefined = linearInterpolate(upperLimit-1, upperLimit, data[upperLimit-1], data[upperLimit], maxHalf);
            
            return (upperLimitRefined-lowerLimitRefined);
        } else {
            return 1.0f;
        }
    }
    


    int
    peak_detector_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const float *actual = (const float *) input_items[0];
      const float *filtered = (const float *) input_items[1];
      const float *low_freq =  (const float *) input_items[2];
      const float *up_freq = (const float *) input_items[3];

      float *max_sig = (float *) output_items[0];
      float *width_sig = (float *) output_items[1];
      if(ninput_items[0] <= 0 ||ninput_items[1] <= 0 || noutput_items < 1){
        return 0;
      }

      // TODO: verify bounds
      int d_start_bin = 2.0 * low_freq[0] / d_freq * d_vec_len;
      int d_end_bin = 2.0 * up_freq[0]  / d_freq * d_vec_len;

      //find filtered maximum
      float max_fil = filtered[d_start_bin];
      int max_fil_i = d_start_bin;
      for(int i = d_start_bin + 1; i <= d_end_bin; i++) {
        if(max_fil<filtered[i]) {
          max_fil = filtered[i];
          max_fil_i = i;
        }
      }
      //find actual maximum in the proximity of the averaged maximum
      //initialize to something smaller than actual max.
      float max = max_fil;
      int max_i = max_fil_i;
      for(int i = 0; i< d_prox; i++) {
        if(max_fil_i+i < d_vec_len &&
            max<actual[max_fil_i + i]){
          max = actual[max_fil_i + i];
          max_i = max_fil_i + i;
        }
        if(max_fil_i-i >= 0 &&
            max<actual[max_fil_i - i]){
          max = actual[max_fil_i - i];
          max_i = max_fil_i - i;
        }
      }

      //find FWHM for stdev approx.
      double hm=actual[max_i]/2.0;
      int whm_i= max_i;
      for(int i =0; i<d_vec_len && whm_i == max_i; i++){
        if(max_i+i < d_vec_len &&
            hm > actual[max_i+i]){ whm_i = max_i+i; }
        if(max_i-i >= 0 &&
            hm > actual[max_i-i]){ whm_i = max_i-i; }
      }
      double freq_whm = 0.0;
      if(whm_i > max_i) {
        double a = actual[whm_i]-actual[whm_i-1];
        double b = actual[whm_i-1];
        freq_whm= (b + (hm-b))/a;
      }
      if(whm_i < max_i) {
        double a = actual[whm_i+1]-actual[whm_i];
        double b = actual[whm_i];
        freq_whm= (b + (hm-b))/a;
      }
      
      // freq_whm = computeFWHM(actual, d_vec_len, max_i);
      freq_whm = computeInterpolatedFWHM(actual, d_vec_len, max_i);      

      //fix width to half maximum from bin count to frequency window
      freq_whm *= d_freq/(d_vec_len);

      
      // see CAS Reference in Common Spec:
      // 
      float maxInterpolated = interpolateGaussian(actual, d_vec_len, max_i);
      
      max_sig[0] = (maxInterpolated * d_freq) / (2.0 * d_vec_len);
      width_sig[0] = freq_whm * whm2stdev;
      consume_each (noutput_items);
      
      /*
      max_sig[0] = (max_i * d_freq) / (2.0 * d_vec_len);
      width_sig[0] = freq_whm * whm2stdev;
      consume_each (1);
      */
      // Tell runtime system how many output items we produced.
      return 1;
    }

  } /* namespace digitizers */
} /* namespace gr */

