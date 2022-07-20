/* -*- c++ -*- */
/*
 * Copyright 2022 fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PULSED_POWER_DAQ_OPENCMW_SINK_H
#define INCLUDED_PULSED_POWER_DAQ_OPENCMW_SINK_H

#include <gnuradio/sync_block.h>
#include <pulsed_power_daq/api.h>

namespace gr {
namespace pulsed_power_daq {

/*!
 * \brief <+description of block+>
 * \ingroup pulsed_power_daq
 *
 */
class PULSED_POWER_DAQ_API opencmw_sink : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<opencmw_sink> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of pulsed_power_daq::opencmw_sink.
     *
     * To avoid accidental use of raw pointers, pulsed_power_daq::opencmw_sink's
     * constructor is in a private implementation
     * class. pulsed_power_daq::opencmw_sink::make is the public interface for
     * creating new instances.
     */
    static sptr make(int nworkers = 1);
};

} // namespace pulsed_power_daq
} // namespace gr

#endif /* INCLUDED_PULSED_POWER_DAQ_OPENCMW_SINK_H */
