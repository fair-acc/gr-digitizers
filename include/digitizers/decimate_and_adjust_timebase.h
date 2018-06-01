/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_DECIMATE_AND_ADJUST_TIMEBASE_H
#define INCLUDED_DIGITIZERS_DECIMATE_AND_ADJUST_TIMEBASE_H

#include <digitizers/api.h>
#include <gnuradio/sync_decimator.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief Decimates the samples (keep 1 in N) and fixes tags.
     *
     * Tags are attached to the correspondent decimated sample, timebase and acq_info tags
     * are fixed with the decimation factor and user provided delay. If more than one acq_info
     * tags falls into the same output offset then the tags get merged.
     *
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API decimate_and_adjust_timebase : virtual public gr::sync_decimator
    {
     public:
      typedef boost::shared_ptr<decimate_and_adjust_timebase> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::decimate_and_adjust_timebase.
       *
       * \param decimation The decimation factor.
       * \param delay The acq info tag delay addition.
       */
      static sptr make(int decimation, double delay);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_DECIMATE_AND_ADJUST_TIMEBASE_H */

