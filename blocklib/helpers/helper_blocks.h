#ifndef GR_HELPERS_HELPER_BLOCKS_H
#define GR_HELPERS_HELPER_BLOCKS_H

#include <node.hpp>

/**
 * TRANSITIONAL: some simple helpers blocks for tests, should go upstream or
 * replaced by upstream blocks.
 */
namespace gr::helpers {

template <typename T>
struct vector_source : public fair::graph::node<vector_source<T>> {
    fair::graph::OUT<T> out;

    std::vector<T> data;
    std::size_t _produced = 0;

    explicit vector_source(std::vector<T> data_) : data{ std::move(data_) } {}

    constexpr std::make_signed_t<std::size_t>
    available_samples(const vector_source&) noexcept
    {
        const auto v =
            static_cast<std::make_signed_t<std::size_t>>(data.size()) - _produced;
        return v > 0 ? v : -1;
    }

    T process_one() noexcept
    {
        const auto n = _produced;
        _produced++;
        return data[n];
    }
};

template <typename T>
struct vector_sink : public fair::graph::node<vector_sink<T>> {
    fair::graph::IN<T> in;
    std::vector<T> data;

    fair::graph::work_return_status_t process_bulk(std::span<const T> input) {
        data.insert(data.end(), input.begin(), input.end());
        return fair::graph::work_return_status_t::OK;
    }
};

template <typename T>
struct count_sink : public fair::graph::node<count_sink<T>> {
    fair::graph::IN<T> in;
    std::size_t samples_seen = 0;

    fair::graph::work_return_status_t process_bulk(std::span<const T> input) noexcept {
        samples_seen += input.size();
        return fair::graph::work_return_status_t::OK;
    }
};

}

ENABLE_REFLECTION_FOR_TEMPLATE(gr::helpers::vector_source, out, data);
ENABLE_REFLECTION_FOR_TEMPLATE(gr::helpers::vector_sink, in, data);
ENABLE_REFLECTION_FOR_TEMPLATE(gr::helpers::count_sink, in);

#endif
