#include "stft_goertzl_dynamic_cpu.h"
#include "stft_goertzl_dynamic_cpu_gen.h"

#include <gnuradio/kernel/fft/window.h>
#include <gnuradio/kernel/math/math.h>

namespace gr::digitizers {

template<class T>
stft_goertzl_dynamic_cpu<T>::stft_goertzl_dynamic_cpu(const typename stft_goertzl_dynamic<T>::block_args &args)
    : INHERITED_CONSTRUCTORS(T)
    , d_window_function(kernel::fft::window::build(kernel::fft::window::window_t::HANN, args.winsize, 1.0)) {
    this->set_tag_propagation_policy(tag_propagation_policy_t::TPP_DONT);
}

template<class T>
void stft_goertzl_dynamic_cpu<T>::goertzel(const float *data, const long data_len, float Ts, float frequency, int filter_size, float &real, float &imag) {
    // https://github.com/NaleRaphael/goertzel-ffrequency/blob/master/src/dsp.c
    float    k; // Related to frequency bins
    float    omega;
    float    sine, cosine, coeff, sf;
    float    q0, q1, q2;
    long int i;

    k             = (0.5f + ((float) (filter_size * frequency) * Ts));

    omega         = 2.0f * M_PI * k / (float) filter_size;
    sine          = sin(omega);
    cosine        = cos(omega);
    coeff         = 2.0f * cosine;
    sf            = (float) data_len / 2.0f; // scale factor: for normalization

    q0            = 0.0f;
    q1            = 0.0f;
    q2            = 0.0f;

    long int dlen = data_len - data_len % 3;
    for (i = 0; i < dlen; i += 3) {
        q0 = coeff * q1 - q2 + data[i] * d_window_function[i];
        q2 = coeff * q0 - q1 + data[i + 1] * d_window_function[i + 1];
        q1 = coeff * q2 - q0 + data[i + 2] * d_window_function[i + 2];
    }

    for (; i < data_len; i++) {
        q0 = coeff * q1 - q2 + data[i];
        q2 = q1;
        q1 = q0;
    }

    real = (q1 - q2 * cosine) / sf;
    imag = (q2 * sine) / sf;
}

template<class T>
void stft_goertzl_dynamic_cpu<T>::dft(const float *data, const long data_len, float Ts, float frequency, float &real, float &imag) {
    // legacy implementation - mathematically most correct, but numerically expensive due to sine and cosine computations
    real         = 0.0;
    imag         = 0.0;
    float omega0 = 2.0 * M_PI * frequency * Ts;
    for (int i = 0; i < data_len; i++) {
        float in = data[i] * d_window_function[i];
        real += cos(omega0 * i) * in;
        imag += sin(omega0 * i) * in;
    }
    real /= 0.5 * data_len;
    imag /= 0.5 * data_len;
}

template<class T>
work_return_t stft_goertzl_dynamic_cpu<T>::work(work_io &wio) {
    static_assert(std::is_same<T, float>());
    const auto in          = wio.inputs()[0].items<float>();
    const auto f_min       = wio.inputs()[1].items<float>();
    const auto f_max       = wio.inputs()[2].items<float>();

    auto       mag         = wio.outputs()[0].items<float>();
    auto       phs         = wio.outputs()[1].items<float>();
    auto       fqs         = wio.outputs()[2].items<float>();

    const auto samp_length = 1. / pmtf::get_as<std::size_t>(*this->param_samp_rate);
    const auto winsize     = pmtf::get_as<std::size_t>(*this->param_winsize);
    const auto nbins       = pmtf::get_as<std::size_t>(*this->param_nbins);

    double     f_range     = f_max[0] - f_min[0];
    // printf("noutput_items = %i, d_winsize =%i\n", noutput_items, d_winsize);

    // do frequency analysis for each bin
    for (std::size_t i = 0; i < nbins; i++) {
        double bin_f_range_factor = static_cast<double>(i) / static_cast<double>(nbins - 1);
        double freq               = f_min[0] + (bin_f_range_factor * f_range);

        // Goertzel vs DFT
        if (1) {
            float  w  = 2.0 * M_PI * freq * samp_length;
            double wr = 2.0 * std::cos(w);
            double wi = std::sin(w);
            double d1 = 0.0; // resets for each iteration
            double d2 = 0.0; // resets for each iteration

            // goertzel magic
            for (std::size_t j = 0; j < winsize; j++) {
                // double y = in[j] + wr * d1 - d2;
                double y = in[j] * d_window_function[j] + wr * d1 - d2;
                d2       = d1;
                d1       = y;
            }
            double re = (0.5 * wr * d1 - d2) / winsize;
            double im = (wi * d1) / winsize;

            // transform from carthesian to polar components
            mag[i] = std::sqrt((re * re) + (im * im)); // this usually should be hypotf(re,im) /over-/under-flow  protected/performance
            mag[i] = std::hypotf(re, im);              // faster and over-/under-flow  protected
            phs[i] = gr::kernel::math::fast_atan2f(re, im);
            phs[i] = 0.0;
        } else {
            float re;
            float im;
            if (1) {
                goertzel(in, winsize, samp_length, freq, winsize, re, im);
            } else {
                dft(in, winsize, samp_length, freq, re, im);
            }
            mag[i] = std::hypotf(re, im); // faster and over-/under-flow  protected
            phs[i] = gr::kernel::math::fast_atan2f(re, im);
        }

        // post frequency of each bin
        fqs[i] = freq;
    }

    auto tags = wio.inputs()[0].tags_in_window(0, 1);

    for (auto &tag : tags) {
        tag.set_offset(wio.outputs()[0].nitems_written());
        wio.outputs()[0].add_tag(tag);
    }

    wio.produce_each(1);

    return work_return_t::OK;
}

} /* namespace gr::digitizers */
