/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_AGGREGATION_HELPER_H
#define INCLUDED_DIGITIZERS_AGGREGATION_HELPER_H

#include <digitizers/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace digitizers {

    /*!
     * A subgroup of the aggregation circuit as specified in the design proposal, implemented
     * as simple function calls instead of connecting blocks.
     *
     * what it does:
     * out = decimate(sqrt(abs(a - b^2) + (sigma_mult * c^2)), decim);
     * \ingroup digitizers
     */
    class DIGITIZERS_API aggregation_helper : virtual public gr::block
    {
     public:
      typedef std::shared_ptr<aggregation_helper> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of digitizers::aggregation_helper.
       *
       * \param decim The decimation factor
       * \param sigma_mult The sigma multiplier in the function specified in the blocks description
       */
      static sptr make(int decim, float sigma_mult);
      /*!
       * \brief Updates the design in the middle of execution.
       *
       * \param sigma_mult The sigma multiplier in the function specified in the blocks description
       */
      virtual void update_design(float sigma_mult) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_AGGREGATION_HELPER_H */

