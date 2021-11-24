#include <digitizers_39/picoscope_4000a.h>
#include <digitizers_39/power_calc.h>

#include <gnuradio/top_block.h>

#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/streams_to_vector.h>

#include <gnuradio/zeromq/pub_sink.h>

#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/filter/mmse_resampler_ff.h>

#include <gnuradio/fft/window.h>

#include <iostream>
#include <vector>

using namespace gr::digitizers_39;
using namespace gr::blocks;

void wire_streaming(int time)
{
    double samp_rate = 2000000.0;

    auto top = gr::make_top_block("ps4000a_to_power_calc");

    auto ps = picoscope_4000a::make("", true);

    ps->set_aichan("A", true, 5.0, AC_1M);
    ps->set_aichan("B", true, 1.0, AC_1M);
    ps->set_aichan("C", false, 1.0, AC_1M);
    ps->set_aichan("D", false, 5.0, AC_1M);

    ps->set_samp_rate(samp_rate);
    ps->set_samples(500000, 10000);
    ps->set_buffer_size(8192);
    ps->set_nr_buffers(64);
    ps->set_driver_buffer_size(200000);
    ps->set_streaming(0.0005);

    auto power_calc_block = power_calc::make(0.00001);

    auto zeromq_pub_sink = gr::zeromq::pub_sink::make(sizeof(float), 10, const_cast<char *>("tcp://10.0.0.2:5001"), 100, false, -1);
    auto blocks_streams_to_vector = gr::blocks::streams_to_vector::make(sizeof(float)*1, 10);

    auto mmse_resampler_xx_0_0 = gr::filter::mmse_resampler_ff::make(
            0,
            200);

    auto mmse_resampler_xx_0 = gr::filter::mmse_resampler_ff::make(
            0,
            200);

    auto band_pass_filter_0_0 = gr::filter::fir_filter_fcc::make(
        200.0,
        gr::filter::firdes::complex_band_pass(
            1.0,
            samp_rate,
            10,
            100,
            50,
            gr::fft::window::WIN_HANN,
            6.76));

    auto band_pass_filter_0 = gr::filter::fir_filter_fcc::make(
        200.0,
        gr::filter::firdes::complex_band_pass(
            1.0,
            samp_rate,
            10,
            100,
            50,
            gr::fft::window::WIN_HANN,
            6.76));

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
    top->connect(power_calc_block, 0, blocks_streams_to_vector, 0);
    top->connect(power_calc_block, 1, blocks_streams_to_vector, 1);
    top->connect(power_calc_block, 2, blocks_streams_to_vector, 2);
    top->connect(power_calc_block, 3, blocks_streams_to_vector, 3);
    // top->connect(power_calc_block, 3, blocks_streams_to_vector, 4);
    top->connect(mmse_resampler_xx_0_0, 0, blocks_streams_to_vector, 4);
    top->connect(mmse_resampler_xx_0, 2, blocks_streams_to_vector, 5);

    top->connect(band_pass_filter_0, 0, power_calc_block, 0);
    top->connect(band_pass_filter_0_0, 0, power_calc_block, 1);

    top->connect(ps, 0, mmse_resampler_xx_0_0, 0);
    top->connect(ps, 2, mmse_resampler_xx_0, 0);

    top->connect(ps, 0, band_pass_filter_0_0, 0);
    top->connect(ps, 2, band_pass_filter_0, 0);


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
