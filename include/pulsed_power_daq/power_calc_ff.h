/* -*- c++ -*- */
/*
 * Copyright 2022 fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PULSED_POWER_DAQ_POWER_CALC_FF_H
#define INCLUDED_PULSED_POWER_DAQ_POWER_CALC_FF_H

#include <gnuradio/sync_block.h>
#include <pulsed_power_daq/api.h>

namespace gr {
namespace pulsed_power_daq {

/*!
 * \brief <+description of block+>
 * \ingroup pulsed_power_daq
 *
 */
class PULSED_POWER_DAQ_API power_calc_ff : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<power_calc_ff> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of pulsed_power_daq::power_calc_ff.
     *
     * To avoid accidental use of raw pointers, pulsed_power_daq::power_calc_ff's
     * constructor is in a private implementation
     * class. pulsed_power_daq::power_calc_ff::make is the public interface for
     * creating new instances.
     */
    static sptr make(double alpha = 0.0000001);
      virtual void set_alpha(double alpha) = 0;
};

} // namespace pulsed_power_daq
} // namespace gr

#endif /* INCLUDED_PULSED_POWER_DAQ_POWER_CALC_FF_H */