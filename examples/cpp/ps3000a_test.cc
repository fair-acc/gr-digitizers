/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#include <digitizers/picoscope_3000a.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/null_sink.h>

#include <iostream>
#include <vector>

using namespace gr::digitizers;
using namespace gr::blocks;

void wire_rapid_block()
{
    auto top = gr::make_top_block("channels");

    auto ps = picoscope_3000a::make("", true);

    ps->set_aichan("A", true, 5.0, AC_1M);
    ps->set_aichan("B", true, 5.0, AC_1M);
    ps->set_aichan("C", true, 5.0, AC_1M);
    ps->set_aichan("D", true, 5.0, AC_1M);
    ps->set_diport("port0", true, 3.0);
    ps->set_diport("port1", true, 3.0);

    double samp_rate = 10000000.0;
    ps->set_samp_rate(samp_rate);
    ps->set_samples(500000, 10000);
    ps->set_rapid_block(10);

    auto sinkA = null_sink::make(sizeof(float));
    auto sinkB = null_sink::make(sizeof(float));
    auto sinkC = null_sink::make(sizeof(float));
    auto sinkD = null_sink::make(sizeof(float));
    auto errsinkA = null_sink::make(sizeof(float));
    auto errsinkB = null_sink::make(sizeof(float));
    auto errsinkC = null_sink::make(sizeof(float));
    auto errsinkD = null_sink::make(sizeof(float));

    auto sink0 = null_sink::make(sizeof(uint8_t));
    auto sink1 = null_sink::make(sizeof(uint8_t));

    // connect and run
    top->connect(ps, 0, sinkA, 0); top->connect(ps, 1, errsinkA, 0);
    top->connect(ps, 2, sinkB, 0); top->connect(ps, 3, errsinkB, 0);
    top->connect(ps, 4, sinkC, 0); top->connect(ps, 5, errsinkC, 0);
    top->connect(ps, 6, sinkD, 0); top->connect(ps, 7, errsinkD, 0);

    top->connect(ps, 8, sink0, 0);
    top->connect(ps, 9, sink1, 0);

    top->start();

    sleep(30);

    top->stop();
    top->wait();
}

void wire_streaming()
{
    auto top = gr::make_top_block("channels");

    auto ps = picoscope_3000a::make("", true);

    ps->set_aichan("A", true, 5.0, AC_1M);
    ps->set_aichan("B", true, 5.0, AC_1M);
    ps->set_aichan("C", true, 5.0, AC_1M);
    ps->set_aichan("D", true, 5.0, AC_1M);
    ps->set_diport("port0", true, 1.5);
    ps->set_diport("port1", true, 1.5);

    //ps->set_aichan_trigger("A", TRIGGER_DIRECTION_RISING, 1.0);

    double samp_rate = 10000000.0;
    ps->set_samp_rate(samp_rate);
    ps->set_samples(500000, 10000);
    ps->set_buffer_size(8192);
    ps->set_nr_buffers(64);
    ps->set_driver_buffer_size(200000);
    ps->set_streaming(0.0005);
    //ps->set_downsampling(DOWNSAMPLING_MODE_DECIMATE, 16);

    auto sinkA = null_sink::make(sizeof(float));
    auto sinkB = null_sink::make(sizeof(float));
    auto sinkC = null_sink::make(sizeof(float));
    auto sinkD = null_sink::make(sizeof(float));
    auto errsinkA = null_sink::make(sizeof(float));
    auto errsinkB = null_sink::make(sizeof(float));
    auto errsinkC = null_sink::make(sizeof(float));
    auto errsinkD = null_sink::make(sizeof(float));

    auto sink0 = null_sink::make(sizeof(uint8_t));
    auto sink1 = null_sink::make(sizeof(uint8_t));

    // connect and run
    top->connect(ps, 0, sinkA, 0); top->connect(ps, 1, errsinkA, 0);
    top->connect(ps, 2, sinkB, 0); top->connect(ps, 3, errsinkB, 0);
    top->connect(ps, 4, sinkC, 0); top->connect(ps, 5, errsinkC, 0);
    top->connect(ps, 6, sinkD, 0); top->connect(ps, 7, errsinkD, 0);

    top->connect(ps, 8, sink0, 0);
    top->connect(ps, 9, sink1, 0);

    top->start();

    sleep(30);

    top->stop();
    top->wait();
}

int main(int argc, char **argv)
{
  //wire_rapid_block();
  wire_streaming();
  return 0;
}
