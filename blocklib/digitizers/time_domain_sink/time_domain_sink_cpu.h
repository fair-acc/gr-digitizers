/* -*- c++ -*- */
/*
 * Copyright 2022 Block Author
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <gnuradio/digitizers/time_domain_sink.h>

namespace gr {
namespace digitizers {

template <class T>
class time_domain_sink_cpu : public time_domain_sink<T>
{
public:
    time_domain_sink_cpu(const typename time_domain_sink<T>::block_args& args);

    work_return_t work(work_io& wio) override;

private:
    // Declare private variables here
};


} // namespace digitizers
} // namespace gr
