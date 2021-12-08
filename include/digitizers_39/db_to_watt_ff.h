/* -*- c++ -*- */
/*
 * Copyright 2021 fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DIGITIZERS_39_DB_TO_WATT_FF_H
#define INCLUDED_DIGITIZERS_39_DB_TO_WATT_FF_H

#include <digitizers_39/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers_39 {

    /*!
     * \brief <+description of block+>
     * \ingroup digitizers_39
     *
     */
    class DIGITIZERS_39_API db_to_watt_ff : virtual public gr::sync_block
    {
     public:
      typedef std::shared_ptr<db_to_watt_ff> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers_39::db_to_watt_ff.
       *
       * To avoid accidental use of raw pointers, digitizers_39::db_to_watt_ff's
       * constructor is in a private implementation
       * class. digitizers_39::db_to_watt_ff::make is the public interface for
       * creating new instances.
       */
      static sptr make();
    };

  } // namespace digitizers_39
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_39_DB_TO_WATT_FF_H */

