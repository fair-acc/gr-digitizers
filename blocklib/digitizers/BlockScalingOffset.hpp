#ifndef GR_DIGITIZERS_BLOCK_SCALING_OFFSET_HPP
#define GR_DIGITIZERS_BLOCK_SCALING_OFFSET_HPP

#include <gnuradio-4.0/Block.hpp>

namespace gr::digitizers {

/**
 * Scales and offsets the input signal. All values are passed through. Scaling and offset
 * parameters are settable.
 *
 * Accepts two inputs and produces two outputs. the first input gets scaled and offsetted,
 * while the second input is only scaled.
 */
template<typename T>
struct BlockScalingOffset : public gr::Block<BlockScalingOffset<T>> {
    gr::PortIn<T>                           in_signal;
    gr::PortIn<T>                           in_error;
    gr::PortOut<T>                          out_signal;
    gr::PortOut<T>                          out_error;
    gr::Annotated<T, "scale", gr::Visible>  scale  = 1.;
    gr::Annotated<T, "offset", gr::Visible> offset = 0.;

    std::tuple<T, T> processOne(T sig, T error) noexcept { return {static_cast<T>(sig * scale - offset), static_cast<T>(error * scale)}; }
};

} // namespace gr::digitizers

ENABLE_REFLECTION_FOR_TEMPLATE(gr::digitizers::BlockScalingOffset, in_signal, in_error, out_signal, out_error, scale, offset);

#endif
