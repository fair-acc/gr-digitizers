#include "stft_goertzl_dynamic_decimated_cpu.h"
#include "stft_goertzl_dynamic_decimated_cpu_gen.h"

namespace gr::digitizers {

stft_goertzl_dynamic_decimated_cpu::stft_goertzl_dynamic_decimated_cpu(
    const block_args& args)
    : INHERITED_CONSTRUCTORS
{
    const double samp_rate_decimated = args.samp_rate / args.bounds_decimation;
    d_str2vec_sig = stream_to_vector_overlay::make(
        { args.window_size, args.samp_rate, args.delta_t });
    d_str2vec_min =
        stream_to_vector_overlay::make({ 1, samp_rate_decimated, args.delta_t });
    d_str2vec_max =
        stream_to_vector_overlay::make({ 1, samp_rate_decimated, args.delta_t });
    d_stft = stft_goertzl_dynamic::make({ args.samp_rate, args.window_size, args.nbins });

    connect(self(), 0, d_str2vec_sig, 0);
    connect(self(), 1, d_str2vec_min, 0);
    connect(self(), 2, d_str2vec_max, 0);
    connect(d_str2vec_sig, 0, d_stft, 0);
    connect(d_str2vec_min, 0, d_stft, 1);
    connect(d_str2vec_max, 0, d_stft, 2);

    connect(d_stft, 0, self(), 0);
    connect(d_stft, 1, self(), 1);
    connect(d_stft, 2, self(), 2);
}

void stft_goertzl_dynamic_decimated_cpu::on_parameter_change(param_action_sptr action)
{
    hier_block::on_parameter_change(action);

    if (action->id() == id_samp_rate) {
        d_stft->set_samp_rate(pmtf::get_as<double>(*this->param_samp_rate));
    }
}

} // namespace gr::digitizers
