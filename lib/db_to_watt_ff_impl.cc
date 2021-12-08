/* -*- c++ -*- */
/*
 * Copyright 2021 fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/io_signature.h>
#include "db_to_watt_ff_impl.h"

namespace gr {
  namespace digitizers_39 {

    db_to_watt_ff::sptr
    db_to_watt_ff::make()
    {
      return gnuradio::make_block_sptr<db_to_watt_ff_impl>(
        );
    }

    /*
     * The private constructor
     */
    db_to_watt_ff_impl::db_to_watt_ff_impl()
      : gr::sync_block("db_to_watt_ff",
              gr::io_signature::make(1 /* min inputs */, 1 /* max inputs */, sizeof(float)),
              gr::io_signature::make(1 /* min outputs */, 1 /*max outputs */, sizeof(float)))
    {}

    /*
     * Our virtual destructor.
     */
    db_to_watt_ff_impl::~db_to_watt_ff_impl()
    {
    }

    int
    db_to_watt_ff_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float* in = (const float*)input_items[0];
      float* out = (float*)output_items[0];

      float scale = 10.0;
      float scaled = 0.0;

      for (int i = 0; i < noutput_items; i++) {
        std::cout << "---------------" << "\n";
        std::cout << "Parameter: " << in[i] << "\n";
        std::cout << "---------------" << "\n";
        float exponent = in[i] / 100.0;
        float tmp = std::ceil(exponent) * 10;
        std::cout << "Exponent: " << exponent << "\n";
        float power = powf(10, exponent);
        std::cout << "Power: " << power << "\n";
        float value = power * ( 1 / tmp );
        std::cout << "Result: " << value << "\n";
        out[i] = (float)value;
      }

      return noutput_items;
    }

  } /* namespace digitizers_39 */
} /* namespace gr */

