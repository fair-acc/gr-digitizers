/* -*- c++ -*- */
/*
 * Copyright 2022 me.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/attributes.h>
#include <pulsed_power_daq/mains_frequency_calc.h>
#include "mains_frequency_calc_impl.h"
#include <boost/test/unit_test.hpp>
#include <gnuradio/top_block.h>
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/blocks/vector_sink.h>
#include <boost/thread/thread.hpp>

namespace gr {
namespace pulsed_power_daq {

BOOST_AUTO_TEST_SUITE(mains_frequency_calc_testing);

bool isSimilar(float first, float second, float decimals){
        float difference = 1/(decimals*10);
        return abs(first-second) < difference;
}


BOOST_AUTO_TEST_CASE(test_mains_frequency_calc_55_Hz_no_noise)
{
        gr::top_block_sptr tb = gr::make_top_block("top");
        int samp_rate = 32000;
        gr::analog::gr_waveform_t waveform = gr::analog::gr_waveform_t::GR_SIN_WAVE;
        double ampl = 2;
        float offset = 0;
        float phase = 0;
        double frequency = 55;
        gr::block_sptr signal_source = gr::analog::sig_source<float>::make(samp_rate, waveform, frequency, ampl, offset, phase);

        float alpha = 0.0001;
        mains_frequency_calc::sptr calc_block = mains_frequency_calc::make(samp_rate, -100, 100);

        gr::blocks::vector_sink<float>::sptr vector_sink = gr::blocks::vector_sink<float>::make();

        tb->connect(signal_source,0,calc_block,0);
        tb->connect(calc_block,0,vector_sink,0);

        tb->start();

        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

        tb->stop();

        std::vector<float> result_data = vector_sink->data();

        result_data.erase(result_data.begin()+samp_rate*2);

        int res_size = result_data.size();
        for (int i = 0; i < res_size; i++)
        {       
                //std::cout<<result_data.at(i);
                if (!isSimilar(result_data.at(i), frequency, 6))
                {
                        BOOST_CHECK(false);
                        break;
                }
                
        }
        BOOST_CHECK (true);
        

}
BOOST_AUTO_TEST_SUITE_END();
    } /* namespace pulsed_power_daq */
} /* namespace gr */
