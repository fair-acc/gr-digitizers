#ifndef FAIR_HELPERS_HELPER_BLOCKS_HPP
#define FAIR_HELPERS_HELPER_BLOCKS_HPP

#include <gnuradio-4.0/node.hpp>

/**
 * TRANSITIONAL: some simple helpers blocks for tests, should go upstream or
 * replaced by upstream blocks.
 */
namespace fair::helpers {

template<typename T>
struct VectorSource : public gr::node<VectorSource<T>> {
    gr::PortOut<T> out;

    std::vector<T> data;
    std::size_t    _produced = 0;

    explicit VectorSource(std::vector<T> data_) : data{ std::move(data_) } {}

    constexpr std::make_signed_t<std::size_t>
    available_samples(const VectorSource &) noexcept {
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
struct VectorSink : public gr::node<VectorSink<T>> {
    gr::PortIn<T>  in;
    std::vector<T> data;

    gr::work_return_status_t
    process_bulk(std::span<const T> input) {
        data.insert(data.end(), input.begin(), input.end());
        return gr::work_return_status_t::OK;
    }
};

template<typename T>
struct TagDebug : public gr::node<TagDebug<T>> {
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
struct CountSink : public gr::node<CountSink<T>> {
    gr::PortIn<T> in;
    std::size_t   samples_seen = 0;

    gr::work_return_status_t
    process_bulk(std::span<const T> input) noexcept {
        samples_seen += input.size();
        return gr::work_return_status_t::OK;
    }
};

} // namespace fair::helpers

ENABLE_REFLECTION_FOR_TEMPLATE(fair::helpers::VectorSource, out, data);
ENABLE_REFLECTION_FOR_TEMPLATE(fair::helpers::VectorSink, in, data);
ENABLE_REFLECTION_FOR_TEMPLATE(fair::helpers::TagDebug, in, out);
ENABLE_REFLECTION_FOR_TEMPLATE(fair::helpers::CountSink, in);

#endif
