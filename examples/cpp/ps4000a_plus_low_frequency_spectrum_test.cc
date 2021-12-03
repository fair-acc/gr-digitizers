#include <digitizers_39/picoscope_4000a.h>

#include <gnuradio/top_block.h>

#include <gnuradio/blocks/null_sink.h>

#include <gnuradio/blocks/multiply_const.h>
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
    double source_samp_rate = 2000000.0;
    double decimation = 20000.0;
    size_t items = 1000;

    auto top = gr::make_top_block("ps4000a_plus_low_frequency_spectrum_test");

    auto ps = picoscope_4000a::make("", true);

    ps->set_aichan("A", true, 5.0, AC_1M);
    ps->set_aichan("B", true, 1.0, AC_1M);
    ps->set_aichan("C", false, 1.0, AC_1M);
    ps->set_aichan("D", false, 5.0, AC_1M);

    ps->set_samp_rate(source_samp_rate);
    ps->set_samples(500000, 10000);
    ps->set_buffer_size(2079152); // 8192
    ps->set_nr_buffers(64); // 64
    ps->set_driver_buffer_size(204800); // 200000
    ps->set_streaming(0.0005);

    auto zeromq_pub_sink_0 = gr::zeromq::pub_sink::make(sizeof(float), 1, const_cast<char *>("tcp://*:5001"), 100, false, -1);
    
    auto blocks_multiply_const_vxx_0_0 = gr::blocks::multiply_const_ff::make(100);
    auto blocks_multiply_const_vxx_0 = gr::blocks::multiply_const_ff::make(2.5);

    auto low_pass_filter_0_0 = gr::filter::fir_filter_fff::make(
            decimation,
            gr::filter::firdes::low_pass(
                1,
                source_samp_rate,
                20,
                100,
                gr::fft::window::win_type::WIN_HAMMING,
                6.76));

    // auto fft_vxx_0 = gr::fft::fft_vfc::make(items, true, fft::window::blackmanharris(items), true, 1);

    std::vector<float> taps = {items};
    auto fft_filter_xxx_0 = gr::filter::fft_filter_fff::make(
        1,
        taps,
        1);
    fft_filter_xxx_0->declare_sample_delay(0);

    auto blocks_multiply_xx_0 = gr::blocks::multiply_ff::make(1);

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

    top->connect(fft_filter_xxx_0, 0, zeromq_pub_sink_0, 0);

    top->connect(low_pass_filter_0_0, 0, fft_filter_xxx_0, 0);

    top->connect(blocks_multiply_xx_0, 0, low_pass_filter_0_0, 0);

     // Calc [S]
    top->connect(blocks_multiply_const_vxx_0_0, 0, blocks_multiply_xx_0, 0);
    top->connect(blocks_multiply_const_vxx_0, 1, blocks_multiply_xx_0, 1);

    top->connect(ps, 0, blocks_multiply_const_vxx_0_0, 0);
    top->connect(ps, 2, blocks_multiply_const_vxx_0, 0);

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
