#include <digitizers_39/picoscope_4000a.h>
#include <digitizers_39/mains_frequency_calc.h>

#include <gnuradio/top_block.h>

#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/streams_to_vector.h>
#include <gnuradio/blocks/multiply_const.h>

#include <gnuradio/zeromq/pub_sink.h>

#include <iostream>
#include <vector>

using namespace gr::digitizers_39;
using namespace gr::blocks;

void wire_streaming(int time)
{
    double samp_rate = 2000000.0;
    double decimation = 2000.0;
    float threshold = 100;
    float low_threshold =- threshold;
    float high_threshold = threshold;

    auto top = gr::make_top_block("ps4000a_mains_freq_block_test");

    auto ps = picoscope_4000a::make("", true);

    ps->set_aichan("A", true, 5.0, AC_1M);
    ps->set_aichan("B", false, 1.0, AC_1M);
    ps->set_aichan("C", false, 1.0, AC_1M);
    ps->set_aichan("D", false, 5.0, AC_1M);

    ps->set_samp_rate(samp_rate);
    ps->set_samples(500000, 10000);
    ps->set_buffer_size(2079152); // 8192
    ps->set_nr_buffers(64); // 64
    ps->set_driver_buffer_size(204800); // 200000
    ps->set_streaming(0.0005);

    auto mains_freq_calc_block_source_1 = mains_frequency_calc::make(samp_rate, -100, 100);

    auto zeromq_pub_sink = gr::zeromq::pub_sink::make(sizeof(float), 2, const_cast<char *>("tcp://*:5001"), 100, false, -1);
    auto blocks_streams_to_vector = gr::blocks::streams_to_vector::make(sizeof(float)*1, 2);

    auto blocks_multiply_const_vxx_0_0 = gr::blocks::multiply_const_ff::make(100);

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
    top->connect(blocks_multiply_const_vxx_0_0, 0, blocks_streams_to_vector, 0);
    top->connect(mains_freq_calc_block_source_1, 0, blocks_streams_to_vector, 1);

    top->connect(blocks_multiply_const_vxx_0_0, 0, mains_freq_calc_block_source_1, 0);

    top->connect(ps, 0, blocks_multiply_const_vxx_0_0, 0);


    top->start();

    sleep(time);

    top->stop();
    top->wait();
}

int main(int argc, char **argv) {
  int time = 60;

  if(argc > 1)
  {
    time = std::stoi(argv[1]);

  }
  std::cout << "start example\n";
  wire_streaming(time);
  std::cout << "example finished\n";
  return 0;
}
