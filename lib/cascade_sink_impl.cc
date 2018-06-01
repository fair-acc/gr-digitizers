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

      int samp_rate_to_ten_kilo = static_cast<int>(samp_rate / 10000.0);
      if(samp_rate != (samp_rate_to_ten_kilo * 10000.0)) {
        GR_LOG_ALERT(logger, "SAMPLE RATE NOT DIVISIBLE BY 1000! OUTPUTS NOT EXACT: 10k, 1k, 100, 10, 1 Hz!");
      }

      //create blocks
      double lf = low_freq; // lower frequency cut-off - decreases by a factor 10 per stage
      double uf = up_freq;  // upper frequency cut-off - decreases by a factor 10 per stage
      double tr = tr_width; // transition width (ie. bandwidth between 3dB and 20 dB point
      // (should not be excessively small <-> relates to the FIR filter length)
      // N.B. original design of hard-coding names... TODO: clean-up and replace by loops for better readability

      // first stage n-MS/S to 10 kS/s
      d_agg10000 = block_aggregation::make(alg_id, samp_rate_to_ten_kilo, delay, fir_taps, lf, uf, tr, fb_user_taps, fw_user_taps, samp_rate);

      // second stage 10 kS/s to 1 kS/s
      lf /= 10;
      uf /= 10;
      tr /= 10;
      d_agg1000  = block_aggregation::make(alg_id, 10,                    delay, fir_taps, lf, uf, tr, fb_user_taps, fw_user_taps, 10000);

      // third stage 1 kS/s -> 100 S/s
      lf /= 10;
      uf /= 10;
      tr /= 10;
      d_agg100   = block_aggregation::make(alg_id, 10,                    delay, fir_taps, lf, uf, tr, fb_user_taps, fw_user_taps, 1000);

      // fourth stage 100 S/s -> 25 S/s -> N.B. alternate continuous update rate @ 25 Hz
      d_agg25    = block_aggregation::make(alg_id, 10,                    delay, fir_taps, lf/4, uf/4, tr/4, fb_user_taps, fw_user_taps, 25);

      // fourth stage 100 S/s -> 10 S/s
      lf /= 10;
      uf /= 10;
      tr /= 10;
      d_agg10    = block_aggregation::make(alg_id, 10,                    delay, fir_taps, lf, uf, tr, fb_user_taps, fw_user_taps, 100);


      // fifth stage 10 S/s -> 1 S/s (N.B. this slow speed is primarily relevant for super-slow storage rings (e.g. HESR)
      lf /= 10;
      uf /= 10;
      tr /= 10;
      d_agg1     = block_aggregation::make(alg_id, 10,                    delay, fir_taps, lf, uf, tr, fb_user_taps, fw_user_taps, 10);

      // FESA will see updates @10Hz at most.
      d_snk10000 = time_domain_sink::make(signal_name+"@10kHz",  unit_name, 10000.0, 1000, 10, TIME_SINK_MODE_STREAMING);
      d_snk1000  = time_domain_sink::make(signal_name+"@1kHz",   unit_name, 1000.0,   100, 10, TIME_SINK_MODE_STREAMING);
      d_snk100   = time_domain_sink::make(signal_name+"@100Hz",  unit_name, 100.0,     10, 10, TIME_SINK_MODE_STREAMING);
      d_snk25    = time_domain_sink::make(signal_name+"@25Hz",   unit_name, 25.0,       1, 10, TIME_SINK_MODE_STREAMING);
      d_snk10    = time_domain_sink::make(signal_name+"@10Hz",   unit_name, 10.0,       1, 10, TIME_SINK_MODE_STREAMING);
      d_snk1     = time_domain_sink::make(signal_name+"@1Hz",    unit_name, 1,          1,  2, TIME_SINK_MODE_STREAMING);

      // To prevent tag explosion we limit the output buffer size. For each output item the aggregation
      // block will generate only one acq_info tag. Therefore number 1024 seems to be reasonable... Note
      // GR works only with output buffers of one page....
      d_agg10000->set_max_output_buffer(1024);
      d_agg1000->set_max_output_buffer(1024);
      d_agg100->set_max_output_buffer(1024);
      d_agg25->set_max_output_buffer(1024);
      d_agg10->set_max_output_buffer(1024);
      d_agg1->set_max_output_buffer(1024);

      d_pm_raw = post_mortem_sink::make(signal_name+":PM@RAW", unit_name, samp_rate, pm_buffer * samp_rate);
      d_pm_1000 = post_mortem_sink::make(signal_name+":PM@10kHz", unit_name, 1000.0f, pm_buffer * 1000.0f);

      // ***
      // connect block cascade
      // ***
      // input to first 10 kHz block
      connect(self(), 0, d_agg10000, 0); // 0: values port
      connect(self(), 1, d_agg10000, 1); // 1: errors

      // first 10 kHz block to 1 kHz Block
      connect(d_agg10000, 0, d_agg1000, 0);
      connect(d_agg10000, 1, d_agg1000, 1);

      // 1 kHz block to 100 Hz Block
      connect(d_agg1000, 0, d_agg100, 0);
      connect(d_agg1000, 1, d_agg100, 1);

      // 100 Hz block to 10 Hz Block
      connect(d_agg100, 0, d_agg10, 0);
      connect(d_agg100, 1, d_agg10, 1);

      // 100 Hz block to 25 Hz Block
      connect(d_agg100, 0, d_agg25, 0);
      connect(d_agg100, 1, d_agg25, 1);

      // 10 Hz block to 1 Hz Block
      connect(d_agg10, 0, d_agg1, 0);
      connect(d_agg10, 1, d_agg1, 1);

      // ***
      // connect blocks to corresponding FESA sink interfaces
      // ***
      connect(d_agg10000, 0, d_snk10000, 0);
      connect(d_agg10000, 1, d_snk10000, 1);

      connect(d_agg1000, 0, d_snk1000, 0);
      connect(d_agg1000, 1, d_snk1000, 1);

      connect(d_agg100, 0, d_snk100, 0);
      connect(d_agg100, 1, d_snk100, 1);

      connect(d_agg25, 0, d_snk25, 0);
      connect(d_agg25, 1, d_snk25, 1);

      connect(d_agg10, 0, d_snk10, 0);
      connect(d_agg10, 1, d_snk10, 1);

      connect(d_agg1, 0, d_snk1, 0);
      connect(d_agg1, 1, d_snk1, 1);

      // post-mortem sinks
      connect(self(), 0, d_pm_raw, 0);
      connect(self(), 1, d_pm_raw, 1);

      connect(d_agg1000, 0, d_pm_1000, 0);
      connect(d_agg1000, 1, d_pm_1000, 1);

      // output
      connect(d_agg1000, 0, self(), 0);
      connect(d_agg1000, 1, self(), 1);


      // interlock and interlock reference function definition (ref, min, max)

      // function definition
      d_interlock_reference_function =  function_ff::make(0);
      // make 1 kHz connection to interlock reference function module (needed to receive timing tags, nothing else)
      connect(d_agg1000, 0, d_interlock_reference_function, 0);

      d_snk_interlock_ref = time_domain_sink::make(signal_name+":InterlockRef@1kHz", unit_name, 1000.0,   100, 10, TIME_SINK_MODE_STREAMING);
      connect(d_interlock_reference_function, 0, d_snk_interlock_ref, 0);

      d_snk_interlock_min = time_domain_sink::make(signal_name+":InterlockLimitMin@1kHz", unit_name, 1000.0,   100, 10, TIME_SINK_MODE_STREAMING);
      connect(d_interlock_reference_function, 1, d_snk_interlock_min, 0);

      d_snk_interlock_max = time_domain_sink::make(signal_name+":InterlockLimitMax@1kHz", unit_name, 1000.0,   100, 10, TIME_SINK_MODE_STREAMING);
      connect(d_interlock_reference_function, 2, d_snk_interlock_max, 0);


      // arbitrary initial interlock limits since they are anyway overwritten by the reference function
      d_interlock =  interlock_generation_ff::make(-10000.0, +10000.0);
      d_snk_interlock = time_domain_sink::make(signal_name+":InterlockState@1kHz", unit_name, 1000.0,   100, 10, TIME_SINK_MODE_STREAMING);
      connect(d_interlock, 0, d_snk_interlock, 0);

      // connect 10 kHz block to interlock port 0 ('sig')
      connect(d_agg1000, 0, d_interlock, 0);

      // connect interlock reference function min to interlock port 1 ('min')
      connect(d_interlock_reference_function, 1, d_interlock, 1);

      // connect interlock reference function max to interlock port 2 ('max')
      connect(d_interlock_reference_function, 2, d_interlock, 2);


      // TODO: add block definition for frequency-domain sinks
      // N.B. three sinks:
      // spectra sampled sampled at input rate and notified on triggered acquisition
      // spectra sampled sampled at 10 kS/s with update rates at 1 kHz, 25 Hz, and 10 Hz

    }

    cascade_sink_impl::~cascade_sink_impl()
    {
    }

    std::vector<time_domain_sink::sptr>
    cascade_sink_impl::get_time_domain_sinks()
    {
      return {d_snk1, d_snk10, d_snk25, d_snk100, d_snk1000, d_snk10000};
    }

    std::vector<post_mortem_sink::sptr>
    cascade_sink_impl::get_post_mortem_sinks()
    {
      return {d_pm_raw, d_pm_1000};
    }

    std::vector<freq_sink_f::sptr>
    cascade_sink_impl::get_frequency_domain_sinks()
    {
      return {};
    }

    std::vector<function_ff::sptr>
    cascade_sink_impl::get_reference_function_blocks()
    {
      return {d_interlock_reference_function};
    }

  } /* namespace digitizers */
} /* namespace gr */

