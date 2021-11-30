
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/analog/noise_source.h>
#include <gnuradio/blocks/file_source.h>
#include <digitizers_39/mains_frequency_calc.h>

#include <gnuradio/top_block.h>

#include <gnuradio/blocks/streams_to_vector.h>
#include <gnuradio/blocks/add_blk.h>

#include <gnuradio/zeromq/pub_sink.h>

#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/filter/mmse_resampler_ff.h>

#include <gnuradio/fft/window.h>

#include <vector>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
using namespace std;


using namespace gr::digitizers_39;
using namespace gr::blocks;

void power_calc_streaming(int runtime=60,
                        double samp_rate=2000000.0,
                        float low_threshold=-0.5,
                        float high_threshold=0.5,
                        int source_1_amp=5, 
                        int source_2_amp=2,
                        int source_1_freq=50,
                        int source_2_freq=50,
                        int source_1_phase=0, 
                        int source_2_phase=1,
                        int source_1_noise_percent=1,
                        int source_2_noise_percent=1)
{   
    float source_1_noise_amp_percentage_calc = 0.0;
    float source_2_noise_amp_percentage_calc = 0.0;

    if (source_1_noise_percent != 0)
    {
        source_1_noise_amp_percentage_calc = float((float)source_1_amp * (float)source_1_noise_percent / 100.0);
    }

    if (source_2_noise_percent != 0)
    {
        source_2_noise_amp_percentage_calc = float((float)source_2_amp * (float)source_2_noise_percent / 100.0);
    }

    auto top = gr::make_top_block("mains_freq_block_test");

    auto mains_freq_calc_block_source_1 = mains_frequency_calc::make(samp_rate, low_threshold, high_threshold);
    // auto mains_freq_calc_block_source_2 = mains_frequency_calc::make(samp_rate, low_threshold, high_threshold);

    auto zeromq_pub_sink = gr::zeromq::pub_sink::make(sizeof(float), 2, const_cast<char *>("tcp://*:5001"), 100, false, -1);
    auto blocks_streams_to_vector = gr::blocks::streams_to_vector::make(sizeof(float)*1, 2);

    // 5,0,5,0 -> P,Q,S,PHI
    auto analog_sig_source_x_0_0 = gr::analog::sig_source_f::make(samp_rate, gr::analog::GR_SIN_WAVE, source_1_freq, source_1_amp, 0, source_1_phase);
    // auto analog_noise_source_x_0_0 = gr::analog::noise_source_f::make(gr::analog::GR_GAUSSIAN, source_1_noise_amp_percentage_calc, 0);
    // auto blocks_add_xx_0_0 = gr::blocks::add_ff::make(1);

    // auto analog_sig_source_x_0 = gr::analog::sig_source_f::make(samp_rate, gr::analog::GR_SIN_WAVE, source_2_freq, source_2_amp, 0, source_2_phase);
    // auto analog_noise_source_x_0 = gr::analog::noise_source_f::make(gr::analog::GR_GAUSSIAN, source_2_noise_amp_percentage_calc, 0);
    // auto blocks_add_xx_0 = gr::blocks::add_ff::make(1);

    // connect PS to stream-to-vector-block and then ZeroMQ Sink
    top->connect(blocks_streams_to_vector, 0, zeromq_pub_sink, 0);
    top->connect(analog_sig_source_x_0_0, 0, blocks_streams_to_vector, 0);
    top->connect(mains_freq_calc_block_source_1, 0, blocks_streams_to_vector, 1);

    top->connect(analog_sig_source_x_0_0, 0, mains_freq_calc_block_source_1, 0);

    // top->connect(blocks_add_xx_0, 0, blocks_streams_to_vector, 2);
    // top->connect(mains_freq_calc_block_source_2, 0, blocks_streams_to_vector, 3);

    // top->connect(blocks_add_xx_0_0, 0, mains_freq_calc_block_source_1, 0);
    // top->connect(blocks_add_xx_0, 0, mains_freq_calc_block_source_2, 0);

    // top->connect(analog_noise_source_x_0_0, 0, blocks_add_xx_0_0, 0);
    // top->connect(analog_sig_source_x_0_0, 0, blocks_add_xx_0_0, 1);

    // top->connect(analog_noise_source_x_0, 0, blocks_add_xx_0, 0);
    // top->connect(analog_sig_source_x_0, 0, blocks_add_xx_0, 1);

    top->start();

    sleep(runtime);

    top->stop();
    top->wait();
}

int main(int argc, char **argv) {
    try {
        int runtime=60;
        double samp_rate=2000000.0;
        float low_threshold=-0.5;
        float high_threshold=-0.5;
        int source_1_amp=5;
        int source_2_amp=2;
        int source_1_freq=50;
        int source_2_freq=50;
        int source_1_phase=0; 
        int source_2_phase=1;
        int source_1_noice_percent=1;
        int source_2_noice_percent=1;

        po::options_description desc("Allowed options");
        desc.add_options()
        // First parameter describes option name/short name
        // The second is parameter to option
        // The third is description
            ("help,h", "print usage message")
            ("rt", po::value<int>(&runtime)->required()->default_value(runtime), "The runtime in seconds")
            ("sr", po::value<double>(&samp_rate)->required()->default_value(samp_rate), "The symulated rate of samples")
            ("lot", po::value<float>(&low_threshold)->required()->default_value(low_threshold), "Low threshold")
            ("hot", po::value<float>(&high_threshold)->required()->default_value(high_threshold), "High threshold")
            ("s1amp", po::value<int>(&source_1_amp)->required()->default_value(source_1_amp), "Simulated amplitude of source 1")
            ("s2amp", po::value<int>(&source_2_amp)->required()->default_value(source_2_amp), "Simulated amplitude of source 2")
            ("s1freq", po::value<int>(&source_1_freq)->required()->default_value(source_1_freq), "Simulated frequency of source 1")
            ("s2freq", po::value<int>(&source_2_freq)->required()->default_value(source_2_freq), "Simulated frequency of source 2")
            ("s1phase", po::value<int>(&source_1_phase)->required()->default_value(source_1_phase), "Simulated phase of source 1")
            ("s2phase", po::value<int>(&source_2_phase)->required()->default_value(source_2_phase), "Simulated phase of source 2")
            ("s1np", po::value<int>(&source_1_noice_percent)->required()->default_value(source_1_noice_percent), "Simulated noice for source 1 in percent (for amplitude)")
            ("s2np", po::value<int>(&source_2_noice_percent)->required()->default_value(source_2_noice_percent), "Simulated noice for source 2 in percent (for amplitude)")
        ;

        po::variables_map vm;
        //po::store(parse_command_line(argc, argv, desc), vm);
        po::store(
        po::command_line_parser(argc, argv).options(desc).style(po::command_line_style::unix_style | po::command_line_style::allow_short).run(), vm);
        po::notify(vm);

        if (vm.count("help")) {  
            cout << desc << "\n";
            return 0;
        }

        std::cout << "start example\n";
        power_calc_streaming(vm["rt"].as<int>(),
                            vm["sr"].as<double>(),
                            vm["lot"].as<float>(),
                            vm["hot"].as<float>(),
                            vm["s1amp"].as<int>(),
                            vm["s2amp"].as<int>(),
                            vm["s1freq"].as<int>(),
                            vm["s2freq"].as<int>(),
                            vm["s1phase"].as<int>(),
                            vm["s2phase"].as<int>(),
                            vm["s1np"].as<int>(),
                            vm["s2np"].as<int>());
        std::cout << "example finished\n";
    }
    catch(exception& e) {
        cerr << e.what() << "\n";
    }
}
