#include <digitizers_39/picoscope_4000a.h>
#include <digitizers_39/power_calc_ff.h>
#include <digitizers_39/mains_frequency_calc.h>

#include <gnuradio/top_block.h>

#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/streams_to_vector.h>

#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/multiply.h>
#include <gnuradio/blocks/divide.h>

#include <gnuradio/zeromq/pub_sink.h>

#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/filter/fft_filter_fff.h>

#include <gnuradio/fft/window.h>

#include <gnuradio/analog/sig_source.h>

#include <iostream>
#include <boost/format.hpp>
#include <vector>

using namespace gr::digitizers_39;
using namespace gr::blocks;

#define LOCAL 1

static const char* address(std::string ip, std::string port)
{
    //std::string fmt = "tcp://{}:{}";
    std::string tmp = (boost::format("tcp://%1%:%2%") % ip % port).str();
    return tmp.c_str();
}


void wire_streaming(int time)
{
    double samp_rate = 200000.0;
    double decimation_power = 20.0; // => 1000S out
    double decimation_freq_spec = 200.0; // => 1000S out
    size_t items = 1000;

    auto top = gr::make_top_block("ps4000a_full");

    auto ps = picoscope_4000a::make("", true);

    ps->set_aichan("A", true, 5.0, AC_1M);
    ps->set_aichan("B", true, 1.0, AC_1M);
    ps->set_aichan("C", false, 1.0, AC_1M);
    ps->set_aichan("D", false, 5.0, AC_1M);

    ps->set_samp_rate(samp_rate);
    ps->set_samples(500000, 10000);
    ps->set_buffer_size(204800); // 8192
    ps->set_nr_buffers(64); // 64
    ps->set_driver_buffer_size(102400); // 200000
    ps->set_streaming(0.0005);

    auto power_calc_block = power_calc_ff::make(0.007);
    auto mains_freq_calc = mains_frequency_calc::make(samp_rate, -90, 90);

#ifdef LOCAL == 1
    std::string ip_string = "*";
    const char * address_string = address(ip_string, "5001");
    std::cout << address_string << "\n";
    auto zeromq_pub_sink_power = gr::zeromq::pub_sink::make(sizeof(float), 4, const_cast<char *>("tcp://*:5001"), 100, false, -1);
    address_string = address(ip_string, "5002");
    auto zeromq_pub_sink_raw = gr::zeromq::pub_sink::make(sizeof(float), 2, const_cast<char *>("tcp://*:5002"), 100, false, -1);
    address_string = address(ip_string, "5003");
    auto zeromq_pub_sink_raw_band_pass = gr::zeromq::pub_sink::make(sizeof(float), 2, const_cast<char *>("tcp://*:5003"), 100, false, -1);
    address_string = address(ip_string, "5004");
    auto zeromq_pub_sink_mains_frequency = gr::zeromq::pub_sink::make(sizeof(float), 1, const_cast<char *>("tcp://*:5004"), 100, false, -1);
    address_string = address(ip_string, "5005");
    auto zeromq_pub_sink_frequency_spectrum = gr::zeromq::pub_sink::make(sizeof(float), 1, const_cast<char *>("tcp://*:5005"), 100, false, -1);
#else
    std::string ip_string = "10.0.0.2";
    const char* address_string = address(ip_string, "5001");
    auto zeromq_pub_sink_power = gr::zeromq::pub_sink::make(sizeof(float), 4, const_cast<char *>(address_string), 100, false, -1);
    address_string = address(ip_string, "5002");
    auto zeromq_pub_sink_raw = gr::zeromq::pub_sink::make(sizeof(float), 2, const_cast<char *>(address_string), 100, false, -1);
    address_string = address(ip_string, "5003");
    auto zeromq_pub_sink_raw_band_pass = gr::zeromq::pub_sink::make(sizeof(float), 2, const_cast<char *>(address_string), 100, false, -1);
    address_string = address(ip_string, "5004");
    auto zeromq_pub_sink_mains_frequency = gr::zeromq::pub_sink::make(sizeof(float), 1, const_cast<char *>(address_string), 100, false, -1);
    address_string = address(ip_string, "5005");
    auto zeromq_pub_sink_frequency_spectrum = gr::zeromq::pub_sink::make(sizeof(float), 1, const_cast<char *>(address_string), 100, false, -1);
#endif

    auto blocks_streams_to_vector_power = gr::blocks::streams_to_vector::make(sizeof(float)*1, 4);
    auto blocks_streams_to_vector_raw = gr::blocks::streams_to_vector::make(sizeof(float)*1, 2);
    auto blocks_streams_to_vector_raw_band_pass = gr::blocks::streams_to_vector::make(sizeof(float)*1, 2);

    auto blocks_streams_to_mains_frequency = gr::blocks::streams_to_vector::make(sizeof(float)*1, 1);

    auto blocks_multiply_xx_raw_apperent_power = gr::blocks::multiply_ff::make(1);

    auto low_pass_filter_0_0 = gr::filter::fir_filter_fff::make(
            decimation_freq_spec,
            gr::filter::firdes::low_pass(
                1,
                samp_rate,
                20,
                100,
                gr::fft::window::win_type::WIN_HAMMING,
                6.76));

    std::vector<float> taps = {items};
    auto fft_filter_xxx_0 = gr::filter::fft_filter_fff::make(
        1,
        taps,
        1);
    fft_filter_xxx_0->declare_sample_delay(0);

    // Preprocessing

    auto low_pass_filter_0_1_2 = gr::filter::fir_filter_fff::make(
        1,
        gr::filter::firdes::low_pass(
            1,
            10000,
            60,
            10,
            gr::fft::window::win_type::WIN_HANN,
            6.76));

    auto low_pass_filter_0_1_1 = gr::filter::fir_filter_fff::make(
        1,
        gr::filter::firdes::low_pass(
            1,
            10000,
            60,
            10,
            gr::fft::window::win_type::WIN_HANN,
            6.76));

    auto low_pass_filter_0_1_0 = gr::filter::fir_filter_fff::make(
        1,
        gr::filter::firdes::low_pass(
            1,
            10000,
            60,
            10,
            gr::fft::window::win_type::WIN_HANN,
            6.76));

    auto low_pass_filter_0_1 = gr::filter::fir_filter_fff::make(
        1,
        gr::filter::firdes::low_pass(
            1,
            10000,
            60,
            10,
            gr::fft::window::win_type::WIN_HANN,
            6.76));

    auto blocks_multiply_xx_0_2 = gr::blocks::multiply_ff::make(1);

    auto blocks_multiply_xx_0_1 = gr::blocks::multiply_ff::make(1);

    auto blocks_multiply_xx_0_0 = gr::blocks::multiply_ff::make(1);

    auto blocks_multiply_xx_0 = gr::blocks::multiply_ff::make(1);

    auto blocks_multiply_const_vxx_current = gr::blocks::multiply_const_ff::make(2.5);

    auto blocks_multiply_const_vxx_voltage = gr::blocks::multiply_const_ff::make(100);

    auto blocks_divide_xx_0_0 = gr::blocks::divide_ff::make(1);

    auto blocks_divide_xx_0 = gr::blocks::divide_ff::make(1);

    auto band_pass_filter_0_0 = gr::filter::fir_filter_fff::make(
        decimation_power,
        gr::filter::firdes::band_pass(
            1,
            samp_rate,
            20,
            80,
            10,
            gr::fft::window::win_type::WIN_HANN,
            6.76));

    auto band_pass_filter_0 = gr::filter::fir_filter_fff::make(
        decimation_power,
        gr::filter::firdes::band_pass(
            1,
            samp_rate,
            20,
            80,
            10,
            gr::fft::window::win_type::WIN_HANN,
            6.76));

    auto analog_sig_source_x_0_1 = gr::analog::sig_source_f::make(10000, gr::analog::GR_SIN_WAVE, 55, 1, 0,0);

    auto analog_sig_source_x_0_0_0 = gr::analog::sig_source_f::make(10000, gr::analog::GR_COS_WAVE, 55, 1, 0,0);

    // RAW Pico Data
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

    // Preprocessing
    top->connect(analog_sig_source_x_0_0_0, 0, blocks_multiply_xx_0_0, 1);
    top->connect(analog_sig_source_x_0_0_0, 0, blocks_multiply_xx_0_1, 1);
    top->connect(analog_sig_source_x_0_1, 0, blocks_multiply_xx_0, 1);
    top->connect(analog_sig_source_x_0_1, 0, blocks_multiply_xx_0_2, 1);
    top->connect(band_pass_filter_0, 0, blocks_multiply_xx_0_0, 0);
    top->connect(band_pass_filter_0, 0, blocks_multiply_xx_0_2, 0);
    top->connect(band_pass_filter_0_0, 0, blocks_multiply_xx_0, 0);
    top->connect(band_pass_filter_0_0, 0, blocks_multiply_xx_0_1, 0);
    top->connect(blocks_multiply_const_vxx_voltage, 0, band_pass_filter_0_0, 0);
    top->connect(blocks_multiply_const_vxx_current, 0, band_pass_filter_0, 0);
    top->connect(blocks_multiply_xx_0, 0, low_pass_filter_0_1, 0);
    top->connect(blocks_multiply_xx_0_0, 0, low_pass_filter_0_1_0, 0);
    top->connect(blocks_multiply_xx_0_1, 0, low_pass_filter_0_1_1, 0);
    top->connect(blocks_multiply_xx_0_2, 0, low_pass_filter_0_1_2, 0);
    
    top->connect(low_pass_filter_0_1, 0, blocks_divide_xx_0, 0);
    top->connect(low_pass_filter_0_1_0, 0, blocks_divide_xx_0_0, 1);
    top->connect(low_pass_filter_0_1_1, 0, blocks_divide_xx_0, 1);
    top->connect(low_pass_filter_0_1_2, 0, blocks_divide_xx_0_0, 0);

    // _IN_
    top->connect(ps, 0, blocks_multiply_const_vxx_voltage, 0);
    top->connect(ps, 2, blocks_multiply_const_vxx_current, 0);

    // Power
    top->connect(blocks_streams_to_vector_power, 0, zeromq_pub_sink_power, 0);
    top->connect(power_calc_block, 0, blocks_streams_to_vector_power, 0);
    top->connect(power_calc_block, 1, blocks_streams_to_vector_power, 1);
    top->connect(power_calc_block, 2, blocks_streams_to_vector_power, 2);
    top->connect(power_calc_block, 3, blocks_streams_to_vector_power, 3);

    top->connect(band_pass_filter_0, 0, power_calc_block, 0);
    top->connect(band_pass_filter_0_0, 0, power_calc_block, 1);
    top->connect(blocks_divide_xx_0, 0, power_calc_block, 2);
    top->connect(blocks_divide_xx_0_0, 0, power_calc_block, 3);

    // Raw
    top->connect(blocks_streams_to_vector_raw, 0, zeromq_pub_sink_raw, 0);
    top->connect(blocks_multiply_const_vxx_voltage, 0, blocks_streams_to_vector_raw, 0);
    top->connect(blocks_multiply_const_vxx_current, 0, blocks_streams_to_vector_raw, 1);

    // Band Pass
    top->connect(blocks_streams_to_vector_raw_band_pass, 0, zeromq_pub_sink_raw_band_pass, 0);
    top->connect(band_pass_filter_0_0, 0, blocks_streams_to_vector_raw_band_pass, 0);
    top->connect(band_pass_filter_0, 0, blocks_streams_to_vector_raw_band_pass, 1);

    // Mains Freq
    top->connect(blocks_streams_to_mains_frequency, 0, zeromq_pub_sink_mains_frequency, 0);
    top->connect(mains_freq_calc, 0, blocks_streams_to_mains_frequency, 0);
    top->connect(blocks_multiply_const_vxx_voltage, 0, mains_freq_calc, 0);

    // Freq Spec
    top->connect(fft_filter_xxx_0, 0, zeromq_pub_sink_frequency_spectrum, 0);
    top->connect(low_pass_filter_0_0, 0, fft_filter_xxx_0, 0);
    top->connect(blocks_multiply_xx_raw_apperent_power, 0, low_pass_filter_0_0, 0);

     // Calc apperent power [S]
    // top->connect(blocks_divide_xx_0, 0, blocks_multiply_xx_raw_apperent_power, 0);
    // top->connect(blocks_divide_xx_0_0, 0, blocks_multiply_xx_raw_apperent_power, 1);
    top->connect(blocks_multiply_const_vxx_voltage, 0, blocks_multiply_xx_raw_apperent_power, 0);
    top->connect(blocks_multiply_const_vxx_current, 0, blocks_multiply_xx_raw_apperent_power, 1);


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
