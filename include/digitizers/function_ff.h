/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_FUNCTION_FF_H
#define INCLUDED_DIGITIZERS_FUNCTION_FF_H

#include <digitizers/api.h>
#include <gnuradio/sync_decimator.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Generates reference signal alongside the min a max signals.
     * \ingroup digitizers
     *
     * The only purpose of the 'timing' input port is to provided precise timing information to this
     * block. This is done by scanning for acq_info tags, holding three pieces of information required
     * for the correct signal generation:
     *  - timestamp
     *  - last beam-in timestamp
     *  - timebase
     *
     * The three functions (reference, min and max) are specified by using four separate vectors. One
     * vector is used to provide relative timing information w.r.t. BEAM_IN event and other three to
     * specify function value for a given time point. Values are calculated by using a direct
     * linear interpolation.
     *
     * Note the time points of the functions should be monotonically increasing.
     */
    class DIGITIZERS_API function_ff : virtual public gr::sync_decimator
    {
     public:
      typedef boost::shared_ptr<function_ff> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::function_ff.
       *
       * To avoid accidental use of raw pointers, digitizers::function_ff's
       * constructor is in a private implementation
       * class. digitizers::function_ff::make is the public interface for
       * creating new instances.
       */
      static sptr make(int decimation);

      /*!
       * \brief Sets reference, min and max functions.
       */
      virtual void set_function(const std::vector<float> &timing,
              const std::vector<float> &ref,
              const std::vector<float> &min,
              const std::vector<float> &max) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_FUNCTION_FF_H */

