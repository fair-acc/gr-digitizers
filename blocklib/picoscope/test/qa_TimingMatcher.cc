#include <TimingMatcher.hpp>
#include <boost/ut.hpp>

using namespace std::string_literals;
using namespace std::chrono_literals;

namespace fair::picoscope::test {

const boost::ut::suite<"TimingMatchers"> TimingMatcherTests = [] {
    using namespace boost::ut;
    using namespace gr;
    using namespace fair::picoscope;
    using fair::picoscope::timingmatcher::TimingMatcher;

    "simpleMatching"_test = [] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{{{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}};
        std::vector<std::size_t>      triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher;
        matcher.maxDelay   = 10us;
        matcher.sampleRate = 1e6f;
        auto result        = matcher.match<true>(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(triggerSampleIndices.size(), result.processedTags));
        expect(eq(240uz, result.processedSamples));
        expect(std::ranges::equal(std::vector<std::pair<std::size_t, gr::property_map>>{{100, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}}, {150, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}}, {200, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}}}, result.tags));
    };

    "differentStartTimes-TimingFirst"_test = [] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{{{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMDA"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 10'200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp - 10'200'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMDB"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 10'150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp - 10'150'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}};
        std::vector<std::size_t>      triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher;
        matcher.maxDelay   = 10us;
        matcher.sampleRate = 1e6f;
        auto result        = matcher.match<true>(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(5uz, result.processedTags));
        expect(eq(240uz, result.processedSamples));
        expect(std::ranges::equal(std::vector<std::pair<std::size_t, gr::property_map>>{{100, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}}, {150, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}}, {200, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}}}, result.tags));
    };

    "differentStartTimes-PulsesFirst"_test = [] {
        unsigned long                 acqTimestamp = 1000000;
        std::vector<gr::property_map> tags{{{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 1'100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 1'100'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 1'150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 1'150'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 1'200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 1'200'000}, {"HW-TRIGGER", true}}};
        std::vector<std::size_t>      triggerSampleIndices{100, 150, 200, 1'100, 1'150, 1'200};

        TimingMatcher matcher;
        matcher.maxDelay   = 10us;
        matcher.sampleRate = 1e6f;
        auto result        = matcher.match<true>(tags, triggerSampleIndices, 1'250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(3uz, result.processedTags));
        expect(eq(1'240uz, result.processedSamples));
        std::vector<std::pair<std::size_t, gr::property_map>> expected{{100, {{gr::tag::TRIGGER_NAME.shortKey(), "UNKNOWN_EVENT"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", false}}}, {150, {{gr::tag::TRIGGER_NAME.shortKey(), "UNKNOWN_EVENT"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", false}}}, {200, {{gr::tag::TRIGGER_NAME.shortKey(), "UNKNOWN_EVENT"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", false}}}, {1'100, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 1'100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 1'100'000}, {"HW-TRIGGER", true}}}, {1'150, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 1'150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 1'150'000}, {"HW-TRIGGER", true}}}, {1'200, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 1'200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 1'200'000}, {"HW-TRIGGER", true}}}};
        // fmt::print("expectedTags: \n");
        // std::ranges::for_each(expected, [](auto a) {fmt::print("  {} -> {{ {} }}\n", a.first, a.second);});
        // for (const auto & [exp, got] : std::ranges::views::zip(expected, result.tags)) {
        //     expect(eq(exp.first, got.first));
        //     bool equal = exp.second == got.second;
        //     expect(equal) << [&exp, &got]() {return fmt::format("expected: {}, got {}\n", exp.second, got.second);};
        // }
        expect(std::ranges::equal(expected, result.tags));
    };

    "overlappingEvents"_test = [] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{{{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2b"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 151'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 151'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}};
        std::vector<std::size_t>      triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher;
        matcher.maxDelay   = 10us;
        matcher.sampleRate = 1e6f;
        auto result        = matcher.match<true>(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(4uz, result.processedTags));
        expect(eq(240uz, result.processedSamples));
        expect(approx(std::get<float>(result.tags[2].second.at(gr::tag::TRIGGER_OFFSET.shortKey())), 0.0f, 1e-10f));
        result.tags[2].second.at(gr::tag::TRIGGER_OFFSET.shortKey()) = 0.0f;

        expect(std::ranges::equal(std::vector<std::pair<std::size_t, gr::property_map>>{{100, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}}, {150, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}}, {151, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2b"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 151'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 151'000}, {"HW-TRIGGER", true}}}, {200, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}}}, result.tags));
    };

    "multiEvents"_test = [] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{{{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2b"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}};
        std::vector<std::size_t>      triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher;
        matcher.maxDelay   = 10us;
        matcher.sampleRate = 1e6f;
        auto result        = matcher.match<true>(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(4uz, result.processedTags));
        expect(eq(240uz, result.processedSamples));

        expect(std::ranges::equal(std::vector<std::pair<std::size_t, gr::property_map>>{{100, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}}, {150, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}}, {150, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2b"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}}, {200, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}}}, result.tags));
    };

    "tagWithoutTrigger"_test = [] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{{{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMDA"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 180'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 180'000}, {"HW-TRIGGER", false}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}};
        std::vector<std::size_t>      triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher;
        matcher.maxDelay   = 10us;
        matcher.sampleRate = 1e6f;
        auto result        = matcher.match<true>(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(4uz, result.processedTags));
        expect(eq(240uz, result.processedSamples));

        expect(std::ranges::equal(std::vector<std::pair<std::size_t, gr::property_map>>{{100, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}}, {150, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}}, {180, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMDA"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 180'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 180'000}, {"HW-TRIGGER", false}}}, {200, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}}}, result.tags));
    };

    "tagWithMissingTrigger"_test = [] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{{{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMDA"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 180'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 180'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}};
        std::vector<std::size_t>      triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher;
        matcher.maxDelay   = 10us;
        matcher.sampleRate = 1e6f;
        auto result        = matcher.match<true>(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(4uz, result.processedTags));
        expect(eq(240uz, result.processedSamples));

        expect(std::ranges::equal(std::vector<std::pair<std::size_t, gr::property_map>>{{100, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}}, {150, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}}, {180, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMDA"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 180'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 180'000}, {"HW-TRIGGER", true}}}, {200, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}}}, result.tags));
        // TODO: how to communicate that the trigger is missing?
    };

    "futurePulses"_test = [] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{{{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}};
        std::vector<std::size_t>      triggerSampleIndices{100, 150, 200, 300, 2'000};

        TimingMatcher matcher;
        matcher.maxDelay   = 10us;
        matcher.sampleRate = 1e6f;
        auto result        = matcher.match<true>(tags, triggerSampleIndices, 2'001uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(3uz, result.processedTags));
        expect(eq(1991uz, result.processedSamples));

        expect(std::ranges::equal(std::vector<std::pair<std::size_t, gr::property_map>>{{100, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}}, {150, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}}, {200, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}}, {300, {{gr::tag::TRIGGER_NAME.shortKey(), "UNKNOWN_EVENT"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 300'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 300'000}, {"HW-TRIGGER", false}}}}, result.tags));
        // TODO: think about having two different timeouts, one for matching and one for keeping events around for later realignment
    };

    "differentClocks"_test = [] {
        unsigned long                 wrTimestamp  = 23456789;
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{{{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), wrTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), wrTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), wrTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}};
        std::vector<std::size_t>      triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher;
        matcher.maxDelay   = 10us;
        matcher.sampleRate = 1e6f;
        auto result        = matcher.match<true>(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(3uz, result.processedTags));
        expect(eq(240uz, result.processedSamples));

        expect(std::ranges::equal(std::vector<std::pair<std::size_t, gr::property_map>>{{100, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), wrTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}}, {150, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), wrTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}}, {200, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), wrTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}}}, result.tags));
    };

    "futureEvents"_test = [] {
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{{{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD4"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 234'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 234'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD5"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 253'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 253'000}, {"HW-TRIGGER", true}}};
        std::vector<std::size_t>      triggerSampleIndices{100, 150, 200};

        TimingMatcher matcher;
        matcher.maxDelay   = 10us;
        matcher.sampleRate = 1e6f;
        auto result        = matcher.match<true>(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(4uz, result.processedTags));
        expect(eq(240uz, result.processedSamples));

        expect(std::ranges::equal(std::vector<std::pair<std::size_t, gr::property_map>>{{100, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 100'000}, {"HW-TRIGGER", true}}}, {150, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 150'000}, {"HW-TRIGGER", true}}}, {200, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}}, {234, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD4"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 234'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 234'000}, {"HW-TRIGGER", true}}}}, result.tags));
    };

    "compensateOffset"_test = [] {
        // since the generated trigger pulses are generated with a small offset compared to the actual event, they have to be shifted accordingly
        // to this end in the raw tags, `TRIGGER_TIME` contains the time in ns that the event has to be shifted compared to the pulse.
        // The matching algorithm has to account for this shift when determining the correct sample and has to correct the `TRIGGER_OFFSET` field according to the sampling rate and sample-relative calculated position
        unsigned long                 acqTimestamp = 123456789;
        std::vector<gr::property_map> tags{{{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), -1e3f}, {"LOCAL-TIME", acqTimestamp + 101'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), -2e4f}, {"LOCAL-TIME", acqTimestamp + 170'000}, {"HW-TRIGGER", true}}, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), -3e2f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}};
        std::vector<std::size_t>      triggerSampleIndices{101, 170, 200};

        TimingMatcher matcher;
        matcher.maxDelay   = 10us;
        matcher.sampleRate = 1e6f;
        auto result        = matcher.match<true>(tags, triggerSampleIndices, 250uz, std::chrono::nanoseconds(acqTimestamp));

        expect(eq(triggerSampleIndices.size(), result.processedTags));
        expect(eq(240uz, result.processedSamples));

        // check and fix inexact offsets
        expect(approx(std::get<float>(result.tags[0].second.at(gr::tag::TRIGGER_OFFSET.shortKey())), 0.0f, 1e-10f));
        result.tags[0].second.at(gr::tag::TRIGGER_OFFSET.shortKey()) = 0.0f;
        expect(approx(std::get<float>(result.tags[1].second.at(gr::tag::TRIGGER_OFFSET.shortKey())), 0.0f, 1e-10f));
        result.tags[1].second.at(gr::tag::TRIGGER_OFFSET.shortKey()) = 0.0f;
        expect(approx(std::get<float>(result.tags[2].second.at(gr::tag::TRIGGER_OFFSET.shortKey())), 0.7f, 1e-10f));
        result.tags[2].second.at(gr::tag::TRIGGER_OFFSET.shortKey()) = 0.0f;

        expect(std::ranges::equal(std::vector<std::pair<std::size_t, gr::property_map>>{{100, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD1"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 100'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 101'000}, {"HW-TRIGGER", true}}}, {150, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD2"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 150'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 170'000}, {"HW-TRIGGER", true}}}, {199, {{gr::tag::TRIGGER_NAME.shortKey(), "EVT_CMD3"}, {gr::tag::TRIGGER_TIME.shortKey(), acqTimestamp + 200'000}, {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f}, {"LOCAL-TIME", acqTimestamp + 200'000}, {"HW-TRIGGER", true}}}}, result.tags));
    };
};
} // namespace fair::picoscope::test

int main() { /* tests are statically executed */ }
