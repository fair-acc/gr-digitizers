/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_BLOCK_COMPLEX_TO_MAG_DEG_H
#define INCLUDED_DIGITIZERS_BLOCK_COMPLEX_TO_MAG_DEG_H

#include <digitizers/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief transforms carthesian values of the complex numbers to polar magnitude and phase
     * vectors, where phase is coded in degrees.
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API block_complex_to_mag_deg : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<block_complex_to_mag_deg> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::block_complex_to_mag_deg.
       *
       * To avoid accidental use of raw pointers, digitizers::block_complex_to_mag_deg's
       * constructor is in a private implementation
       * class. digitizers::block_complex_to_mag_deg::make is the public interface for
       * creating new instances.
       *
       * \param vec_len Length of input and output vectors.
       */
      static sptr make(int vec_len);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_BLOCK_COMPLEX_TO_MAG_DEG_H */

