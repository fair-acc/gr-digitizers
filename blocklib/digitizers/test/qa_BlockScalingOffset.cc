#include <BlockScalingOffset.hpp>
#include <HelperBlocks.hpp>

#include <gnuradio-4.0/scheduler.hpp>

#include <boost/ut.hpp>

namespace gr::digitizers::block_scaling_offset_test {

const boost::ut::suite BlockScalingOffsetTests = [] {
    using namespace boost::ut;
    using namespace gr;
    using namespace gr::digitizers;
    using namespace fair::helpers;

    "scale and offset"_test = [] {
        constexpr float       scale  = 1.5;
        constexpr float       offset = 2;
        constexpr std::size_t n      = 30;
        std::vector<float>    data(n);
        std::iota(data.begin(), data.end(), 0);

        graph flowGraph;

        auto &src0 = flowGraph.make_node<VectorSource<float>>(data);
        auto &src1 = flowGraph.make_node<VectorSource<float>>(data);
        auto &snk0 = flowGraph.make_node<VectorSink<float>>();
        auto &snk1 = flowGraph.make_node<VectorSink<float>>();
        auto &bso  = flowGraph.make_node<BlockScalingOffset<float>>({ { { "scale", scale }, { "offset", offset } } });

        expect(eq(connection_result_t::SUCCESS, flowGraph.connect<"out">(src0).template to<"in_signal">(bso)));
        expect(eq(connection_result_t::SUCCESS, flowGraph.connect<"out">(src1).template to<"in_error">(bso)));
        expect(eq(connection_result_t::SUCCESS, flowGraph.connect<"out_signal">(bso).template to<"in">(snk0)));
        expect(eq(connection_result_t::SUCCESS, flowGraph.connect<"out_error">(bso).template to<"in">(snk1)));

        scheduler::simple sched{ std::move(flowGraph) };
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
