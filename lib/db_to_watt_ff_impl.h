/* -*- c++ -*- */
/*
 * Copyright 2021 fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DIGITIZERS_39_DB_TO_WATT_FF_IMPL_H
#define INCLUDED_DIGITIZERS_39_DB_TO_WATT_FF_IMPL_H

#include <digitizers_39/db_to_watt_ff.h>

namespace gr {
  namespace digitizers_39 {

    class db_to_watt_ff_impl : public db_to_watt_ff
    {
     private:
      // Nothing to declare in this block.

     public:
      db_to_watt_ff_impl();
      ~db_to_watt_ff_impl();

      // Where all the action really happens
      int work(
              int noutput_items,
              gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items
      );
    };

  } // namespace digitizers_39
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_39_DB_TO_WATT_FF_IMPL_H */

