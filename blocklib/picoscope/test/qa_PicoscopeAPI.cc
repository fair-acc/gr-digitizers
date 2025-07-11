#include <boost/ut.hpp>

#include <Picoscope3000a.hpp>
#include <Picoscope4000a.hpp>
#include <Picoscope5000a.hpp>
#include <Picoscope6000.hpp>

#include <StatusMessages.hpp>

#include <thread>

using namespace std::string_literals;

namespace fair::picoscope::test {

// enumerate the picoscope types to be tested
using picoscopeTypes = std::tuple<
        Picoscope3000a,
        Picoscope4000a,
        Picoscope5000a,
        Picoscope6000
        >;

/**
 * These Tests test the picoscope c++ wrappers with the basic workflows described in the manuals for streaming and rapid block mode
 * There is not dependency to gnuradio4 to keep these tests deliberately simple.
 */
const boost::ut::suite<"PicoscopeAPITests"> PicoscopeAPITests = [] {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::picoscope;

    "open and close"_test = []<PicoscopeImplementationLike PSImpl>() {
        PSImpl picoscope{};
        PICO_STATUS openResult = picoscope.openUnit("");
        expect(openResult == PICO_OK) << [&]() { return std::format("failed to open picoscope: {}", openResult); } << fatal;
        std::array<int8_t, 50> string{};
        int16_t len;
        expect(picoscope.getUnitInfo(string.data(), string.size(), &len, PICO_VARIANT_INFO) == PICO_OK) << fatal;
        std::string variant = {reinterpret_cast<char*>(string.data()), static_cast<std::size_t>(len-1)};
        expect(picoscope.getUnitInfo(string.data(), string.size(), &len, PICO_HARDWARE_VERSION) == PICO_OK) << fatal;
        std::string hw_version = {reinterpret_cast<char*>(string.data()), static_cast<std::size_t>(len-1)};
        expect(picoscope.getUnitInfo(string.data(), string.size(), &len, PICO_BATCH_AND_SERIAL) == PICO_OK) << fatal;
        std::string serial = {reinterpret_cast<char*>(string.data()), static_cast<std::size_t>(len-1)};
        uint16_t updatesRequired;
        auto fwUpdates = [&]() {
            if constexpr (requires {picoscope.checkFirmwareUpdates(&updatesRequired);} ) {
                return picoscope.checkFirmwareUpdates();
            } else {
                return std::vector<std::pair<std::string, PICO_FIRMWARE_INFO>>{};
            }
        }();
        expect(picoscope.closeUnit() == PICO_OK) << fatal;
        std::println("Checked Picoscope: variant:{}, hw_version: {}, batch/serial: {}, nFirmwareInfo = {}, updatesRequired = {}", variant, hw_version, serial, fwUpdates.size(), updatesRequired);
        for (auto info : fwUpdates) {
            std::println("  - {}, currentVersion: {}, updateVersion: {}, update required: {}", info.first, info.second.currentVersion, info.second.updateVersion, info.second.updateRequired > 0 ? "yes" : "no");
        }
    } | picoscopeTypes{};

    "streamingMode"_test = []<PicoscopeImplementationLike PSImpl>() {
        constexpr float sampleRate = 1000.0f;
        constexpr auto streamingDuration = 2s;
        PSImpl picoscope{};
        expect(picoscope.openUnit("") == PICO_OK) << fatal;
        // TODO: check cast of channel range to probe range... looks fishy
        expect(picoscope.setChannel(PSImpl::convertToChannel("A").value(), true, PSImpl::convertToCoupling(Coupling::DC), PSImpl::convertToRange(5.0f), 0.0f) == PICO_OK) << fatal;
        std::array<int16_t, 4096> buffer{};
        expect(picoscope.setDataBuffer(PSImpl::convertToChannel("A").value(), buffer.data(), buffer.size(), PSImpl::ratioNone()) == PICO_OK) << fatal;
        auto timeInterval = static_cast<uint32_t>(1000.f / sampleRate);
        expect(picoscope.runStreaming(                                 //
                                    &timeInterval,                           // in: desired interval, out: actual interval
                                    picoscope.convertTimeUnits(TimeUnits::ms), // time unit of interval
                                    0,                                       // pre-trigger-samples (unused)
                                    buffer.size(),                           //
                                    false,                                   // autoStop
                                    1,                                       // downsampling ratio
                                    picoscope.ratioNone(),                     // downsampling ratio mode
                                    static_cast<uint32_t>(buffer.size()))    // the size of the overview buffers
                == PICO_OK) << fatal;
        std::println("streaming started: time interval: 1ms -> {}ms, thread: {}", timeInterval, std::this_thread::get_id());
        auto start = std::chrono::steady_clock::now();
        struct StreamingResult {
            std::optional<std::chrono::steady_clock::time_point> firstUpdate{};
            std::optional<std::chrono::steady_clock::time_point> lastUpdate{};
            std::optional<std::thread::id> threadId;
            std::size_t samples = 0UZ;
            std::size_t n_updates = 0UZ;
        } result;
        auto streamingReadyCallback = static_cast<PSImpl::StreamingReadyType>([](int16_t /*handle*/, PSImpl::NSamplesType noOfSamples, uint32_t startIndex, int16_t overflow, uint32_t /*triggerAt*/, int16_t /*triggered*/, int16_t /*autoStop*/, void* vobj) {
            StreamingResult* result = (static_cast<StreamingResult*>(vobj));
            if (!result->firstUpdate) {
                result->firstUpdate = std::chrono::steady_clock::now();
            } else {
                result->lastUpdate = std::chrono::steady_clock::now();
                result->samples += static_cast<size_t>(noOfSamples);
                result->n_updates++;
            }
            if (!result->threadId) {
                result->threadId = std::this_thread::get_id();
            } else {
                expect(result->threadId == std::this_thread::get_id()); // assume that all updates come from the same thread
            }
            //std::println("received samples: {}, thread: {}", noOfSamples, std::this_thread::get_id());
        });
        bool switchedRange = false;
        while (std::chrono::steady_clock::now() - start < streamingDuration) {
            if (!switchedRange && std::chrono::steady_clock::now() - start > streamingDuration / 2 ) {
                switchedRange = true;
                // test that we can switch the range while streaming is in progress
                expect(picoscope.setChannel(PSImpl::convertToChannel("A").value(), true, PSImpl::convertToCoupling(Coupling::DC), PSImpl::convertToRange(0.1f), 0.0f) == PICO_OK) << fatal;
                std::println("picoscope switched channel range from +/-5V to +/-0.1V during streaming");
            }
            expect(picoscope.getStreamingLatestValues(streamingReadyCallback, &result) == PICO_OK) << fatal;
        }
        double effectiveSampleRate = 1e9 * static_cast<double>(result.samples) / static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>( result.lastUpdate.value() - result.firstUpdate.value()).count());
        std::println("finished streaming: {} updates with a total of {} samples, effective sample rate = {}, configured sample rate: 1000", result.n_updates, result.samples, effectiveSampleRate);
        expect(approx(effectiveSampleRate, static_cast<double>(sampleRate), 1e2));
        expect(picoscope.driverStop() == PICO_OK) << fatal;
        //optional: retrieve stored data
        expect(picoscope.closeUnit() == PICO_OK) << fatal;
} | picoscopeTypes{};

