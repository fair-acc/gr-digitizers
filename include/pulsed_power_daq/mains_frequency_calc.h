/* -*- c++ -*- */
/*
 * Copyright 2022 fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PULSED_POWER_DAQ_MAINS_FREQUENCY_CALC_H
#define INCLUDED_PULSED_POWER_DAQ_MAINS_FREQUENCY_CALC_H

#include <gnuradio/sync_block.h>
#include <pulsed_power_daq/api.h>

namespace gr {
namespace pulsed_power_daq {

/*!
 * \brief <+description of block+>
 * \ingroup pulsed_power_daq
 *
 */
class PULSED_POWER_DAQ_API mains_frequency_calc : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<mains_frequency_calc> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of
     * pulsed_power_daq::mains_frequency_calc.
     *
     * To avoid accidental use of raw pointers, pulsed_power_daq::mains_frequency_calc's
     * constructor is in a private implementation
     * class. pulsed_power_daq::mains_frequency_calc::make is the public interface for
     * creating new instances.
     */
    static sptr make(float expected_sample_rate, float low_threshold, float high_threshold);
};

} // namespace pulsed_power_daq
} // namespace gr

#endif /* INCLUDED_PULSED_POWER_DAQ_MAINS_FREQUENCY_CALC_H */
