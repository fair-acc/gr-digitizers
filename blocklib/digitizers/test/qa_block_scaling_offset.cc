#include <boost/ut.hpp>

#include <block_scaling_offset.hpp>
#include <helper_blocks.hpp>

#include <scheduler.hpp>

namespace gr::digitizers::block_scaling_offset_test {

const boost::ut::suite BlockScalingOffsetTests = [] {
    using namespace boost::ut;
    using namespace fair::graph;
    using namespace gr::digitizers;
    using namespace gr::helpers;

    "scale and offset"_test = [] {
        constexpr float       scale  = 1.5;
        constexpr float       offset = 2;
        constexpr std::size_t n      = 30;
        std::vector<float>    data(n);
        std::iota(data.begin(), data.end(), 0);

        graph flow_graph;

        auto &src0 = flow_graph.make_node<vector_source<float>>(data);
        auto &src1 = flow_graph.make_node<vector_source<float>>(data);
        auto &snk0 = flow_graph.make_node<vector_sink<float>>();
        auto &snk1 = flow_graph.make_node<vector_sink<float>>();
        auto &bso  = flow_graph.make_node<block_scaling_offset<float>>({ { { "scale", scale }, { "offset", offset } } });

        expect(eq(connection_result_t::SUCCESS, flow_graph.connect<"out">(src0).template to<"in_signal">(bso)));
        expect(eq(connection_result_t::SUCCESS, flow_graph.connect<"out">(src1).template to<"in_error">(bso)));
        expect(eq(connection_result_t::SUCCESS, flow_graph.connect<"out_signal">(bso).template to<"in">(snk0)));
        expect(eq(connection_result_t::SUCCESS, flow_graph.connect<"out_error">(bso).template to<"in">(snk1)));

        scheduler::simple sched{ std::move(flow_graph) };
        sched.run_and_wait();

        expect(eq(data.size(), snk0.data.size()));
        expect(eq(data.size(), snk1.data.size()));

        for (std::size_t i = 0; i < n; i++) {
            expect(data[i] * scale - offset == snk0.data[i]) << "epsilon=0.0001";
            expect(data[i] * scale == snk1.data[i]) << "epsilon=0.0001";
        }
    };
};

} // namespace gr::digitizers::block_scaling_offset_test

int
main() { /* tests are statically executed */
}
