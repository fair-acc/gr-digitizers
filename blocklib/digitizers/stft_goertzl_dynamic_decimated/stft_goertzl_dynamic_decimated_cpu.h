#pragma once

#include <gnuradio/digitizers/stft_goertzl_dynamic.h>
#include <gnuradio/digitizers/stft_goertzl_dynamic_decimated.h>
#include <gnuradio/digitizers/stream_to_vector_overlay.h>

namespace gr::digitizers {

class stft_goertzl_dynamic_decimated_cpu : public stft_goertzl_dynamic_decimated {
public:
    explicit stft_goertzl_dynamic_decimated_cpu(const block_args &args);

    void on_parameter_change(param_action_sptr action) override;

private:
    stft_goertzl_dynamic::sptr     d_stft;
    stream_to_vector_overlay::sptr d_str2vec_sig;
    stream_to_vector_overlay::sptr d_str2vec_min;
    stream_to_vector_overlay::sptr d_str2vec_max;
};

} // namespace gr::digitizers
