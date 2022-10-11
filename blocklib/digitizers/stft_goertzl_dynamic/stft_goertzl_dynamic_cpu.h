#pragma once

#include <gnuradio/digitizers/stft_goertzl_dynamic.h>

namespace gr::digitizers {

template<class T>
class stft_goertzl_dynamic_cpu : public stft_goertzl_dynamic<T> {
public:
    stft_goertzl_dynamic_cpu(const typename stft_goertzl_dynamic<T>::block_args &args);

    work_return_t work(work_io &wio) override;

private:
    void               goertzel(const float *data, const long data_len, float Ts, float frequency, int filter_size, float &real, float &imag);
    void               dft(const float *data, const long data_len, float Ts, float frequency, float &real, float &imag);

    std::vector<float> d_window_function;
};

} // namespace gr::digitizers
