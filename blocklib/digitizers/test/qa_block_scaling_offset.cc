#include <boost/ut.hpp>

#include <block_scaling_offset.h>

#include <scheduler.hpp>

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

ENABLE_REFLECTION_FOR_TEMPLATE(vector_source, out, data);

template <typename T>
struct vector_sink : public fair::graph::node<vector_sink<T>> {
    fair::graph::IN<T> in;
    std::vector<T> data;

    void process_one(T v) { data.push_back(v); }
};

ENABLE_REFLECTION_FOR_TEMPLATE(vector_sink, in, data);

namespace gr::digitizers::block_scaling_offset_test {

const boost::ut::suite BlockScalingOffsetTests = [] {
    using namespace boost::ut;
    using namespace fair::graph;
    using namespace gr::digitizers;

    "scale and offset"_test = [] {
        double scale = 1.5;
        double offset = 2;
        int n = 30;
        std::vector<float> data;
        for (int i = 0; i < n; i++) {
            data.push_back(i);
        }

        graph flow_graph;

        auto& src0 = flow_graph.make_node<vector_source<float>>(data);
        auto& src1 = flow_graph.make_node<vector_source<float>>(data);
        auto& snk0 = flow_graph.make_node<vector_sink<float>>();
        auto& snk1 = flow_graph.make_node<vector_sink<float>>();
        auto& bso = flow_graph.make_node<block_scaling_offset<float>>(scale, offset);

        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"out">(src0).template to<"in_signal">(bso)));
        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"out">(src1).template to<"in_error">(bso)));
        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"out_signal">(bso).template to<"in">(snk0)));
        expect(eq(connection_result_t::SUCCESS,
                  flow_graph.connect<"out_error">(bso).template to<"in">(snk1)));

        scheduler::simple sched{ std::move(flow_graph) };
        sched.run_and_wait();

        auto result0 = snk0.data;
        auto result1 = snk1.data;
        expect(eq(data.size(), result0.size()));
        expect(eq(data.size(), result1.size()));

        for (int i = 0; i < n; i++) {
            float should_be0 = ((data.at(i) * scale) - offset);
            float should_be1 = (data.at(i) * scale);
            expect(should_be0 == result0.at(i)) << "epsilon=0.0001";
            expect(should_be1 == result1.at(i)) << "epsilon=0.0001";
        }
    };
};

} // namespace gr::digitizers::block_scaling_offset_test

int main()
{ /* tests are statically executed */
}
