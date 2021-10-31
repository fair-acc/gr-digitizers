#include <digitizers_39/picoscope_4000a.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/streams_to_vector.h>
#include <gnuradio/zeromq/pub_sink.h>

#include <iostream>
#include <vector>

using namespace gr::digitizers_39;
using namespace gr::blocks;

void wire_streaming()
{
    auto top = gr::make_top_block("channels");

    auto ps = picoscope_4000a::make("", true);

    ps->set_aichan("A", true, 5.0, AC_1M);
    ps->set_aichan("B", true, 5.0, AC_1M);
    ps->set_aichan("C", true, 5.0, AC_1M);
    ps->set_aichan("D", false, 5.0, AC_1M);

    //ps->set_aichan_trigger("A", TRIGGER_DIRECTION_RISING, 1.0);

    auto zeromq_pub_sink = gr::zeromq::pub_sink::make(sizeof(float), 3, const_cast<char *>("tcp://*:5001"), 100, false, -1);
    auto blocks_streams_to_vector = gr::blocks::streams_to_vector::make(sizeof(float)*1, 3);

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
    auto sinkE = null_sink::make(sizeof(float));
    auto sinkF = null_sink::make(sizeof(float));
    auto sinkG = null_sink::make(sizeof(float));
    auto sinkH = null_sink::make(sizeof(float));
    auto errsinkA = null_sink::make(sizeof(float));
    auto errsinkB = null_sink::make(sizeof(float));
    auto errsinkC = null_sink::make(sizeof(float));
    auto errsinkD = null_sink::make(sizeof(float));
    auto errsinkE = null_sink::make(sizeof(float));
    auto errsinkF = null_sink::make(sizeof(float));
    auto errsinkG = null_sink::make(sizeof(float));
    auto errsinkH = null_sink::make(sizeof(float));


    // connect and run
    top->connect(ps, 0, sinkA, 0); top->connect(ps, 1, errsinkA, 0);
    top->connect(ps, 2, sinkB, 0); top->connect(ps, 3, errsinkB, 0);
    top->connect(ps, 4, sinkC, 0); top->connect(ps, 5, errsinkC, 0);
    top->connect(ps, 6, sinkD, 0); top->connect(ps, 7, errsinkD, 0);

    top->connect(ps, 8, sinkE, 0); top->connect(ps, 9, errsinkE, 0);
    top->connect(ps, 10, sinkF, 0); top->connect(ps, 11, errsinkF, 0);
    top->connect(ps, 12, sinkG, 0); top->connect(ps, 13, errsinkG, 0);
    top->connect(ps, 14, sinkH, 0); top->connect(ps, 15, errsinkH, 0);

    // connect PS to stream-to-vector-block and then ZeroMQ Sink
    top->connect(blocks_streams_to_vector, 0, zeromq_pub_sink, 0);
    top->connect(ps, 0, blocks_streams_to_vector, 1);
    top->connect(ps, 2, blocks_streams_to_vector, 0);
    top->connect(ps, 4, blocks_streams_to_vector, 2);

    top->start();

    sleep(30);

    top->stop();
    top->wait();
}

int main(int argc, char **argv) {
  std::cout << "start example\n";
  wire_streaming();
  std::cout << "example finished\n";
  return 0;
}
