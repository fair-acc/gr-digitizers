#include "freq_estimator_cpu.h"
#include "freq_estimator_cpu_gen.h"

namespace gr::digitizers {

template<class T>
freq_estimator_cpu<T>::freq_estimator_cpu(const typename freq_estimator<T>::block_args &args)
    : INHERITED_CONSTRUCTORS(T)
    , d_sig_avg(args.signal_window_size)
    , d_freq_avg(args.averager_window_size)
    , d_counter(args.decim) {
}

template<class T>
work_return_t freq_estimator_cpu<T>::work(work_io &wio) {
    static_assert(std::is_same<T, float>());

    const int  n_in  = wio.inputs()[0].n_items;
    int        n_out = 0;
    const auto in    = wio.inputs()[0].items<float>();
    auto       out   = wio.outputs()[0].items<float>();

    float      new_sig_avg;

    const auto samp_rate = pmtf::get_as<float>(*this->param_samp_rate);
    const auto decim     = pmtf::get_as<int>(*this->param_decim);

    for (int i = 0; i < n_in; i++) {
        // average the signal to get rid of noise
        new_sig_avg = d_sig_avg.add(in[i]);

        d_prev_zero_dist += 1;

        // previous running average is differently signed as this one -> signal went through zero
        if (!((new_sig_avg < 0.0 && d_old_sig_avg < 0.0) || (new_sig_avg >= 0.0 && d_old_sig_avg >= 0.0))) {
            // interpolate where the averaged signal passed through zero
            double x = (-new_sig_avg) / (new_sig_avg - d_old_sig_avg);

            // estimate of the frequency is an inverse of the distances between zero values.
            d_avg_freq = d_freq_avg.add(samp_rate / (2.0 * (d_prev_zero_dist + x)));

            // starter offset for next estimate.
            d_prev_zero_dist = -x;
        }
        // for the next iteration
        d_old_sig_avg = new_sig_avg;

        // decimate and post
        d_counter--;
        if (d_counter <= 0) {
            d_counter  = decim;
            out[n_out] = d_avg_freq;
            n_out++;
        }
    }

    // add tags with corrected offset to the output stream
    auto tags = wio.inputs()[0].tags_in_window(0, n_in);
    for (auto &tag : tags) {
        if (decim != 0) {
            tag.set_offset(tag.offset() / decim);
        }
        wio.outputs()[0].add_tag(tag);
    }

    wio.consume_each(n_in);
    wio.produce_each(n_out);

    return work_return_t::OK;
}

} /* namespace gr::digitizers */
