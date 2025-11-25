#include <boost/ut.hpp>

#include <fair/picoscope/Picoscope3000a.hpp>
#include <fair/picoscope/Picoscope4000a.hpp>
#include <fair/picoscope/Picoscope5000a.hpp>
#include <fair/picoscope/Picoscope6000.hpp>

#include <fair/picoscope/StatusMessages.hpp>

#include <thread>

using namespace std::string_literals;

namespace fair::picoscope::test {

// enumerate the picoscope types to be tested
using picoscopeTypes = std::tuple<Picoscope3000a, Picoscope4000a, Picoscope5000a, Picoscope6000>;

/**
 * These Tests test the picoscope c++ wrappers with the basic workflows described in the manuals for streaming and rapid block mode
 * There is not dependency to gnuradio4 to keep these tests deliberately simple.
 */
const boost::ut::suite<"PicoscopeAPITests"> _ = [] {
    using namespace std::literals;
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::picoscope;

    "open and close"_test = []<PicoscopeImplementationLike PSImpl>() {
        PSImpl      picoscope{};
        PICO_STATUS openResult = picoscope.openUnit("");
        expect(openResult == PICO_OK) << [&]() { return std::format("failed to open picoscope: {}", openResult); } << fatal;
        std::array<int8_t, 50> string{};
        int16_t                len;
        expect(picoscope.getUnitInfo(string.data(), string.size(), &len, PICO_VARIANT_INFO) == PICO_OK) << fatal;
        std::string variant = {reinterpret_cast<char*>(string.data()), static_cast<std::size_t>(len - 1)};
        expect(picoscope.getUnitInfo(string.data(), string.size(), &len, PICO_HARDWARE_VERSION) == PICO_OK) << fatal;
        std::string hw_version = {reinterpret_cast<char*>(string.data()), static_cast<std::size_t>(len - 1)};
        expect(picoscope.getUnitInfo(string.data(), string.size(), &len, PICO_BATCH_AND_SERIAL) == PICO_OK) << fatal;
        std::string serial = {reinterpret_cast<char*>(string.data()), static_cast<std::size_t>(len - 1)};
        uint16_t    updatesRequired;
        auto        fwUpdates = [&]() {
            if constexpr (requires { picoscope.checkFirmwareUpdates(&updatesRequired); }) {
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
        constexpr float sampleRate        = 1000.0f;
        constexpr auto  streamingDuration = 2s;
        PSImpl          picoscope{};
        expect(picoscope.openUnit("") == PICO_OK) << fatal;
        const auto range = PicoscopeWrapper<PSImpl>::toRangeEnum(toAnalogChannelRange(5.0f).value()).value();
        expect(picoscope.setChannel(PSImpl::outputs[0].second, true, PSImpl::convertToCoupling(Coupling::DC).value(), range, 0.0f) == PICO_OK) << fatal;
        std::array<int16_t, 4096> buffer{};
        expect(picoscope.setDataBuffer(PSImpl::outputs[0].second, buffer.data(), buffer.size(), PSImpl::ratioNone) == PICO_OK) << fatal;
        auto timeInterval = static_cast<uint32_t>(1000.f / sampleRate);
        expect(picoscope.runStreaming(                        //
                   &timeInterval,                             // in: desired interval, out: actual interval
                   picoscope.convertTimeUnits(TimeUnits::ms), // time unit of the interval
                   0,                                         // pre-trigger-samples (unused)
                   buffer.size(),                             //
                   false,                                     // autoStop
                   1,                                         // downsampling ratio
                   picoscope.ratioNone,                       // downsampling ratio mode
                   static_cast<uint32_t>(buffer.size()))      // the size of the overview buffers
               == PICO_OK)
            << fatal;
        std::println("streaming started: time interval: 1ms -> {}ms, thread: {}", timeInterval, std::this_thread::get_id());
        const auto start = std::chrono::steady_clock::now();
        struct StreamingResult {
            std::optional<std::chrono::steady_clock::time_point> firstUpdate{};
            std::optional<std::chrono::steady_clock::time_point> lastUpdate{};
            std::optional<std::thread::id>                       threadId;
            std::size_t                                          samples   = 0UZ;
            std::size_t                                          n_updates = 0UZ;
        } result;
        auto streamingReadyCallback = static_cast<typename PSImpl::StreamingReadyType>([](int16_t /*handle*/, typename PSImpl::NSamplesType noOfSamples, uint32_t /*startIndex*/, int16_t /*overflow*/, uint32_t /*triggerAt*/, int16_t /*triggered*/, int16_t /*autoStop*/, void* vobj) {
            StreamingResult* lambdaResult = (static_cast<StreamingResult*>(vobj));
            if (!lambdaResult->firstUpdate) {
                lambdaResult->firstUpdate = std::chrono::steady_clock::now();
            } else {
                lambdaResult->lastUpdate = std::chrono::steady_clock::now();
                lambdaResult->samples += static_cast<size_t>(noOfSamples);
                ++lambdaResult->n_updates;
            }
            if (!lambdaResult->threadId) {
                lambdaResult->threadId = std::this_thread::get_id();
            } else {
                expect(lambdaResult->threadId == std::this_thread::get_id()); // assume that all updates come from the same thread
            }
            // std::println("received samples: {}, thread: {}", noOfSamples, std::this_thread::get_id());
        });
        bool switchedRange          = false;
        while (std::chrono::steady_clock::now() - start < streamingDuration) {
            if (!switchedRange && std::chrono::steady_clock::now() - start > streamingDuration / 2) {
                switchedRange = true;
                // Test that we can switch the range while streaming is in progress. Unfortunately, the effect is only observed the next time the acquisition is restarted
                auto newRange = PicoscopeWrapper<PSImpl>::toRangeEnum(toAnalogChannelRange(0.1f).value()).value();
                expect(picoscope.setChannel(PSImpl::outputs[0].second, true, PSImpl::convertToCoupling(Coupling::DC).value(), newRange, 0.0f) == PICO_OK) << fatal;
                std::println("picoscope switched channel range from +/-5V to +/-0.1V during streaming");
            }
            const PICO_STATUS streamingLatestValuesResult = picoscope.getStreamingLatestValues(streamingReadyCallback, &result);
            expect(streamingLatestValuesResult == PICO_OK || streamingLatestValuesResult == PICO_NO_SAMPLES_AVAILABLE) << [=]() { return std::format("Failed to get streaming latest values from the scope: {}", fair::picoscope::detail::statusToStringVerbose(streamingLatestValuesResult)); };
            ;
        }
        double effectiveSampleRate = 1e9 * static_cast<double>(result.samples) / static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(result.lastUpdate.value() - result.firstUpdate.value()).count());
        std::println("finished streaming: {} updates with a total of {} samples, effective sample rate = {}, configured sample rate: {}", result.n_updates, result.samples, effectiveSampleRate, sampleRate);
        expect(approx(effectiveSampleRate, static_cast<double>(sampleRate), 1e2));
        expect(picoscope.driverStop() == PICO_OK) << fatal;
        // optional: retrieve stored data
        expect(picoscope.closeUnit() == PICO_OK) << fatal;
    } | picoscopeTypes{};

    "rapidBlockMode"_test = []<PicoscopeImplementationLike PSImpl> {
        float              sampleRate = 1e3f;
        constexpr uint32_t nSegments  = 3;
        constexpr uint32_t nChannels  = 2;
        PSImpl             picoscope{};
        expect(picoscope.openUnit("") == PICO_OK) << fatal;
        const auto range = PicoscopeWrapper<PSImpl>::toRangeEnum(toAnalogChannelRange(5.0f).value()).value();
        expect(picoscope.setChannel(PSImpl::outputs[0].second, true, PSImpl::convertToCoupling(Coupling::DC).value(), range, 0.0f) == PICO_OK) << fatal;
        expect(picoscope.setChannel(PSImpl::outputs[0].second, true, PSImpl::convertToCoupling(Coupling::DC).value(), range, 0.0f) == PICO_OK) << fatal;
        int32_t maxSamples;
        expect(picoscope.memorySegments(nSegments, &maxSamples) == PICO_OK) << fatal;
        const TimebaseResult timebaseResult = picoscope.convertSampleRateToTimebase(sampleRate).value();
        expect(eq(sampleRate, timebaseResult.actualFreq)); // for 1 kHz this should be exact
        constexpr int32_t nPre       = 50;
        constexpr int32_t nPost      = 100;
        constexpr int32_t no_samples = nPre + nPost;
        for (uint32_t segment_idx = 0U; segment_idx < nSegments; segment_idx++) {
            float   time_interval_ns;
            int32_t max_samples;
            expect(picoscope.getTimebase2(timebaseResult.timebase, no_samples, &time_interval_ns, &max_samples, segment_idx) == PICO_OK) << fatal;
            expect(eq(time_interval_ns, 1e9f / timebaseResult.actualFreq));
            expect(gt(max_samples, no_samples));
        }
        const bool analogTrigger = true || PSImpl::N_DIGITAL_CHANNELS == 0; // Change true to false to test the digital trigger.
        // Since this needs a state change to trigger, it needs some kind of signal generator e.g. `saft-clk-gen tr0 -n IO3 -f 100 0` to create a 100 Hz clock on IO3 of a timing card
        if (analogTrigger) {
            constexpr int16_t  enable          = true;
            constexpr int16_t  threshold       = 0;
            constexpr uint32_t delay           = 0;
            constexpr int16_t  auto_trigger_ms = 50;
            expect(picoscope.setSimpleTrigger(enable, PSImpl::outputs[0].second, threshold, PSImpl::convertToThresholdDirection(TriggerDirection::Rising).value(), delay, auto_trigger_ms) == PICO_OK) << fatal;
        } else { // digital
            if constexpr (PSImpl::N_DIGITAL_CHANNELS > 0) {
                expect(picoscope.setDigitalPorts(true, static_cast<std::int16_t>(1.5 * std::numeric_limits<std::int16_t>::max() / 5.0)) == PICO_OK) << fatal;
                expect(picoscope.setTriggerDigitalPort(2U, TriggerDirection::Rising) == PICO_OK) << fatal;
            }
        }

        std::array<std::array<std::array<int16_t, no_samples>, nSegments>, nChannels + 2UZ> buffer{};
        for (uint32_t chanIdx = 0; chanIdx < nChannels; chanIdx++) {
            for (uint32_t segmentIdx = 0; segmentIdx < nSegments; segmentIdx++) {
                auto bufferSize = static_cast<int32_t>(buffer[chanIdx][segmentIdx].size());
                expect(picoscope.setDataBuffer(PSImpl::outputs[0].second, buffer[chanIdx][segmentIdx].data(), bufferSize, segmentIdx, PSImpl::ratioNone) == PICO_OK) << fatal;
            }
        }
        if constexpr (PSImpl::N_DIGITAL_CHANNELS > 0UZ) { // digital channels
            for (uint32_t chanIdx = 0; chanIdx < 2; chanIdx++) {
                for (uint32_t segmentIdx = 0; segmentIdx < nSegments; segmentIdx++) {
                    auto bufferSize = static_cast<int32_t>(buffer[nChannels + chanIdx][segmentIdx].size());
                    expect(picoscope.setDataBuffer(static_cast<typename PSImpl::ChannelType>(PSImpl::DIGI_PORT_0 + chanIdx), buffer[nChannels + chanIdx][segmentIdx].data(), bufferSize, segmentIdx, PSImpl::ratioNone) == PICO_OK) << fatal;
                }
            }
        }

        expect(picoscope.setNoOfCaptures(nSegments) == PICO_OK) << fatal;

        std::atomic_size_t result;
        auto               blockReadyCallback = static_cast<typename PSImpl::BlockReadyType>([](int16_t handle, PICO_STATUS status, void* param) {
            expect(handle != -1);
            expect(status == PICO_OK) << [&]() { return std::format("Block ready callback failed with error code: {}", detail::statusToStringVerbose(status)); };
            std::println("Block ready, thread = {}", std::this_thread::get_id());
            static_cast<std::atomic_size_t*>(param)->fetch_add(1UZ);
            // picoscope.getValues(...);
            // picoscope.getValuesTriggerTimeOffsetBulk64();
        });
        expect(picoscope.runBlock(nPre, nPost, timebaseResult.timebase, nullptr, 0, blockReadyCallback, &result) == PICO_OK) << fatal;
        std::println("started running in block mode, thread = {}", std::this_thread::get_id());

        uint32_t   no_of_captures           = 0;
        uint32_t   no_of_captures_processed = 0;
        const auto start                    = std::chrono::steady_clock::now();
        while (result.load(std::memory_order_relaxed) <= 0) {
            if (start + 10s < std::chrono::steady_clock::now()) {
                std::println("timed out waiting for capturing {} blocks", nSegments);
                expect(picoscope.driverStop() == PICO_OK) << fatal;
                break;
            }
            auto oldCaptures          = no_of_captures;
            auto oldCapturesProcessed = no_of_captures_processed;
            expect(picoscope.getNoOfCaptures(&no_of_captures) == PICO_OK) << fatal;
            expect(picoscope.getNoOfProcessedCaptures(&no_of_captures_processed) == PICO_OK) << fatal;
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
        std::array<int64_t, nSegments>                        times{};
        std::array<typename PSImpl::TimeUnitsType, nSegments> timeUnits{};
        const PICO_STATUS                                     getTriggerOffsetsResult = picoscope.getValuesTriggerTimeOffsetBulk64(times.data(), timeUnits.data(), 0, nSegments - 1);
        expect(getTriggerOffsetsResult == PICO_OK) << [&]() { return std::format("Failed to get trigger offsets from the scope: {}", detail::statusToStringVerbose(getTriggerOffsetsResult)); };
        std::println("time offsets: ", times);
        auto              noOfSamples    = static_cast<uint32_t>(nPre + nPost);
        int16_t           overflow       = 0;
        const PICO_STATUS getValueResult = picoscope.getValuesBulk(&noOfSamples, 0U, nSegments - 1, 1U, PSImpl::ratioNone, &overflow);
        expect(getValueResult == PICO_OK) << [&]() { return std::format("Failed to get Values from the scope: {}", detail::statusToStringVerbose(getValueResult)); };
        expect(eq(noOfSamples, static_cast<uint16_t>(nPre + nPost)));
        expect(eq(overflow, 0));
        expect(picoscope.driverStop() == PICO_OK) << fatal;
        expect(picoscope.closeUnit() == PICO_OK) << fatal;
    } | picoscopeTypes{};

    "rapidBlockModeOverlapped"_test = []<PicoscopeImplementationLike PSImpl> {
        float              sampleRate = 1e4f;
        constexpr uint32_t nSegments  = 3;
        constexpr uint32_t nChannels  = 2;
        PSImpl             picoscope{};
        expect(picoscope.openUnit("") == PICO_OK) << fatal;
        const auto range = PicoscopeWrapper<PSImpl>::toRangeEnum(toAnalogChannelRange(5.0f).value()).value();
        expect(picoscope.setChannel(PSImpl::outputs[0].second, true, PSImpl::convertToCoupling(Coupling::DC).value(), range, 0.0f) == PICO_OK) << fatal;
        expect(picoscope.setChannel(PSImpl::outputs[1].second, true, PSImpl::convertToCoupling(Coupling::DC).value(), range, 0.0f) == PICO_OK) << fatal;
        int32_t maxSamples;
        expect(picoscope.memorySegments(nSegments, &maxSamples) == PICO_OK) << fatal;
        const TimebaseResult timebaseResult = picoscope.convertSampleRateToTimebase(sampleRate).value();
        expect(eq(sampleRate, timebaseResult.actualFreq)); // for 1 kHz this should be exact
        int32_t nPre       = 1000;
        int32_t nPost      = 5000;
        int32_t no_samples = nPre + nPost;
        for (uint32_t segment_idx = 0U; segment_idx < nSegments; segment_idx++) {
            float   time_interval_ns;
            int32_t max_samples;
            expect(picoscope.getTimebase2(timebaseResult.timebase, no_samples, &time_interval_ns, &max_samples, segment_idx) == PICO_OK) << fatal;
            expect(eq(time_interval_ns, 1e9f / timebaseResult.actualFreq));
            expect(gt(max_samples, no_samples));
        }
        const int16_t  enable          = true;
        const int16_t  threshold       = 0;
        const uint32_t delay           = 0;
        const int16_t  auto_trigger_ms = 10;
        expect(picoscope.setSimpleTrigger(enable, PSImpl::outputs[0].second, threshold, PSImpl::convertToThresholdDirection(TriggerDirection::Rising).value(), delay, auto_trigger_ms) == PICO_OK) << fatal;

        std::array<std::array<std::array<int16_t, 4096>, nSegments>, nChannels> buffer{};
        for (uint32_t chanIdx = 0; chanIdx < nChannels; chanIdx++) {
            for (uint32_t segmentIdx = 0; segmentIdx < nSegments; segmentIdx++) {
                auto bufferSize = static_cast<int32_t>(buffer[chanIdx][segmentIdx].size());
                expect(picoscope.setDataBuffer(PSImpl::outputs[chanIdx].second, buffer[chanIdx][segmentIdx].data(), bufferSize, segmentIdx, PSImpl::ratioNone) == PICO_OK) << fatal;
            }
        }

        expect(picoscope.setNoOfCaptures(nSegments) == PICO_OK) << fatal;

        std::atomic_size_t             result;
        auto                           blockReadyCallback = static_cast<typename PSImpl::BlockReadyType>([](int16_t /*handle*/, PICO_STATUS /*status*/, void* param) {
            std::println("Block ready, thread = {}", std::this_thread::get_id());
            static_cast<std::atomic_size_t*>(param)->fetch_add(1UZ);
            // picoscope.getValues(...);
            // picoscope.getValuesTriggerTimeOffsetBulk64();
        });
        auto                           noOfSamples        = static_cast<uint32_t>(nPre + nPost);
        std::array<int16_t, nChannels> overflow{};
        PICO_STATUS                    getValueResult = picoscope.getValuesOverlappedBulk(0, &noOfSamples, 1U, PSImpl::ratioNone, 0U, nSegments - 1, overflow.data());
        expect(getValueResult == PICO_OK) << [&]() { return std::format("Failed to get Values from the scope: {}", fair::picoscope::detail::statusToStringVerbose(getValueResult)); };
        expect(picoscope.runBlock(nPre, nPost, timebaseResult.timebase, nullptr, 0, blockReadyCallback, &result) == PICO_OK) << fatal;
        expect(eq(noOfSamples, static_cast<uint16_t>(nPre + nPost)));
        std::println("started running in block mode, thread = {}", std::this_thread::get_id());

        uint32_t no_of_captures           = 0;
        uint32_t no_of_captures_processed = 0;
        bool     prematureStop            = false;
        bool     isArmed                  = true;
        auto     start                    = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < 5s) {
            if (start + 10s < std::chrono::steady_clock::now()) {
                std::println("timed out waiting for capturing {} blocks", nSegments);
                break;
            }
            auto oldCaptures          = no_of_captures;
            auto oldCapturesProcessed = no_of_captures_processed;
            expect(picoscope.getNoOfCaptures(&no_of_captures) == PICO_OK) << fatal;
            expect(picoscope.getNoOfProcessedCaptures(&no_of_captures_processed) == PICO_OK) << fatal;
            if (oldCaptures != no_of_captures || oldCapturesProcessed != no_of_captures_processed) {
                std::println("captured segments: {}, processed segments: {}, ready called {} times", no_of_captures, no_of_captures_processed, result.load(std::memory_order_relaxed));
            }
            if (prematureStop && isArmed && start + 1500ms < std::chrono::steady_clock::now()) {
                expect(picoscope.driverStop() == PICO_OK) << fatal;
                isArmed                     = false;
                PICO_STATUS getValueResult2 = picoscope.getValuesBulk(&noOfSamples, 0U, no_of_captures - 1, 1U, PSImpl::ratioNone, overflow.data()); // trigger transmission of all existing values
                expect(getValueResult2 == PICO_OK) << [&]() { return std::format("Failed to get Values from the scope: {}", fair::picoscope::detail::statusToStringVerbose(getValueResult)); };
                std::println("stopped the acquisition");
            }
            std::this_thread::sleep_for(1ms);
        }

        expect(picoscope.getNoOfCaptures(&no_of_captures) == PICO_OK) << fatal;
        expect(picoscope.getNoOfProcessedCaptures(&no_of_captures_processed) == PICO_OK) << fatal;
        expect(eq(no_of_captures, nSegments));
        expect(eq(no_of_captures_processed, nSegments));

        // get trigger offsets
        std::array<int64_t, nSegments>                        times{};
        std::array<typename PSImpl::TimeUnitsType, nSegments> timeUnits{};
        PICO_STATUS                                           getTriggerOffsetsResult = picoscope.getValuesTriggerTimeOffsetBulk64(times.data(), timeUnits.data(), 0, nSegments - 1);
        expect(getTriggerOffsetsResult == PICO_OK) << fatal;
        std::println("time offsets: ", times);
        for (auto segment_overflow : overflow) {
            expect(eq(segment_overflow, 0));
        }
        expect(picoscope.driverStop() == PICO_OK) << fatal;
        expect(picoscope.closeUnit() == PICO_OK) << fatal;
    } | picoscopeTypes{};

    "picoscopeWrapperStreaming"_test = []<PicoscopeImplementationLike PSImpl> {
        using std::chrono::duration_cast;
        using TClock                        = std::chrono::steady_clock;
        constexpr float          sampleRate = 2e4f;
        const auto               duration   = 4s;
        PicoscopeWrapper<PSImpl> picoscope{"", false};
        picoscope.configureChannel(0UZ, ChannelConfig{true, AnalogChannelRange::ps5V, 0.0f, Coupling::DC});
        picoscope.configureChannel(1UZ, ChannelConfig{true, AnalogChannelRange::ps5V, 0.0f, Coupling::DC});
        picoscope.startStreamingAcquisition(sampleRate, PSImpl::N_DIGITAL_CHANNELS > 0UZ);
        while (!picoscope.ready()) {
            picoscope.poll(); // wait for the picoscope to be initialised (takes some time)
        }
        struct Result {
            std::size_t        samples  = 0UZ;
            std::size_t        updates  = 0UZ;
            std::int16_t       overflow = 0;
            TClock::time_point start    = TClock::now();
            TClock::time_point last     = TClock::now();
        } result;
        const auto start = TClock::now();
        while (TClock::now() - start < duration) {
            picoscope.poll([&result](const std::span<std::span<const std::int16_t>>& values, std::int16_t overflow) {
                if (result.updates == 0UZ) {
                    result.start = TClock::now();
                }
                result.last = TClock::now();
                expect(eq(values.size(), PSImpl::N_DIGITAL_CHANNELS > 0UZ ? 3UZ : 2UZ)); // two active channels + digital
                result.samples += values[0].size();
                ++result.updates;
                result.overflow |= overflow;
            });
        }
        auto effectiveDuration = result.last - result.start;
        auto expectedSamples   = static_cast<std::size_t>(sampleRate * static_cast<float>(duration_cast<std::chrono::microseconds>(effectiveDuration).count()) * 1e-6f);
        std::println("model: {}, serial: {}, hardware version: {}", picoscope.getDeviceInfo().model, picoscope.getDeviceInfo().serial, picoscope.getDeviceInfo().hardwareVersion);
        std::println("Last error: {}", picoscope.getLastError().transform([](const Error& e) -> std::string { return std::format("{}: {} at {}:L{}:{}", e.getError(), e.getDescription(), e.location.file_name(), e.location.line(), e.location.column()); }).value_or("No errors occurred"));
        std::println("effective capture duration: {}, samples: {}, expected: {}, updates: {}, overflow: {:#b}", duration_cast<std::chrono::milliseconds>(effectiveDuration), result.samples, expectedSamples, result.updates, result.overflow);
        expect(approx(result.samples, expectedSamples, 5000UZ));
        expect(eq(result.overflow, 0));
        expect(ge(result.updates, 10UZ));
        expect(approx(effectiveDuration, duration, 1s));
        expect(picoscope.getDeviceInfo().model.starts_with(meta::type_name<PSImpl>().substr(26, 1))) << [&]() { return std::format("incorrect picoscope model {} for model: {}", picoscope.getDeviceInfo().model, meta::type_name<PSImpl>()); }; // assure that we've opened the correct model
        expect(picoscope.getDeviceInfo().serial.contains('/')) << [&]() { return picoscope.getDeviceInfo().serial; };                                                                                                                            // all picoscope serial numbers contain a slash
        expect(!picoscope.getDeviceInfo().hardwareVersion.empty());
    } | picoscopeTypes{};

    "picoscopeWrapperTriggered"_test = []<PicoscopeImplementationLike PSImpl> {
        using std::chrono::duration_cast;
        using TClock                                            = std::chrono::steady_clock;
        constexpr float          sampleRate                     = 1e4f; // 10 kHz
        constexpr std::size_t    nCaptures                      = 3;
        constexpr std::size_t    nPre                           = 500;
        constexpr std::size_t    nPast                          = 1500;
        constexpr std::size_t    nSamples                       = nPre + nPast;
        constexpr auto           expectedDurationPerAcquisition = std::chrono::microseconds(static_cast<long>(static_cast<float>(nSamples) / sampleRate * 1e6f));
        constexpr auto           duration                       = 5s;
        std::size_t              expectedCaptures               = (duration / expectedDurationPerAcquisition);
        PicoscopeWrapper<PSImpl> picoscope{"", false};
        while (!picoscope.ready()) {
            picoscope.poll(); // wait for the picoscope to be initialised (takes some time)
        }
        expect(picoscope.getDeviceInfo().model.starts_with(meta::type_name<PSImpl>().substr(26, 1))) << [&]() { return std::format("incorrect picoscope model {} for model: {}", picoscope.getDeviceInfo().model, meta::type_name<PSImpl>()); }; // assure that we've opened the correct model
        expect(picoscope.getDeviceInfo().serial.contains('/')) << [&]() { return picoscope.getDeviceInfo().serial; };                                                                                                                            // all picoscope serial numbers contain a slash
        expect(!picoscope.getDeviceInfo().hardwareVersion.empty());

        picoscope.configureChannel(0UZ, ChannelConfig{true, AnalogChannelRange::ps5V, 0.0f, Coupling::DC});
        picoscope.configureChannel(1UZ, ChannelConfig{true, AnalogChannelRange::ps5V, 0.0f, Coupling::DC});

        { // Test with an analogue trigger
            picoscope.configureTrigger(TriggerConfig{.source = ChannelName::A, .direction = TriggerDirection::Rising, .threshold = 0, .delay = 0, .auto_trigger_ms = 1});
            std::atomic nTriggered{0UZ};
            auto        callback = [&nTriggered]() { nTriggered.fetch_add(1UZ); };
            picoscope.startTriggeredAcquisition(sampleRate, nPre, nPast, nCaptures, callback, false);
            struct Result {
                std::size_t nCaptures = 0UZ;
            } result;
            const auto start = TClock::now();
            while (TClock::now() - start < duration) {
                picoscope.poll([&result, &nSamples](const std::span<std::span<const std::int16_t>>& values, const std::int16_t overflow) {
                    expect(eq(overflow, 0x0));
                    expect(eq(values.size(), 2UZ)); // two active channels
                    expect(eq(values[0].size(), nSamples));
                    expect(eq(values[1].size(), nSamples));
                    ++result.nCaptures;
                });
            }
            std::println("model: {}, serial: {}, hardware version: {}", picoscope.getDeviceInfo().model, picoscope.getDeviceInfo().serial, picoscope.getDeviceInfo().hardwareVersion);
            std::println("Last error: {}", picoscope.getLastError().transform([](const Error& e) -> std::string { return std::format("{}: {} at {}:L{}:{}", e.getError(), e.getDescription(), e.location.file_name(), e.location.line(), e.location.column()); }).value_or("No errors occurred"));
            std::println("expected captures: {}, actual captures: {}, actual callbacks: {}", expectedCaptures, result.nCaptures, nTriggered.load());
            expect(approx(nTriggered.load(), (expectedCaptures / nCaptures) - 2UZ, 4UZ));
            expect(approx(result.nCaptures, (expectedCaptures - 3 * nCaptures), 3 * nCaptures));
            picoscope.stopAcquisition();
        }

        if constexpr (PSImpl::N_DIGITAL_CHANNELS > 0UZ) { // test with a digital trigger if available
            constexpr auto threshold = static_cast<std::int16_t>(1.5 * std::numeric_limits<std::int16_t>::max() / 5.0);
            picoscope.configureTrigger(TriggerConfig{.source = 2U, .direction = TriggerDirection::Rising, .threshold = threshold, .delay = 0, .auto_trigger_ms = 0});
            std::atomic nTriggered{0UZ};
            auto        callback = [&nTriggered]() { nTriggered.fetch_add(1UZ); };
            picoscope.startTriggeredAcquisition(sampleRate, nPre, nPast, nCaptures, callback, true);
            while (!picoscope.ready()) {
                picoscope.poll(); // wait for the picoscope to be initialised (takes some time)
            }
            struct Result {
                std::size_t nCaptures = 0UZ;
            } result;
            const auto start = TClock::now();
            while (TClock::now() - start < duration) {
                picoscope.poll([&result, &nSamples](const std::span<std::span<const std::int16_t>>& values, const std::int16_t overflow) {
                    expect(eq(overflow, 0x0));
                    expect(eq(values.size(), 3UZ)); // two active channels + digital
                    expect(eq(values[0].size(), nSamples));
                    expect(eq(values[1].size(), nSamples));
                    ++result.nCaptures;
                });
            }
            std::println("model: {}, serial: {}, hardware version: {}", picoscope.getDeviceInfo().model, picoscope.getDeviceInfo().serial, picoscope.getDeviceInfo().hardwareVersion);
            std::println("Last error: {}", picoscope.getLastError().transform([](const Error& e) -> std::string { return std::format("{}: {} at {}:L{}:{}", e.getError(), e.getDescription(), e.location.file_name(), e.location.line(), e.location.column()); }).value_or("No errors occurred"));
            std::println("expected captures: {}, actual captures: {}, actual callbacks: {}", expectedCaptures, result.nCaptures, nTriggered.load());
            expect(approx(nTriggered.load(), (expectedCaptures / nCaptures) - 2UZ, 4UZ));
            expect(approx(result.nCaptures, (expectedCaptures - 3 * nCaptures), 3 * nCaptures));
            picoscope.stopAcquisition();
        }
    } | picoscopeTypes{};
};

} // namespace fair::picoscope::test

int main() { /* tests are statically executed */ }
