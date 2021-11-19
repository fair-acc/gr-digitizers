
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/blocks/file_source.h>
#include <digitizers_39/power_calc.h>

#include <gnuradio/top_block.h>

#include <gnuradio/blocks/streams_to_vector.h>
#include <gnuradio/zeromq/pub_sink.h>

#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/fft/window.h>

#include <iostream>
#include <vector>

using namespace gr::digitizers_39;
using namespace gr::blocks;

void power_calc_streaming()
{
    auto top = gr::make_top_block("power_block_test");

    auto power_calc_block = power_calc::make(0.00001);

    auto zeromq_pub_sink = gr::zeromq::pub_sink::make(sizeof(float), 4, const_cast<char *>("tcp://*:5001"), 100, false, -1);
    auto blocks_streams_to_vector = gr::blocks::streams_to_vector::make(sizeof(float)*1, 4);
    
    auto band_pass_filter_0_0 = gr::filter::fir_filter_fcc::make(
        100.0,
        gr::filter::firdes::complex_band_pass(
            1.0,
            100000.0,
            10,
            100,
            50,
            gr::fft::window::WIN_HANN,
            6.76));

    auto band_pass_filter_0 = gr::filter::fir_filter_fcc::make(
        100.0,
        gr::filter::firdes::complex_band_pass(
            1.0,
            100000.0,
            10,
            100,
            50,
            gr::fft::window::WIN_HANN,
            6.76));

    // 5,0,5,0 -> P,Q,S,PHI
    auto analog_sig_source_x_0_0 = gr::analog::sig_source_f::make(100000.0, gr::analog::GR_SIN_WAVE, 50, 5, 0, 0);
    auto analog_sig_source_x_0 = gr::analog::sig_source_f::make(100000.0, gr::analog::GR_SIN_WAVE, 50, 2, 0, 1);
    // auto blocks_file_source_1 = gr::blocks::file_source::make(sizeof(float)*1, "/home/neumann/voltage", true, 0, 0);
    // auto blocks_file_source_0 = gr::blocks::file_source::make(sizeof(float)*1, "/home/neumann/current", true, 0, 0);

    // connect PS to stream-to-vector-block and then ZeroMQ Sink
    top->connect(blocks_streams_to_vector, 0, zeromq_pub_sink, 0);
    top->connect(power_calc_block, 0, blocks_streams_to_vector, 0);
    top->connect(power_calc_block, 1, blocks_streams_to_vector, 1);
    top->connect(power_calc_block, 2, blocks_streams_to_vector, 2);
    top->connect(power_calc_block, 3, blocks_streams_to_vector, 3);

    top->connect(analog_sig_source_x_0, 0, band_pass_filter_0_0, 0);
    top->connect(analog_sig_source_x_0_0, 0, band_pass_filter_0, 0);

    top->connect(band_pass_filter_0, 0, power_calc_block, 0);
    top->connect(band_pass_filter_0_0, 0, power_calc_block, 1);

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
