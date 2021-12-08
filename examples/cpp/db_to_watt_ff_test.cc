#include <digitizers_39/db_to_watt_ff.h>

#include <gnuradio/analog/sig_source.h>

#include <gnuradio/top_block.h>

#include <gnuradio/blocks/null_sink.h>

#include <gnuradio/blocks/stream_to_vector.h>
#include <gnuradio/blocks/multiply.h>

#include <gnuradio/zeromq/pub_sink.h>

#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/filter/fft_filter_fff.h>

#include <gnuradio/fft/window.h>

#include <iostream>
#include <vector>

using namespace gr::digitizers_39;
using namespace gr::blocks;

void wire_streaming(int time)
{

    double source_samp_rate = 200000.0;
    double decimation = 2000.0;
    size_t items = 1000;

    auto top = gr::make_top_block("db_to_watt");

    auto db_to_watt_ff_block = db_to_watt_ff::make();

    auto zeromq_pub_sink_0 = gr::zeromq::pub_sink::make(sizeof(float), 1, const_cast<char *>("tcp://*:5001"), 100, false, -1);
    auto blocks_stream_to_vector = gr::blocks::stream_to_vector::make(sizeof(float)*1, 1);

    
    auto analog_sig_source_x_0_0 = gr::analog::sig_source_f::make(source_samp_rate, gr::analog::GR_SIN_WAVE, 50, 325, 0, 0);

    auto analog_sig_source_x_0 = gr::analog::sig_source_f::make(source_samp_rate, gr::analog::GR_SIN_WAVE, 50, 5.3, 0, 0);

    auto low_pass_filter_0_0 = gr::filter::fir_filter_fff::make(
            decimation,
            gr::filter::firdes::low_pass(
                1,
                source_samp_rate,
                80,
                100,
                gr::fft::window::win_type::WIN_HAMMING,
                6.76));

    std::vector<float> taps = {items};
    auto fft_filter_xxx_0 = gr::filter::fft_filter_fff::make(
        1,
        taps,
        1);
    fft_filter_xxx_0->declare_sample_delay(0);

    auto blocks_multiply_xx_0 = gr::blocks::multiply_ff::make(1);

    top->connect(db_to_watt_ff_block, 0, zeromq_pub_sink_0, 0);
    //top->connect(db_to_watt_ff_block, 0, blocks_stream_to_vector, 0);

    // top->connect(db_to_watt_ff_block, 0, fft_filter_xxx_0, 0);
    top->connect(low_pass_filter_0_0, 0, db_to_watt_ff_block, 0);

    top->connect(blocks_multiply_xx_0, 0, low_pass_filter_0_0, 0);

     // Calc [S]
    top->connect(analog_sig_source_x_0_0, 0, blocks_multiply_xx_0, 0);
    top->connect(analog_sig_source_x_0, 0, blocks_multiply_xx_0, 1);

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
