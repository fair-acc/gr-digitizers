#include "qa_PicoscopeTimingHelpers.hpp"
#include <fair/picoscope/Picoscope5000a.hpp>

using namespace std::string_literals;
using namespace fair::picoscope;

using TPSImpl = Picoscope5000a;

namespace fair::picoscope::test {

const boost::ut::suite<"PicoscopeTimingTests"> PicoscopeTimingTests = [] {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::picoscope;
    /**
     * These are special hardware tests which needs a properly configured timing receiver with its IO1..3 connected to the Picoscopes input A, B, D/H.
     * Additionally, for MSO scopes, IO3 should be connnected to D4 or to External if supported on the scope.
     * It will generate a known pattern of timing events, configure the timing receiver to generate pules on IO3 for each event and output a known pattern on IO1/2.
     * The flowgraph in this test will then store the corresponding output samples and tags and check that the tags and output levels are correct.
     *
     * Use different sampling rates and Trigger configurations.
     */
    "streamingWithTimingSampleRate"_test = [&](const float samplingRate) {
        testStreamingWithTiming<TPSImpl>(samplingRate, 7s, "D");
        std::this_thread::sleep_for(100ms);
    } | std::array{10000.0f, 100000.0f};

    "streamingWithTimingDigitalTriggerSampleRate"_test = [&](const float samplingRate) {
        testStreamingWithTiming<TPSImpl>(samplingRate, 7s, "DI4");
        std::this_thread::sleep_for(100ms);
    } | std::array{10000.0f, 100000.0f};

    "triggeredTimingAcquisition"_test = [&](const float samplingRate) {
        testTriggeredAcquisitionWithTiming<TPSImpl>(samplingRate, 7s, "D");
        std::this_thread::sleep_for(100ms);
    } | std::array{10000.0f, 100000.0f};

    "triggeredTimingAcquisitionFast"_test = [&](const float samplingRate) { testTriggeredAcquisitionWithTiming<TPSImpl>(samplingRate, 7s, "D", 100us); } | std::array{250.0e6f};
};

} // namespace fair::picoscope::test

int main() { /* tests are statically executed */ }
