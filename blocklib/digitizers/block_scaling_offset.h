#ifndef GR_DIGITIZERS_BLOCK_SCALING_OFFSET_H
#define GR_DIGITIZERS_BLOCK_SCALING_OFFSET_H

#include <node.hpp>

namespace gr::digitizers {

/**
 * Scales and offsets the input signal. All values are passed through. Scaling and offset
 * parameters are settable.
 *
 * Accepts two inputs and produces two outputs. the first input gets scaled and offsetted,
 * while the second input is only scaled.
 */
template <typename T>
struct block_scaling_offset : public fair::graph::node<block_scaling_offset<T>> {
    fair::graph::IN<T> in_signal;
    fair::graph::IN<T> in_error;
    fair::graph::OUT<T> out_signal;
    fair::graph::OUT<T> out_error;
    double scale = 1.;
    double offset = 0.;

    explicit block_scaling_offset(double scale_, double offset_)
        : scale(scale_), offset(offset_)
    {
    }

    std::tuple<T, T> process_one(T sig, T error) noexcept
    {
        return { static_cast<T>(sig * scale - offset), static_cast<T>(error * scale) };
    }
};

} // namespace gr::digitizers

ENABLE_REFLECTION_FOR_TEMPLATE(gr::digitizers::block_scaling_offset,
                               in_signal,
                               in_error,
                               out_signal,
                               out_error,
                               scale,
                               offset);

#endif
