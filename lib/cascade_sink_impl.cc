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
              gr::io_signature::make(0,0 , sizeof(float)))
    {

      //std::vector<int> allowed_cores = { 2,3,4 };
      //set_processor_affinity(allowed_cores);

      int samp_rate_to_ten_kilo = static_cast<int>(samp_rate / 10000.0);
      if(samp_rate != (samp_rate_to_ten_kilo * 10000.0)) {
        GR_LOG_ALERT(logger, "SAMPLE RATE NOT DIVISIBLE BY 1000! OUTPUTS NOT EXACT: 10k, 1k, 100, 10, 1 Hz!");
      }

      // create sinks -- FESA will see updates @10Hz at most.
      //                                  signal-name,           unit name, sample rate, dataPackageSize, sink mode
      d_snk10000 = time_domain_sink::make(signal_name+"@10kHz",  unit_name, 10000.0,     1000,            TIME_SINK_MODE_STREAMING);
      d_snk1000  = time_domain_sink::make(signal_name+"@1kHz",   unit_name, 1000.0,       100,            TIME_SINK_MODE_STREAMING);
      d_snk100   = time_domain_sink::make(signal_name+"@100Hz",  unit_name, 100.0,         10,            TIME_SINK_MODE_STREAMING);
      d_snk25    = time_domain_sink::make(signal_name+"@25Hz",   unit_name, 25.0,           1,            TIME_SINK_MODE_STREAMING);
      d_snk10    = time_domain_sink::make(signal_name+"@10Hz",   unit_name, 10.0,           1,            TIME_SINK_MODE_STREAMING);
      d_snk1     = time_domain_sink::make(signal_name+"@1Hz",    unit_name, 1,              1,            TIME_SINK_MODE_STREAMING);

      //create aggregation blocks
      double lf = low_freq; // lower frequency cut-off - decreases by a factor 10 per stage
      double uf = up_freq;  // upper frequency cut-off - decreases by a factor 10 per stage
      double tr = tr_width; // transition width (ie. bandwidth between 3dB and 20 dB point
      // (should not be excessively small <-> relates to the FIR filter length)
      // N.B. original design of hard-coding names... TODO: clean-up and replace by loops for better readability

      // first stage n-MS/S to 10 kS/s
      d_agg10000 = block_aggregation::make(alg_id, samp_rate_to_ten_kilo, delay, fir_taps, lf, uf, tr, fb_user_taps, fw_user_taps, samp_rate);
      // input to first 10 kHz block
      connect(self(), 0, d_agg10000, 0); // 0: values port
      connect(self(), 1, d_agg10000, 1); // 1: errors

      // connect raw->10kHz aggregation to corresponding FESA time-domain sink
      connect(d_agg10000, 0, d_snk10000, 0); // 0: values port
      connect(d_agg10000, 1, d_snk10000, 1); // 1: errors


      // second stage 10 kS/s to 1 kS/s
      lf /= 10;
      uf /= 10;
      tr /= 10;
      d_agg1000  = block_aggregation::make(alg_id, 10,                    delay, fir_taps, lf, uf, tr, fb_user_taps, fw_user_taps, 10000);
      // first 10 kHz block to 1 kHz Block
      connect(d_agg10000, 0, d_agg1000, 0);
      connect(d_agg10000, 1, d_agg1000, 1);
      // connect 1kHz stream to output
      //connect(d_agg1000, 0, self(), 0);
      //connect(d_agg1000, 1, self(), 1);
      // connect 1kHz aggregation to corresponding FESA time-domain sink
      connect(d_agg1000, 0, d_snk1000, 0);
      connect(d_agg1000, 1, d_snk1000, 1);

      // third stage 1 kS/s -> 100 S/s
      lf /= 10;
      uf /= 10;
      tr /= 10;
      d_agg100   = block_aggregation::make(alg_id, 10,                    delay, fir_taps, lf, uf, tr, fb_user_taps, fw_user_taps, 1000);
      // 1 kHz block to 100 Hz Block
      connect(d_agg1000, 0, d_agg100, 0);
      connect(d_agg1000, 1, d_agg100, 1);
      // connect 100 Hz aggregation to corresponding FESA time-domain sink
      connect(d_agg100, 0, d_snk100, 0);
      connect(d_agg100, 1, d_snk100, 1);

      // fourth stage 100 S/s -> 25 S/s -> N.B. alternate continuous update rate @ 25 Hz
      d_agg25    = block_aggregation::make(alg_id,  4,                   delay, fir_taps, lf/4, uf/4, tr/4, fb_user_taps, fw_user_taps, 100);
      // 100 Hz block to 25 Hz Block
      connect(d_agg100, 0, d_agg25, 0);
      connect(d_agg100, 1, d_agg25, 1);
      // connect 25 Hz aggregation to corresponding FESA time-domain sink
      connect(d_agg25, 0, d_snk25, 0);
      connect(d_agg25, 1, d_snk25, 1);

      // fourth stage 100 S/s -> 10 S/s
      lf /= 10;
      uf /= 10;
      tr /= 10;
      d_agg10    = block_aggregation::make(alg_id, 10,                    delay, fir_taps, lf, uf, tr, fb_user_taps, fw_user_taps, 100);
      // 100 Hz block to 10 Hz Block
      connect(d_agg100, 0, d_agg10, 0);
      connect(d_agg100, 1, d_agg10, 1);
      // connect 10 Hz aggregation to corresponding FESA time-domain sink
      connect(d_agg10, 0, d_snk10, 0);
      connect(d_agg10, 1, d_snk10, 1);


      // fifth stage 10 S/s -> 1 S/s (N.B. this slow speed is primarily relevant for super-slow storage rings (e.g. HESR)
      lf /= 10;
      uf /= 10;
      tr /= 10;
      d_agg1     = block_aggregation::make(alg_id, 10,                    delay, fir_taps, lf, uf, tr, fb_user_taps, fw_user_taps, 10);
      // 10 Hz block to 1 Hz Block
      connect(d_agg10, 0, d_agg1, 0);
      connect(d_agg10, 1, d_agg1, 1);
      // connect 1 Hz aggregation to corresponding FESA time-domain sink
      connect(d_agg1, 0, d_snk1, 0);
      connect(d_agg1, 1, d_snk1, 1);


      // To prevent tag explosion we limit the output buffer size. For each output item the aggregation
      // block will generate only one acq_info tag. Therefore number 1024 seems to be reasonable... Note
      // GR works only with output buffers of one page....
//      d_agg10000->set_max_output_buffer(1024);
//      d_agg1000->set_max_output_buffer(1024);
//      d_agg100->set_max_output_buffer(1024);
//      d_agg25->set_max_output_buffer(1024);
//      d_agg10->set_max_output_buffer(1024);
//      d_agg1->set_max_output_buffer(1024);


      // **
      // post-mortem sinks
      // **

      /* FIXME: Post-Mortem Sinks are about 400MB per Cascade in FESA .. too much for something we dont even use yet
      // setup post-mortem data sinks
      d_pm_raw = post_mortem_sink::make(signal_name+":PM@RAW", unit_name, samp_rate, pm_buffer * samp_rate);
      d_pm_1000 = post_mortem_sink::make(signal_name+":PM@10kHz", unit_name, 1000.0f, pm_buffer * 1000.0f);

      // connect raw-rate PM data sinks
      connect(self(), 0, d_pm_raw, 0);
      connect(self(), 1, d_pm_raw, 1);
      // connect 10 kHz PM data sinks
      connect(d_agg1000, 0, d_pm_1000, 0);
      connect(d_agg1000, 1, d_pm_1000, 1);
       */

//      // triggered demux blocks (triggered time-domain acquisition)
//      d_snk_raw_triggered  = time_domain_sink::make(signal_name+":Triggered@Raw",  unit_name, samp_rate, TRIGGER_BUFFER_SIZE_TIME_DOMAIN_FAST, TIME_SINK_MODE_TRIGGERED);
//      d_demux_raw  = demux_ff::make(0.9*TRIGGER_BUFFER_SIZE_TIME_DOMAIN_FAST, 0.1*TRIGGER_BUFFER_SIZE_TIME_DOMAIN_FAST);
//      // input to first raw-data-rate demux
//      connect(self(), 0, d_demux_raw, 0); // 0: values port
//      connect(self(), 1, d_demux_raw, 1); // 1: errors
//      // connect raw-data-rate demux to triggered time-domain sink
//      connect(d_demux_raw, 0, d_snk_raw_triggered, 0); // 0: values port
//      connect(d_demux_raw, 1, d_snk_raw_triggered, 1); // 1: errors

      d_snk10000_triggered = time_domain_sink::make(signal_name+":Triggered@10kHz",  unit_name, 10000.0,   TRIGGER_BUFFER_SIZE_TIME_DOMAIN_SLOW, TIME_SINK_MODE_TRIGGERED);
      d_demux_10000 = demux_ff::make(0.9*TRIGGER_BUFFER_SIZE_TIME_DOMAIN_SLOW, 0.1*TRIGGER_BUFFER_SIZE_TIME_DOMAIN_SLOW);
      // first 10 kHz block to 10 kHz demux
      connect(d_agg10000, 0, d_demux_10000, 0);
      connect(d_agg10000, 1, d_demux_10000, 1);
      // connect 10 kHz demux to triggered time-domain sink
      connect(d_demux_10000, 0, d_snk10000_triggered, 0); // 0: values port
      connect(d_demux_10000, 1, d_snk10000_triggered, 1); // 1: errors

      // **
      // interlock and interlock reference function definition (ref, min, max)
      // **

      // function definition
      //d_interlock_reference_function =  function_ff::make(1);
      // make 1 kHz connection to interlock reference function module (needed to receive timing tags, nothing else)
      //connect(d_agg1000, 0, d_interlock_reference_function, 0);

      //d_snk_interlock_ref = time_domain_sink::make(signal_name+":InterlockRef@1kHz", unit_name, 1000.0,   100, TIME_SINK_MODE_STREAMING);
      //connect(d_interlock_reference_function, 0, d_snk_interlock_ref, 0);

      //d_snk_interlock_min = time_domain_sink::make(signal_name+":InterlockLimitMin@1kHz", unit_name, 1000.0,   100, TIME_SINK_MODE_STREAMING);
      //connect(d_interlock_reference_function, 1, d_snk_interlock_min, 0);

     // d_snk_interlock_max = time_domain_sink::make(signal_name+":InterlockLimitMax@1kHz", unit_name, 1000.0,   100, TIME_SINK_MODE_STREAMING);
      //connect(d_interlock_reference_function, 2, d_snk_interlock_max, 0);


      // arbitrary initial interlock limits since they are anyway overwritten by the reference function
      //d_interlock =  interlock_generation_ff::make(-10000.0, +10000.0);
      //d_snk_interlock = time_domain_sink::make(signal_name+":InterlockState@1kHz", unit_name, 1000.0,   100, TIME_SINK_MODE_STREAMING);
      //connect(d_interlock, 0, d_snk_interlock, 0);

      // connect 10 kHz block to interlock port 0 ('sig')
      //connect(d_agg1000, 0, d_interlock, 0);

      // connect interlock reference function min to interlock port 1 ('min')
      //connect(d_interlock_reference_function, 1, d_interlock, 1);

      // connect interlock reference function max to interlock port 2 ('max')
      //connect(d_interlock_reference_function, 2, d_interlock, 2);

      // block definition for frequency-domain sinks
      // setup ST-Fourier Trafo blocks
      //int wintype = filter::firdes::win_type::WIN_BLACKMAN;
      // setup demux blocks - default 10% for pre- and 90% of samples for post-trigger samples

      // **
      // frequency-domain -type acquisition
      // **

      /* FIXME: Cascase Triggered Freq.Sinks dont need much extra memory, however they simply are not visible/usable in FESA currently .. to be investigated why
      // triggered frequency domain sinks
      d_freq_snk_triggered    = freq_sink_f::make(signal_name+":TriggeredSpectrum@Raw",    SAMPLE_RATE_TRIGGERED_FREQ_SINK, WINDOW_SIZE_FREQ_DOMAIN_FAST, N_BUFFERS, 1, FREQ_SINK_MODE_TRIGGERED);
      d_demux_freq_raw  = demux_ff::make(samp_rate, MIN_HISTORY * samp_rate, WINDOW_SIZE_FREQ_DOMAIN_FAST, 0);
      // connect raw-data-rate demux to STFT and then frequency-domain sink
      connect(self(), 0, d_demux_freq_raw, 0); // 0: values port
      connect(self(), 1, d_demux_freq_raw, 1); // 1: errors
      stft_algorithms::sptr stft_raw_triggered = stft_algorithms::make(samp_rate, 0.001, WINDOW_SIZE_FREQ_DOMAIN_FAST, wintype, FFT, 0, samp_rate/2, WINDOW_SIZE_FREQ_DOMAIN_FAST);
      stft_raw_triggered->set_block_alias("stft_raw_triggered<"+signal_name+">");
      connect(d_demux_freq_raw, 0, stft_raw_triggered, 0);
      // connect(d_demux_freq_raw, 1, stft_raw_triggered, 1); // 'err' input does not exist yet
      connect(stft_raw_triggered, 0, d_freq_snk_triggered, 0); // amplitude input
      connect(stft_raw_triggered, 1, d_freq_snk_triggered, 1); // phase input
      connect(stft_raw_triggered, 2, d_freq_snk_triggered, 2); // frequency inputs

      d_freq_snk10k_triggered = freq_sink_f::make(signal_name+":TriggeredSpectrum@10kHz", SAMPLE_RATE_TRIGGERED_FREQ_SINK, WINDOW_SIZE_FREQ_DOMAIN_SLOW, N_BUFFERS, 1, FREQ_SINK_MODE_TRIGGERED);
      d_demux_freq_10k = demux_ff::make(10000.0f , MIN_HISTORY * 10000.0f, WINDOW_SIZE_FREQ_DOMAIN_SLOW, 0);
      // connect 10 kHz freq demux to STFT and then frequency-domain sink
      connect(d_agg10000, 0, d_demux_freq_10k, 0);
      connect(d_agg10000, 1, d_demux_freq_10k, 1);
      stft_algorithms::sptr stft_10k_triggered = stft_algorithms::make(10000.0f,  0.001, WINDOW_SIZE_FREQ_DOMAIN_SLOW, wintype, FFT, 0, samp_rate/2, WINDOW_SIZE_FREQ_DOMAIN_FAST);
      stft_10k_triggered->set_block_alias("stft_10k_triggered<"+signal_name+">");
      connect(d_demux_freq_10k, 0, stft_10k_triggered, 0);
      // connect(d_demux_freq_10k, 1, stft_10k_triggered, 1); // 'err' input does not exist yet
      connect(stft_10k_triggered, 0, d_freq_snk10k_triggered, 0); // amplitude input
      connect(stft_10k_triggered, 1, d_freq_snk10k_triggered, 1); // phase input
      connect(stft_10k_triggered, 2, d_freq_snk10k_triggered, 2); // frequency inputs
      */


      /* FIXME: 2200MB per Cascade in FESA .. disabled for now. To be checked if caused by misconfiguration
      // connect streaming aggregated data to stft block and subsequently to frequency sink
      // N.B. sampling frequency is always 10 kHz, but the window function is updated at a reduced rate
      // streaming-mode frequency domain sinks
      d_freq_snk1000 = freq_sink_f::make(signal_name+":Spectrum@1kHz", 1000.0, WINDOW_SIZE_FREQ_DOMAIN_SLOW, N_BUFFERS, 10, FREQ_SINK_MODE_STREAMING);
      d_freq_snk25   = freq_sink_f::make(signal_name+":Spectrum@25Hz",   25.0, WINDOW_SIZE_FREQ_DOMAIN_SLOW, N_BUFFERS, 1, FREQ_SINK_MODE_STREAMING);
      d_freq_snk10   = freq_sink_f::make(signal_name+":Spectrum@10Hz",   10.0, WINDOW_SIZE_FREQ_DOMAIN_SLOW, N_BUFFERS, 1, FREQ_SINK_MODE_STREAMING);

      // Short-Term Fourier Transform with fs=10 kHz und 1 kHz update rate
      stft_algorithms::sptr stft_1k = stft_algorithms::make(10000.0f,  0.001, WINDOW_SIZE_FREQ_DOMAIN_SLOW, wintype, FFT, 0, samp_rate/2, WINDOW_SIZE_FREQ_DOMAIN_SLOW);
      stft_1k->set_block_alias("stft_1k<"+signal_name+">");
      connect(d_agg10000, 0, stft_1k, 0);
      connect(stft_1k, 0, d_freq_snk1000, 0); // amplitude input
      connect(stft_1k, 1, d_freq_snk1000, 1); // phase input
      connect(stft_1k, 2, d_freq_snk1000, 2); // frequency inputs

      // Short-Term Fourier Transform with fs=10 kHz und 25 Hz update rate
      stft_algorithms::sptr stft_25 = stft_algorithms::make(10000.0f,  0.04, WINDOW_SIZE_FREQ_DOMAIN_SLOW, wintype, FFT, 0, samp_rate/2, WINDOW_SIZE_FREQ_DOMAIN_SLOW);
      stft_25->set_block_alias("stft_25<"+signal_name+">");
      connect(d_agg10000, 0, stft_25, 0);
      connect(stft_25, 0, d_freq_snk25, 0); // amplitude input
      connect(stft_25, 1, d_freq_snk25, 1); // phase input
      connect(stft_25, 2, d_freq_snk25, 2); // frequency inputs

      // Short-Term Fourier Transform with fs=10 kHz und 10 Hz update rate
      stft_algorithms::sptr stft_10 = stft_algorithms::make(10000.0f,  0.1, WINDOW_SIZE_FREQ_DOMAIN_SLOW, wintype, FFT, 0, samp_rate/2, WINDOW_SIZE_FREQ_DOMAIN_SLOW);
      stft_10->set_block_alias("stft_10<"+signal_name+">");
      connect(d_agg10000, 0, stft_10, 0);
      connect(stft_10, 0, d_freq_snk10, 0); // amplitude input
      connect(stft_10, 1, d_freq_snk10, 1); // phase input
      connect(stft_10, 2, d_freq_snk10, 2); // frequency inputs
      */

    }

    cascade_sink_impl::~cascade_sink_impl()
    {
    }

    std::vector<time_domain_sink::sptr>
    cascade_sink_impl::get_time_domain_sinks()
    {
      return {d_snk1, d_snk10, d_snk25, d_snk100, d_snk1000, d_snk10000, d_snk10000_triggered};
      //return {d_snk1, d_snk10, d_snk25, d_snk100, d_snk1000, d_snk10000, d_snk_raw_triggered, d_snk10000_triggered};
      //return {d_snk1, d_snk10, d_snk25, d_snk100, d_snk1000, d_snk10000};
      //return {d_snk1000, d_snk10000};
      //return {d_snk1, d_snk10, d_snk25};
    }

    std::vector<post_mortem_sink::sptr>
    cascade_sink_impl::get_post_mortem_sinks()
    {
      return {};
      //return {d_pm_raw, d_pm_1000};
    }

    std::vector<freq_sink_f::sptr>
    cascade_sink_impl::get_frequency_domain_sinks()
    {
     // return {d_freq_snk1000, d_freq_snk25, d_freq_snk10, d_freq_snk_triggered, d_freq_snk10k_triggered};
     // return {d_freq_snk1000, d_freq_snk25, d_freq_snk10};
     // return {d_freq_snk_triggered, d_freq_snk10k_triggered};
        return {};
    }

    std::vector<function_ff::sptr>
    cascade_sink_impl::get_reference_function_blocks()
    {
      return {};
      //return {d_interlock_reference_function};
    }

  } /* namespace digitizers */
} /* namespace gr */

