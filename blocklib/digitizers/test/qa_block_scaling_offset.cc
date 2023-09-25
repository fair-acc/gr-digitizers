#include <boost/ut.hpp>

#include <block_scaling_offset.h>
#include <helper_blocks.h>

#include <scheduler.hpp>

namespace gr::digitizers::block_scaling_offset_test {

const boost::ut::suite BlockScalingOffsetTests = [] {
    using namespace boost::ut;
    using namespace fair::graph;
    using namespace gr::digitizers;
    using namespace gr::helpers;

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
        auto& bso = flow_graph.make_node<block_scaling_offset<float>>({{{"scale", scale}, {"offset", offset}}});

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
