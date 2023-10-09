#ifndef FAIR_HELPERS_HELPER_BLOCKS_HPP
#define FAIR_HELPERS_HELPER_BLOCKS_HPP

#include <gnuradio-4.0/node.hpp>

/**
 * TRANSITIONAL: some simple helpers blocks for tests, should go upstream or
 * replaced by upstream blocks.
 */
namespace fair::helpers {

template<typename T>
struct vector_source : public gr::node<vector_source<T>> {
    gr::PortOut<T> out;

    std::vector<T> data;
    std::size_t    _produced = 0;

    explicit vector_source(std::vector<T> data_) : data{ std::move(data_) } {}

    constexpr std::make_signed_t<std::size_t>
    available_samples(const vector_source &) noexcept {
        const auto v = static_cast<std::make_signed_t<std::size_t>>(data.size() - _produced);
        return v > 0 ? v : -1;
    }

    T
    process_one() noexcept {
        const auto n = _produced;
        _produced++;
        return data[n];
    }
};

template<typename T>
struct vector_sink : public gr::node<vector_sink<T>> {
    gr::PortIn<T>  in;
    std::vector<T> data;

    gr::work_return_status_t
    process_bulk(std::span<const T> input) {
        data.insert(data.end(), input.begin(), input.end());
        return gr::work_return_status_t::OK;
    }
};

template<typename T>
struct tag_debug : public gr::node<tag_debug<T>> {
    gr::PortIn<T>          in;
    gr::PortOut<T>         out;
    std::vector<gr::tag_t> seen_tags;
    std::size_t            samples_seen = 0;

    gr::work_return_status_t
    process_bulk(std::span<const T> input, std::span<T> output) noexcept {
        std::copy(input.begin(), input.end(), output.begin());
        if (this->input_tags_present()) {
            auto tag = this->input_tags()[0];
            tag.index += static_cast<int64_t>(samples_seen);
            seen_tags.push_back(std::move(tag));
        }
        samples_seen += input.size();
        return gr::work_return_status_t::OK;
    }
};

template<typename T>
struct count_sink : public gr::node<count_sink<T>> {
    gr::PortIn<T> in;
    std::size_t   samples_seen = 0;

    gr::work_return_status_t
    process_bulk(std::span<const T> input) noexcept {
        samples_seen += input.size();
        return gr::work_return_status_t::OK;
    }
};

} // namespace fair::helpers

ENABLE_REFLECTION_FOR_TEMPLATE(fair::helpers::vector_source, out, data);
ENABLE_REFLECTION_FOR_TEMPLATE(fair::helpers::vector_sink, in, data);
ENABLE_REFLECTION_FOR_TEMPLATE(fair::helpers::tag_debug, in, out);
ENABLE_REFLECTION_FOR_TEMPLATE(fair::helpers::count_sink, in);

#endif
