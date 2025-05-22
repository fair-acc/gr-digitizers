#include <TimingMatcher.hpp>
#include <boost/ut.hpp>
#include <format>

using namespace std::string_literals;
using namespace std::chrono_literals;

template<>
struct std::formatter<gr::Tag> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    constexpr auto format(const gr::Tag& tag, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "  {}->{{ {} }}\n", tag.index, tag.map);
    }
};

namespace fair::picoscope::test {

const boost::ut::suite<"TimingMatchers"> TimingMatcherTests = [] {
    using namespace boost::ut;
    using namespace gr;
    using namespace gr::test;
    using namespace fair::picoscope;
    using fair::picoscope::timingmatcher::TimingMatcher;

    auto generateTimingTag = [](std::string&& event, std::uint64_t time, float offset, bool hwTrigger = true, std::optional<std::uint64_t> localTime = std::nullopt) {
        return gr::property_map{
            {gr::tag::TRIGGER_NAME.shortKey(), std::move(event)},
            {gr::tag::TRIGGER_TIME.shortKey(), time},
            {gr::tag::TRIGGER_OFFSET.shortKey(), offset},
            {gr::tag::TRIGGER_META_INFO.shortKey(), gr::property_map{{"LOCAL-TIME", localTime.value_or(time)}, {"HW-TRIGGER", hwTrigger}}},
        };
    };

    // small helper to print the content of the ranges if there is a mismatch
    auto expectRangesEquals = [](const auto& r1, const auto& r2) { expect(std::ranges::equal(r1, r2)) << [&r1, &r2]() { return std::format("exp: {}\n got: {}", gr::join(r1), gr::join(r2)); }; };

    "simpleMatching"_test = [&] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1"s, acqTimestamp + 100'000, 0.0f, true),
            generateTimingTag("EVT_CMD2"s, acqTimestamp + 150'000, 0.0f, true),
            generateTimingTag("EVT_CMD3"s, acqTimestamp + 200'000, 0.0f, true),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
        auto          result = matcher.match(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(triggerSampleIndices.size(), result.processedTags));
        expect(eq(240uz, result.processedSamples));
        expectRangesEquals(
            std::vector<gr::Tag>{
                {100, generateTimingTag("EVT_CMD1"s, acqTimestamp + 100'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2"s, acqTimestamp + 150'000, 0.0f, true)},
                {200, generateTimingTag("EVT_CMD3"s, acqTimestamp + 200'000, 0.0f, true)},
            },
            result.tags);
    };

    "identical_timestamps"_test = [&] { // checks correct handling of the case of multiple events on the exact same whiterabbit timestamp
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1"s, acqTimestamp + 100'000, 0.0f, true),
            generateTimingTag("EVT_CMD2A"s, acqTimestamp + 150'000, 0.0f, true),
            generateTimingTag("EVT_CMD2B"s, acqTimestamp + 150'000, 0.0f, true),
            generateTimingTag("EVT_CMD2C"s, acqTimestamp + 150'000, 0.0f, true),
            generateTimingTag("EVT_CMD3"s, acqTimestamp + 200'000, 0.0f, true),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
        auto          result = matcher.match(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(5, result.processedTags));
        expect(eq(240uz, result.processedSamples));
        expectRangesEquals(
            std::vector<gr::Tag>{
                {100, generateTimingTag("EVT_CMD1"s, acqTimestamp + 100'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2A"s, acqTimestamp + 150'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2B"s, acqTimestamp + 150'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2C"s, acqTimestamp + 150'000, 0.0f, true)},
                {200, generateTimingTag("EVT_CMD3"s, acqTimestamp + 200'000, 0.0f, true)},
            },
            result.tags);
    };

    "identical_timestamps_no_hw"_test = [&] { // checks correct handling of the case of multiple events on the exact same whiterabbit timestamp where some of the events do not provide hardware pulses and some do
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1"s, acqTimestamp + 100'000, 0.0f, true),
            generateTimingTag("EVT_CMD2A"s, acqTimestamp + 150'000, 0.0f, false),
            generateTimingTag("EVT_CMD2B"s, acqTimestamp + 150'000, 0.0f, true),
            generateTimingTag("EVT_CMD2C"s, acqTimestamp + 150'000, 0.0f, false),
            generateTimingTag("EVT_CMD3"s, acqTimestamp + 200'000, 0.0f, true),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
        auto          result = matcher.match(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(5, result.processedTags));
        expect(eq(240uz, result.processedSamples));
        expectRangesEquals(
            std::vector<gr::Tag>{
                {100, generateTimingTag("EVT_CMD1"s, acqTimestamp + 100'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2A"s, acqTimestamp + 150'000, 0.0f, false)},
                {150, generateTimingTag("EVT_CMD2B"s, acqTimestamp + 150'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2C"s, acqTimestamp + 150'000, 0.0f, false)},
                {200, generateTimingTag("EVT_CMD3"s, acqTimestamp + 200'000, 0.0f, true)},
            },
            result.tags);
    };

    "differentStartTimes-TimingFirst"_test = [&] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMDA", acqTimestamp - 10'200'000, 0.0f, true),
            generateTimingTag("EVT_CMDB", acqTimestamp - 10'150'000, 0.0f, true),
            generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.0f, true),
            generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.0f, true),
            generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.0f, true),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
        auto          result = matcher.match(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(5uz, result.processedTags));
        expect(eq(240uz, result.processedSamples));
        expectRangesEquals(
            std::vector<gr::Tag>{
                {100, generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.0f, true)},
                {200, generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.0f, true)},
            },
            result.tags);
    };

    "differentStartTimes-PulsesFirst"_test = [&] {
        unsigned long                 acqTimestamp = 1000000;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1", acqTimestamp + 1'100'000, 0.0f, true),
            generateTimingTag("EVT_CMD2", acqTimestamp + 1'150'000, 0.0f, true),
            generateTimingTag("EVT_CMD3", acqTimestamp + 1'200'000, 0.0f, true),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 150, 200, 1'100, 1'150, 1'200};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
        auto          result = matcher.match(tags, triggerSampleIndices, 1'250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(3uz, result.processedTags));
        expect(eq(1'240uz, result.processedSamples));
        std::vector<gr::Tag> expected{
            {100, generateTimingTag("UNKNOWN_EVENT", acqTimestamp + 100'000, 0.0f, false)},
            {150, generateTimingTag("UNKNOWN_EVENT", acqTimestamp + 150'000, 0.0f, false)},
            {200, generateTimingTag("UNKNOWN_EVENT", acqTimestamp + 200'000, 0.0f, false)},
            {1'100, generateTimingTag("EVT_CMD1", acqTimestamp + 1'100'000, 0.0f, true)},
            {1'150, generateTimingTag("EVT_CMD2", acqTimestamp + 1'150'000, 0.0f, true)},
            {1'200, generateTimingTag("EVT_CMD3", acqTimestamp + 1'200'000, 0.0f, true)},
        };
        expectRangesEquals(expected, result.tags);
    };

    "overlappingEvents"_test = [&] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.0f, true),
            generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.0f, true),
            generateTimingTag("EVT_CMD2b", acqTimestamp + 151'000, 0.0f, true),
            generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.0f, true),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
        auto          result = matcher.match(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(4uz, result.processedTags));
        expect(eq(240uz, result.processedSamples));
        expect(approx(std::get<float>(result.tags[2].map.at(gr::tag::TRIGGER_OFFSET.shortKey())), 0.0f, 1e-10f));
        result.tags[2].map.at(gr::tag::TRIGGER_OFFSET.shortKey()) = 0.0f;

        expectRangesEquals(
            std::vector<gr::Tag>{
                {100, generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.0f, true)},
                {151, generateTimingTag("EVT_CMD2b", acqTimestamp + 151'000, 0.0f, true)},
                {200, generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.0f, true)},
            },
            result.tags);
    };

    "multiEvents"_test = [&] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.0f, true),
            generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.0f, true),
            generateTimingTag("EVT_CMD2b", acqTimestamp + 150'000, 0.0f, true),
            generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.0f, true),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
        auto          result = matcher.match(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(4uz, result.processedTags));
        expect(eq(240uz, result.processedSamples));

        expectRangesEquals(
            std::vector<gr::Tag>{
                {100, generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2b", acqTimestamp + 150'000, 0.0f, true)},
                {200, generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.0f, true)},
            },
            result.tags);
    };

    "tagWithoutTrigger"_test = [&] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.0f, true),
            generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.0f, true),
            generateTimingTag("EVT_CMDA", acqTimestamp + 180'000, 0.0f, false),
            generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.0f, true),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
        auto          result = matcher.match(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(4uz, result.processedTags));
        expect(eq(240uz, result.processedSamples));

        expectRangesEquals(
            std::vector<gr::Tag>{
                {100, generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.0f, true)},
                {180, generateTimingTag("EVT_CMDA", acqTimestamp + 180'000, 0.0f, false)},
                {200, generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.0f, true)},
            },
            result.tags);
    };

    "tagWithMissingTrigger"_test = [&] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.0f, true),
            generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.0f, true),
            generateTimingTag("EVT_CMDA", acqTimestamp + 180'000, 0.0f, true),
            generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.0f, true),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
        auto          result = matcher.match(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(4uz, result.processedTags));
        expect(eq(240uz, result.processedSamples));

        expectRangesEquals(
            std::vector<gr::Tag>{
                {100, generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.0f, true)},
                {180, generateTimingTag("EVT_CMDA", acqTimestamp + 180'000, 0.0f, true)},
                {200, generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.0f, true)},
            },
            result.tags);
    };

    "futurePulses"_test = [&] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.0f, true),
            generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.0f, true),
            generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.0f, true),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 150, 200, 300, 2'000};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
        auto          result = matcher.match(tags, triggerSampleIndices, 2'001uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(3uz, result.processedTags));
        expect(eq(1991uz, result.processedSamples));

        expectRangesEquals(
            std::vector<gr::Tag>{
                {100, generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.0f, true)},
                {200, generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.0f, true)},
                {300, generateTimingTag("UNKNOWN_EVENT", acqTimestamp + 300'000, 0.0f, false)},
            },
            result.tags);
    };

    "differentClocks"_test = [&] {
        unsigned long                 wrTimestamp  = 23456789;
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1", wrTimestamp + 100'000, 0.0f, true, acqTimestamp + 100'000),
            generateTimingTag("EVT_CMD2", wrTimestamp + 150'000, 0.0f, true, acqTimestamp + 150'000),
            generateTimingTag("EVT_CMD3", wrTimestamp + 200'000, 0.0f, true, acqTimestamp + 200'000),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
        auto          result = matcher.match(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(3uz, result.processedTags));
        expect(eq(240uz, result.processedSamples));

        expectRangesEquals(
            std::vector<gr::Tag>{
                {100, generateTimingTag("EVT_CMD1", wrTimestamp + 100'000, 0.0f, true, acqTimestamp + 100'000)},
                {150, generateTimingTag("EVT_CMD2", wrTimestamp + 150'000, 0.0f, true, acqTimestamp + 150'000)},
                {200, generateTimingTag("EVT_CMD3", wrTimestamp + 200'000, 0.0f, true, acqTimestamp + 200'000)},
            },
            result.tags);
    };

    "futureEvents"_test = [&] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.0f, true),
            generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.0f, true),
            generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.0f, true),
            generateTimingTag("EVT_CMD4", acqTimestamp + 234'000, 0.0f, true),
            generateTimingTag("EVT_CMD5", acqTimestamp + 253'000, 0.0f, true),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
        auto          result = matcher.match(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(4uz, result.processedTags));
        expect(eq(240uz, result.processedSamples));

        expectRangesEquals(
            std::vector<gr::Tag>{
                {100, generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.0f, true)},
                {200, generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.0f, true)},
                {234, generateTimingTag("EVT_CMD4", acqTimestamp + 234'000, 0.0f, true)},
            },
            result.tags);
    };

    "compensateOffset"_test = [&] {
        // since the generated trigger pulses are generated with a small offset compared to the actual event, they have to be shifted accordingly
        // to this end in the raw tags, `TRIGGER_TIME` contains the time in ns that the event has to be shifted compared to the pulse.
        // The matching algorithm has to account for this shift when determining the correct sample and has to correct the `TRIGGER_OFFSET` field according to the sampling rate and sample-relative calculated position
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, -1e3f, true, acqTimestamp + 101'000),
            generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, -2e4f, true, acqTimestamp + 170'000),
            generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, -3e2f, true, acqTimestamp + 200'000),
        };
        std::vector<std::size_t> triggerSampleIndices{101, 170, 200};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
        auto          result = matcher.match(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(triggerSampleIndices.size(), result.processedTags));
        expect(eq(240uz, result.processedSamples));

        // check and fix inexact offsets
        expect(approx(std::get<float>(result.tags[0].map.at(gr::tag::TRIGGER_OFFSET.shortKey())), 0.0f, 1e-10f));
        result.tags[0].map.at(gr::tag::TRIGGER_OFFSET.shortKey()) = 0.0f;
        expect(approx(std::get<float>(result.tags[1].map.at(gr::tag::TRIGGER_OFFSET.shortKey())), 0.0f, 1e-10f));
        result.tags[1].map.at(gr::tag::TRIGGER_OFFSET.shortKey()) = 0.0f;
        expect(approx(std::get<float>(result.tags[2].map.at(gr::tag::TRIGGER_OFFSET.shortKey())), 0.7f, 1e-10f));
        result.tags[2].map.at(gr::tag::TRIGGER_OFFSET.shortKey()) = 0.0f;

        expectRangesEquals(
            std::vector<gr::Tag>{
                {100, generateTimingTag("EVT_CMD1", acqTimestamp + 100'000, 0.f, true, acqTimestamp + 101'000)},
                {150, generateTimingTag("EVT_CMD2", acqTimestamp + 150'000, 0.f, true, acqTimestamp + 170'000)},
                {199, generateTimingTag("EVT_CMD3", acqTimestamp + 200'000, 0.f, true, acqTimestamp + 200'000)},
            },
            result.tags);
    };

    "emptyInputs"_test = [&] {
        // Empty inputs: (tags.empty(), triggerSamples.empty()) must not crash and should return {0,0,{}}. This also catches the trigger_index‑1 noted in the header review.
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1"s, acqTimestamp + 100'000, 0.0f, true),
            generateTimingTag("EVT_CMD2"s, acqTimestamp + 150'000, 0.0f, true),
            generateTimingTag("EVT_CMD3"s, acqTimestamp + 200'000, 0.0f, true),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 150, 200};
        { // empty timing tag list
            TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
            auto          result = matcher.match(std::vector<gr::property_map>{}, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));
            expect(eq(result.processedTags, 0uz));
            expect(eq(result.processedSamples, 240uz));
            expectRangesEquals(
                std::vector<gr::Tag>{
                    {100, generateTimingTag("UNKNOWN_EVENT", acqTimestamp + 100'000, 0.0f, false)},
                    {150, generateTimingTag("UNKNOWN_EVENT", acqTimestamp + 150'000, 0.0f, false)},
                    {200, generateTimingTag("UNKNOWN_EVENT", acqTimestamp + 200'000, 0.0f, false)},
                },
                result.tags);
        }
        { // empty hw edge list
            TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
            auto          result = matcher.match(tags, std::vector<std::size_t>{}, 250uz, std::chrono::nanoseconds(acqTimestamp));
            expect(eq(result.processedTags, 3uz));
            expect(eq(result.processedSamples, 240uz));
            expectRangesEquals(std::vector<gr::Tag>{}, result.tags);
        }
        { // empty hw edge list
            TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
            auto          result = matcher.match(std::vector<gr::property_map>{}, std::vector<std::size_t>{}, 250uz, std::chrono::nanoseconds(acqTimestamp));
            expect(eq(result.processedTags, 0uz));
            expect(eq(result.processedSamples, 240uz));
            expectRangesEquals(std::vector<gr::Tag>{}, result.tags);
        }
    };

    "maxDelayGreaterChunkLength"_test = [&] { // a chunk length of 8 is less than the timout of 10us which corresponds to 10 samples -> if there is nothing matched, nothing will be consumed/published
        unsigned long acqTimestamp = 123456789;
        {
            std::vector<gr::property_map> tags{
                generateTimingTag("EVT_CMD1"s, acqTimestamp + 1'000, 0.0f, true),
                generateTimingTag("EVT_CMD3"s, acqTimestamp + 4'000, 0.0f, true),
            };
            std::vector<std::size_t> triggerSampleIndices{1, 4};

            TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
            auto          result = matcher.match(tags, triggerSampleIndices, 8uz, std::chrono::nanoseconds(acqTimestamp));

            expect(eq(2uz, result.processedTags));
            expect(eq(4uz, result.processedSamples)); // processed up to the last matching tag
            expectRangesEquals(
                std::vector<gr::Tag>{
                    {1, generateTimingTag("EVT_CMD1"s, acqTimestamp + 1'000, 0.0f, true)},
                    {4, generateTimingTag("EVT_CMD3"s, acqTimestamp + 4'000, 0.0f, true)},
                },
                result.tags);
        }
        {
            TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
            auto          result = matcher.match(std::vector<property_map>{}, std::vector<std::size_t>{}, 8uz, std::chrono::nanoseconds(acqTimestamp));

            expect(eq(0uz, result.processedTags));
            expect(eq(0uz, result.processedSamples));
            expectRangesEquals(std::vector<gr::Tag>{}, result.tags);
        }
    };

    "neighbour_tags"_test = [&] { // triggerSampleIndices = {100,101,102} with 3 tags
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1"s, acqTimestamp + 100'000, 0.0f, true),
            generateTimingTag("EVT_CMD2"s, acqTimestamp + 101'000, 0.0f, true),
            generateTimingTag("EVT_CMD3"s, acqTimestamp + 102'000, 0.0f, true),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 101, 102};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = 1e6f};
        auto          result = matcher.match(tags, triggerSampleIndices, 150uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(triggerSampleIndices.size(), result.processedTags));
        expect(eq(140uz, result.processedSamples));
        expectRangesEquals(
            std::vector<gr::Tag>{
                {100, generateTimingTag("EVT_CMD1"s, acqTimestamp + 100'000, 0.0f, true)},
                {101, generateTimingTag("EVT_CMD2"s, acqTimestamp + 101'000, 0.0f, true)},
                {102, generateTimingTag("EVT_CMD3"s, acqTimestamp + 102'000, 0.0f, true)},
            },
            result.tags);
    };

    "statePropagation"_test = [&] { // Multiple consecutive calls to the same instance to verify lastMatchedTag state propagates correctly between chunks.
        float                         sampleRate   = 1e6f;
        float                         Ts           = 1e9f / sampleRate;
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{
            generateTimingTag("EVT_CMD1"s, acqTimestamp + 100'000, 0.0f, true),
            generateTimingTag("EVT_CMD2"s, acqTimestamp + 150'000, 0.0f, true),
            // split (but the next hw edge is already in the data)
            generateTimingTag("EVT_CMD3"s, acqTimestamp + 200'000, 0.0f, true),
            generateTimingTag("EVT_CMD4"s, acqTimestamp + 250'000, 0.0f, true),
            generateTimingTag("EVT_CMD5"s, acqTimestamp + 300'000, 0.0f, true),
            // split (but the edge for the last event has not yet arrived)
            generateTimingTag("EVT_CMD6"s, acqTimestamp + 350'000, 0.0f, true),
        };
        std::vector<std::size_t> triggerSampleIndices{100, 150, 200, /*split*/ 250, /*split*/ 300, 350};

        TimingMatcher matcher{.timeout = 10us, .sampleRate = sampleRate};
        // chunk 1
        auto result = matcher.match(std::span(tags).subspan(0, 2), std::span(triggerSampleIndices).subspan(0, 3), 201uz, std::chrono::nanoseconds(acqTimestamp));
        expect(eq(2uz, result.processedTags));
        expect(eq(191uz, result.processedSamples));
        expectRangesEquals(
            std::vector<gr::Tag>{
                {100, generateTimingTag("EVT_CMD1"s, acqTimestamp + 100'000, 0.0f, true)},
                {150, generateTimingTag("EVT_CMD2"s, acqTimestamp + 150'000, 0.0f, true)},
            },
            result.tags);

        // chunk 2
        auto triggerSampleIndices2 = std::span(triggerSampleIndices).subspan(2, 2) | std::views::transform([&](std::size_t idx) { return idx - result.processedSamples; }) | std::ranges::to<std::vector>();
        auto result2               = matcher.match(std::span(tags).subspan(2, 3), triggerSampleIndices2, 115uz, std::chrono::nanoseconds(acqTimestamp + static_cast<std::size_t>(191.f * Ts)));
        expect(eq(2uz, result2.processedTags));
        expect(eq(105uz, result2.processedSamples));
        expectRangesEquals(
            std::vector<gr::Tag>{
                {9, generateTimingTag("EVT_CMD3"s, acqTimestamp + 200'000, 0.0f, true)},
                {59, generateTimingTag("EVT_CMD4"s, acqTimestamp + 250'000, 0.0f, true)},
            },
            result2.tags);
        // chunk 3
        matcher                    = TimingMatcher{.timeout = 10us, .sampleRate = sampleRate};
        auto triggerSampleIndices3 = std::span(triggerSampleIndices).subspan(4, 2) | std::views::transform([&](std::size_t idx) { return idx - result.processedSamples - result2.processedSamples; }) | std::ranges::to<std::vector>();
        auto result3               = matcher.match(std::span(tags).subspan(4, 2), triggerSampleIndices3, 104uz, std::chrono::nanoseconds(acqTimestamp + static_cast<std::size_t>((191.f + 105.f) * Ts)));
        expect(eq(2uz, result3.processedTags));
        expect(eq(94uz, result3.processedSamples));
        expectRangesEquals(
            std::vector<gr::Tag>{
                {4, generateTimingTag("EVT_CMD5"s, acqTimestamp + 300'000, 0.0f, true)},
                {54, generateTimingTag("EVT_CMD6"s, acqTimestamp + 350'000, 0.0f, true)},
            },
            result3.tags);
    };
};
} // namespace fair::picoscope::test

int main() { /* tests are statically executed */ }
