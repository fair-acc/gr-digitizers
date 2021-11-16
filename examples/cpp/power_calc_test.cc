
#include <gnuradio/analog/sig_source.h>
#include <digitizers_39/power_calc.h>
#include <gnuradio/fft/goertzel_fc.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/stream_to_vector.h>
#include <gnuradio/blocks/streams_to_vector.h>
#include <gnuradio/zeromq/pub_sink.h>

#include <iostream>
#include <vector>

using namespace gr::digitizers_39;
using namespace gr::blocks;

void power_calc_streaming()
{
    auto top = gr::make_top_block("power_block_test");

    auto power_calc_block = power_calc::make(0.0000001);

    auto zeromq_pub_sink = gr::zeromq::pub_sink::make(sizeof(float), 4, const_cast<char *>("tcp://*:5001"), 100, false, -1);
    auto blocks_streams_to_vector = gr::blocks::streams_to_vector::make(sizeof(float)*1, 4);
    
    auto goertzel_fc_0_0 = gr::fft::goertzel_fc::make(2000000, 1, 50);
    auto goertzel_fc_0 = gr::fft::goertzel_fc::make(2000000, 1, 50);

    auto analog_sig_source_x_0_0 = gr::analog::sig_source_f::make(2000000, gr::analog::GR_SIN_WAVE, 50, 5, 0, 0);
    auto analog_sig_source_x_0 = gr::analog::sig_source_f::make(2000000, gr::analog::GR_SIN_WAVE, 50, 2, 0, 30);

    // connect PS to stream-to-vector-block and then ZeroMQ Sink
    top->hier_block2::connect(blocks_streams_to_vector, 0, zeromq_pub_sink, 0);
    top->hier_block2::connect(power_calc_block, 0, blocks_streams_to_vector, 0);
    top->hier_block2::connect(power_calc_block, 1, blocks_streams_to_vector, 1);
    top->hier_block2::connect(power_calc_block, 2, blocks_streams_to_vector, 2);
    top->hier_block2::connect(power_calc_block, 3, blocks_streams_to_vector, 3);

    top->hier_block2::connect(analog_sig_source_x_0, 0, goertzel_fc_0_0, 0);
    top->hier_block2::connect(analog_sig_source_x_0_0, 0, goertzel_fc_0, 0);
    top->hier_block2::connect(goertzel_fc_0, 0, power_calc_block, 0);
    top->hier_block2::connect(goertzel_fc_0_0, 0, power_calc_block, 1);

    top->start();

    sleep(60);

    top->stop();
    top->wait();
}

int main(int argc, char **argv) {
  std::cout << "start example\n";
  power_calc_streaming();
  std::cout << "example finished\n";
  return 0;
}