    "rapidBlockMode"_test = []<PicoscopeImplementationLike PSImpl> {
        float sampleRate = 1e3f;
        constexpr uint32_t nSegments = 3;
        constexpr uint32_t nChannels = 2;
        PSImpl picoscope{};
        expect(picoscope.openUnit("") == PICO_OK) << fatal;
        expect(picoscope.setChannel(PSImpl::convertToChannel("A").value(), true, PSImpl::convertToCoupling(Coupling::DC), PSImpl::convertToRange(5.0f), 0.0f) == PICO_OK) << fatal;
        expect(picoscope.setChannel(PSImpl::convertToChannel("B").value(), true, PSImpl::convertToCoupling(Coupling::DC), PSImpl::convertToRange(5.0f), 0.0f) == PICO_OK) << fatal;
        int32_t maxSamples;
        expect(picoscope.memorySegments(nSegments, &maxSamples) == PICO_OK) << fatal;
        const TimebaseResult timebaseResult = picoscope.convertSampleRateToTimebase(sampleRate);
        expect(eq(sampleRate, timebaseResult.actualFreq)); // for 1kHz this should be exact
        int32_t no_samples = 1100;
        for (uint32_t segment_idx = 0U; segment_idx < nSegments; segment_idx++) {
            float time_interval_ns;
            int32_t max_samples;
            expect(picoscope.getTimebase2(timebaseResult.timebase, no_samples, &time_interval_ns, &max_samples, segment_idx) == PICO_OK) << fatal;
            expect(eq(time_interval_ns, 1e9f / timebaseResult.actualFreq));
            expect(gt(max_samples, no_samples));
        }
        bool simpleTrigger = true;
        if (simpleTrigger) {
            const int16_t enable = true;
            const int16_t threshold = 0;
            const uint32_t delay = 0;
            const int16_t auto_trigger_ms = 50;
            expect(picoscope.setSimpleTrigger(enable, PSImpl::convertToChannel("A").value(), threshold, PSImpl::convertToThresholdDirection(TriggerDirection::Rising), delay, auto_trigger_ms) == PICO_OK) << fatal;
        } else {
            // more complicated but flexible way to set up triggers
            //std::array<typename picoscope.ConditionType, 2> conditions{{
            //    {picoscope.convertToChannel("A").value(), PSImpl::conditionTrue()},
            //    {picoscope.convertToChannel("B").value(), PSImpl::conditionTrue()},
            //}};
            //expect(picoscope.setTriggerChannelConditions(conditions.data(), conditions.size(), PSImpl::conditionsInfoAdd()) == PICO_OK) << fatal;
            //std::array<typename picoscope.TriggerDirectionType, 2> triggerDirections{{
            //    {picoscope.convertToChannel("A").value(), PSImpl::convertToThresholdDirection(TriggerDirection::Rising)},
            //    {picoscope.convertToChannel("B").value(), PSImpl::convertToThresholdDirection(TriggerDirection::Falling)},
            //}};
            //expect(picoscope.setTriggerChannelDirections(triggerDirections) == PICO_OK) << fatal;
            //int16_t auxOutputEnable = 0; // unused for ps4000a
            //int32_t autoTriggerMilliseconds = 1; // 0 means wait indefinitely for trigger, otherwise ms without trigger after which an acquisition is triggered anyway
            //std::array<typename picoscope.TriggerChannelProperties, 2> triggerProperties{{
            //    {0, 0U, 0, 0U, picoscope.convertToChannel("A").value(), PSImpl::thresholdMode()},
            //    {0, 0U, 0, 0U, picoscope.convertToChannel("B").value(), PSImpl::thresholdMode()},
            //}};
            //expect(picoscope.setTriggerChannelProperties(triggerProperties.data(), triggerProperties.size(), auxOutputEnable, autoTriggerMilliseconds) == PICO_OK) << fatal;
            //expect(picoscope.setTriggerDelay(0) == PICO_OK) << fatal; // sets trigger delay in sample periods
        }
        int32_t nPre = 50;
        int32_t nPost = 100;

        std::array<std::array<std::array<int16_t, 4096>, nSegments>, nChannels> buffer{};
        for (uint32_t chanIdx = 0; chanIdx < nChannels; chanIdx++) {
            for (uint32_t segmentIdx = 0; segmentIdx < nSegments; segmentIdx++) {
                auto bufferSize = static_cast<int32_t>(buffer[chanIdx][segmentIdx].size());
                expect(picoscope.setDataBufferForSegment(PSImpl::convertToChannel(chanIdx).value(), buffer[chanIdx][segmentIdx].data(), bufferSize, segmentIdx, PSImpl::ratioNone()) == PICO_OK) << fatal;
            }
        }

        expect(picoscope.setNoOfCaptures(nSegments) == PICO_OK) << fatal;

        std::atomic_size_t result;
        auto blockReadyCallback = static_cast<PSImpl::BlockReadyType>([](int16_t handle, PICO_STATUS status, void* param) {
            std::println("Block ready, thread = {}", std::this_thread::get_id());
            static_cast<std::atomic_size_t*>(param)->fetch_add(1UZ);
            //picoscope.getValues(...);
            //picoscope.getValuesTriggerTimeOffsetBulk64();
        });
        expect(picoscope.runBlock(nPre, nPost, timebaseResult.timebase, nullptr, 0, blockReadyCallback, &result) == PICO_OK) << fatal;
        std::println("started running in block mode, thread = {}", std::this_thread::get_id());

        uint32_t no_of_captures = 0;
        uint32_t no_of_captures_processed = 0;
        auto start = std::chrono::steady_clock::now();
        while (result.load(std::memory_order_relaxed) <= 0) {
            if (start + 10s < std::chrono::steady_clock::now()) {
                std::println("timed out waiting for capturing {} blocks", nSegments);
                break;
            }
            auto oldCaptures = no_of_captures;
            auto oldCapturesProcessed = no_of_captures_processed;
            expect(picoscope.getNoOfCaptures(&no_of_captures) == PICO_OK) << fatal;
            if (no_of_captures > oldCaptures || no_of_captures_processed > oldCapturesProcessed) {
                std::println("captured segments: {}, processed segments: {}", no_of_captures, no_of_captures_processed);
            }
            std::this_thread::sleep_for(1ms);
        }

        expect(picoscope.getNoOfCaptures(&no_of_captures) == PICO_OK) << fatal;
        expect(picoscope.getNoOfProcessedCaptures(&no_of_captures_processed) == PICO_OK) << fatal;
        expect(eq(no_of_captures, nSegments));
        expect(eq(no_of_captures_processed, nSegments));

        // get trigger offsets
        std::array<int64_t,nSegments> times{};
        std::array<typename PSImpl::TimeUnitsType,nSegments> timeUnits{};
        PICO_STATUS getTriggerOffsetsResult = picoscope.getValuesTriggerTimeOffsetBulk64(times.data(), timeUnits.data(), 0, nSegments - 1);
        std::println("time offsets: ", times);
        auto noOfSamples = static_cast<uint32_t>(nPre + nPost);
        int16_t overflow = 0;
        PICO_STATUS getValueResult = picoscope.getValuesBulk(&noOfSamples, 0U, nSegments-1, 1U, PSImpl::ratioNone(), &overflow);
        expect(getValueResult == PICO_OK) << [&]() {return std::format("Failed to get Values from the scope: {}", fair::picoscope::detail::statusToStringVerbose(getValueResult));};
        expect(eq(noOfSamples, static_cast<uint16_t>(nPre + nPost)));
        expect(eq(overflow, 0));
        expect(picoscope.driverStop() == PICO_OK) << fatal;
        expect(picoscope.closeUnit() == PICO_OK) << fatal;
    } | picoscopeTypes{};

};

} // namespace fair::picoscope::test

int main() { /* tests are statically executed */ }
