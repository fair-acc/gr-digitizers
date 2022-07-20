/* -*- c++ -*- */
/*
 * Copyright 2022 fair.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "opencmw_sink_impl.h"
#include <gnuradio/io_signature.h>
#include <majordomo/base64pp.hpp>
#include <majordomo/Broker.hpp>
#include <majordomo/RestBackend.hpp>
#include <majordomo/Worker.hpp>

#include <thread>

namespace gr {
namespace pulsed_power_daq {

#pragma message("set the following appropriately and remove this warning")
using input_type = float;
opencmw_sink::sptr opencmw_sink::make(int nworkers)
{
    return gnuradio::make_block_sptr<opencmw_sink_impl>(nworkers);
}


/*
 * The private constructor
 */
opencmw_sink_impl::opencmw_sink_impl(int nworkers)
    : gr::sync_block("opencmw_sink",
                     gr::io_signature::make(
                         1 /* min inputs */, 1 /* max inputs */, sizeof(input_type)),
                     gr::io_signature::make(0, 0, 0))
{
    _nworkers = nworkers;
}

/*
 * Our virtual destructor.
 */
opencmw_sink_impl::~opencmw_sink_impl() {}

int opencmw_sink_impl::work(int noutput_items,
                            gr_vector_const_void_star& input_items,
                            gr_vector_void_star& output_items)
{
    auto in = static_cast<const input_type*>(input_items[0]);

#pragma message("Implement the signal processing in your block and remove this warning")
    // Do <+signal processing+>

    // Tell runtime system how many output items we produced.
    return noutput_items;
}

} /* namespace pulsed_power_daq */
} /* namespace gr */
