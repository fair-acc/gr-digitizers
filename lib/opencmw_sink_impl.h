/* -*- c++ -*- */
/*
 * Copyright 2022 fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PULSED_POWER_DAQ_OPENCMW_SINK_IMPL_H
#define INCLUDED_PULSED_POWER_DAQ_OPENCMW_SINK_IMPL_H

#include <pulsed_power_daq/opencmw_sink.h>

namespace gr {
namespace pulsed_power_daq {

class opencmw_sink_impl : public opencmw_sink
{
private:
    // Nothing to declare in this block.
    int _nworkers;

public:
    opencmw_sink_impl(int nworkers);
    ~opencmw_sink_impl();

    // Where all the action really happens
    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);
};

} // namespace pulsed_power_daq
} // namespace gr

#endif /* INCLUDED_PULSED_POWER_DAQ_OPENCMW_SINK_IMPL_H */
