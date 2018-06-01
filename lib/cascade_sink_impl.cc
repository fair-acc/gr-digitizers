/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "cascade_sink_impl.h"

namespace gr {
  namespace digitizers {

    cascade_sink::sptr
    cascade_sink::make(int alg_id,
        int delay,
        const std::vector<float> &fir_taps,
        double low_freq,
        double up_freq,
        double tr_width,
        const std::vector<double> &fb_user_taps,
        const std::vector<double> &fw_user_taps,
        double samp_rate,
        float pm_buffer,
        std::string signal_name,
        std::string unit_name)
    {
      return gnuradio::get_initial_sptr
        (new cascade_sink_impl(alg_id,
            delay,
            fir_taps,
            low_freq,
            up_freq,
            tr_width,
            fb_user_taps,
            fw_user_taps,
            samp_rate,
            pm_buffer,
            signal_name,
            unit_name));
    }

    /*
     * The private constructor
     */
    cascade_sink_impl::cascade_sink_impl(int alg_id,
        int delay,
        const std::vector<float> &fir_taps,
        double low_freq,
        double up_freq,
        double tr_width,
        const std::vector<double> &fb_user_taps,
        const std::vector<double> &fw_user_taps,
        double samp_rate,
        float pm_buffer,
        std::string signal_name,
        std::string unit_name)
      : gr::hier_block2("cascade_sink",
              gr::io_signature::make(2,2, sizeof(float)),
              gr::io_signature::make(2,2 , sizeof(float)))
    {

      int samp_rate_to_one_kilo = static_cast<int>(samp_rate / 1000.0);
      if(samp_rate != (samp_rate_to_one_kilo * 1000.0)) {
        GR_LOG_ALERT(logger, "SAMPLE RATE NOT DIVISIBLE BY 1000! OUTPUTS NOT EXACT: 1k, 100, 10, 1 Hz!");
      }

      //create blocks
      d_agg1000 = block_aggregation::make(alg_id, samp_rate_to_one_kilo, delay, fir_taps, low_freq, up_freq, tr_width, fb_user_taps, fw_user_taps, samp_rate);
      d_agg100 = block_aggregation::make(alg_id, 10, delay, fir_taps, low_freq, up_freq, tr_width, fb_user_taps, fw_user_taps, 1000);
      d_agg10 = block_aggregation::make(alg_id, 10, delay, fir_taps, low_freq, up_freq, tr_width, fb_user_taps, fw_user_taps, 100);
      d_agg1 = block_aggregation::make(alg_id, 10, delay, fir_taps, low_freq, up_freq, tr_width, fb_user_taps, fw_user_taps, 10);

      // FESA will see updates @10Hz at most.
      d_snk1000 = time_domain_sink::make(signal_name+"@1000Hz", unit_name, 1000.0,  100, 10, TIME_SINK_MODE_STREAMING);
      d_snk100  = time_domain_sink::make(signal_name+"@100Hz",  unit_name, 100.0,    10, 10, TIME_SINK_MODE_STREAMING);
      d_snk10   = time_domain_sink::make(signal_name+"@10Hz",   unit_name, 10.0,      1, 10, TIME_SINK_MODE_STREAMING);
      d_snk1    = time_domain_sink::make(signal_name+"@1Hz",    unit_name, 1,         1,  2, TIME_SINK_MODE_STREAMING);

      // To prevent tag explosion we limit the output buffer size. For each output item the aggregation
      // block will generate only one acq_info tag. Therefore number 1024 seems to be reasonable... Note
      // GR works only with output buffers of one page....
      d_agg1000->set_max_output_buffer(1024);
      d_agg100->set_max_output_buffer(1024);
      d_agg10->set_max_output_buffer(1024);
      d_agg1->set_max_output_buffer(1024);

      d_pm_raw = post_mortem_sink::make(signal_name+"PM@RAW", unit_name, samp_rate, pm_buffer * samp_rate);
      d_pm_1000 = post_mortem_sink::make(signal_name+"PM@10kHz", unit_name, 1000.0f, pm_buffer * 1000.0f);

      //connect block cascade
      connect(self(), 0, d_agg1000, 0);
      connect(self(), 1, d_agg1000, 1);

      connect(d_agg1000, 0, d_agg100, 0);
      connect(d_agg1000, 1, d_agg100, 1);

      connect(d_agg100, 0, d_agg10, 0);
      connect(d_agg100, 1, d_agg10, 1);

      connect(d_agg10, 0, d_agg1, 0);
      connect(d_agg10, 1, d_agg1, 1);

      //FESA interfaces
      connect(d_agg1000, 0, d_snk1000, 0);
      connect(d_agg1000, 1, d_snk1000, 1);

      connect(d_agg100, 0, d_snk100, 0);
      connect(d_agg100, 1, d_snk100, 1);

      connect(d_agg10, 0, d_snk10, 0);
      connect(d_agg10, 1, d_snk10, 1);

      connect(d_agg1, 0, d_snk1, 0);
      connect(d_agg1, 1, d_snk1, 1);

      //post mortems
      connect(self(), 0, d_pm_raw, 0);
      connect(self(), 1, d_pm_raw, 1);

      connect(d_agg1000, 0, d_pm_1000, 0);
      connect(d_agg1000, 1, d_pm_1000, 1);

      //output
      connect(d_agg1000, 0, self(), 0);
      connect(d_agg1000, 1, self(), 1);
    }

    cascade_sink_impl::~cascade_sink_impl()
    {
    }

    std::vector<time_domain_sink::sptr>
    cascade_sink_impl::get_time_domain_sinks()
    {
      return {d_snk1, d_snk10, d_snk100, d_snk1000};
    }

    std::vector<post_mortem_sink::sptr>
    cascade_sink_impl::get_post_mortem_sinks()
    {
      return {d_pm_raw, d_pm_1000};
    }

  } /* namespace digitizers */
} /* namespace gr */

