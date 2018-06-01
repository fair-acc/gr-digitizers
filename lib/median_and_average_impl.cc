/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "median_and_average_impl.h"

namespace gr {
  namespace digitizers {

    median_and_average::sptr
    median_and_average::make(int vec_len, int n_med, int n_lp)
    {
      return gnuradio::get_initial_sptr
        (new median_and_average_impl(vec_len, n_med, n_lp));
    }

    /*
     * The private constructor
     */
    median_and_average_impl::median_and_average_impl(int vec_len, int n_med, int n_lp)
      : gr::block("median_and_average",
        gr::io_signature::make(1, 1, sizeof(float) * vec_len),
        gr::io_signature::make(1, 1, sizeof(float) * vec_len)),
        d_median(n_med), d_average(n_lp), d_vec_len(vec_len)
    {
      set_tag_propagation_policy(TPP_DONT);
    }

    /*
     * Our virtual destructor.
     */
    median_and_average_impl::~median_and_average_impl()
    {
    }

    void
    median_and_average_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = noutput_items;
    }
    
    
    
    int compare_floats(const void* a, const void* b)
    {
    float arg1 = *(const float*)a;
    float arg2 = *(const float*)b;
 
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
 
    // return (arg1 > arg2) - (arg1 < arg2); // possible shortcut
    }
    
    float median(float *buffer, int size) {
       if (size<=0) return 0;
       if (size<=1) return buffer[0];
       
       qsort(buffer, size, sizeof(float), compare_floats);   
       if (size%2==0) {
            return 0.5*(buffer[size/2] + buffer[size/2-1]);
       } else {
           return buffer[size/2];
       }       
    }
    
     
    float average(const float *buffer, int size) {
       if (size<=0) return 0;
       if (size<=1) return buffer[0];
       
       float sum = 0.0;    
       for (int i=0; i < size; i++) {
           sum += buffer[i];
       }
       
       return sum/(float)size;
    }

    int
    median_and_average_impl::general_work (int noutput_items,
      gr_vector_int &ninput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items)
    {
      const float *in = (const float *) input_items[0];
      float *out = (float *) output_items[0];

      if(ninput_items[0] <= 0 || noutput_items <= 0){
        return 0;
      }
      
      const int med_len =d_median;
      const int avg_len = d_average;
      float buffer[2*med_len+1];
      float temp_buffer[d_vec_len];
      //calculate median of samples and average it.
      for(int i = 0; i < d_vec_len; i++) {
        if (med_len == 0) {
          temp_buffer[i] = in[i];
          //out[i] = in[i];
          continue;
        }
        int count = 0;
        for (int j=0; j < (2*med_len+1); j++) {
          int k = i - med_len + j;
          if (k<0) {
            k = 0;
          }
          else if (k>=d_vec_len) {
            k = d_vec_len-1;
          }
          if (k != i) {
            buffer[count] = in[k];
            count++;
          }
        }
        temp_buffer[i] = median(buffer, count);
      }

      float buffer2[2*avg_len+1];
      for(int i = 0; i < d_vec_len; i++) {
        if (avg_len == 0) {
          out[i] = temp_buffer[i];
          continue;
        }
        int count = 0;
        for (int j=0; j < (2*avg_len+1); j++) {
          int k = i - avg_len + j;
          if (k<0) {
            k = 0;
          }
          else if (k>=d_vec_len) {
            k = d_vec_len-1;
          }
          buffer2[count] = temp_buffer[k];
          count++;
        }

        out[i] = average(buffer2, count);
      }


      consume_each (1);
      return 1;
      }

  } /* namespace digitizers */
} /* namespace gr */

