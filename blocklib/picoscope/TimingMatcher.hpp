#ifndef GR_DIGITIZERS_TIMINGMATCHER_HPP
#define GR_DIGITIZERS_TIMINGMATCHER_HPP
#include <gnuradio-4.0/Buffer.hpp>
#include <gnuradio-4.0/Tag.hpp>

namespace fair::picoscope::timingmatcher {
using namespace std::chrono_literals;

class TimingMatcher {
    // state
    std::optional<std::pair<long, std::size_t>> lastMatchedTag; // last successfully matched tag: idx relative to the next unprocessed chunk -> time in ns
public:
    // settings
    std::chrono::nanoseconds maxDelay;
    float                    sampleRate = 1000;

    static bool checkValidTimingTag(const gr::property_map& tag) {
        if (!tag.contains(gr::tag::TRIGGER_TIME.shortKey())) {
            return false;
        }
        if (!tag.contains(gr::tag::TRIGGER_OFFSET.shortKey())) {
            return false;
        }
        if (!tag.contains(gr::tag::TRIGGER_NAME.shortKey())) {
            return false;
        }
        if (!tag.contains("LOCAL-TIME")) {
            return false;
        }
        if (!tag.contains("HW-TRIGGER")) {
            return false;
        }
        return true;
    }

    /**
     * matches each tag from a span of tags to a corresponding trigger pulse
     *  idx                  123456789012345678901234567
     *                        ┌─┐    ┌─┐  ┌─┐      ┌──┐
     *  pulse                ─┘ └────┘ └──┘ └──────┘  └─
     *  triggerSampleIndices  2      9    4        3
     *  tag            x      x      x    x   x    xx       x
     *  tag0 ──────────┘      |      |    |   |    ||       | note: outdated tag: cannot be matched anymore and has to be dropped
     *  evt1 ─────────────────┘      |    |   |    ||       |
     *  evt2 ────────────────────────┘    |   |    ||       |
     *  evt3 ─────────────────────────────┘   |    ||       |
     *  evtA ─────────────────────────────────┘    ||       | note: tag without corresponding pulse, tag["HW-TRIGGER"] = false
     *  evt4 ──────────────────────────────────────┘|       |
     *  evt5 ───────────────────────────────────────┘       | note: trigger pulses overlap -> position tag relative to last matched tag
     *  evt5 ───────────────────────────────────────────────┘ note: corresponding pulse not yet occured, has to be retained for the next chunk of data
     *
     *  This routine returns the index of the next unmatched triggerSample and sets the tag indices for all matched tags in the supplied tagIndices vector.
     *  tags: a span of tags that should be matched
     *
     */
    template<bool verbose = false>
    auto match(std::span<gr::property_map> tags, std::vector<std::size_t>& triggerSampleIndices, std::size_t nSamples, std::chrono::nanoseconds local_acq_time) {
        struct matcherResult {
            std::size_t                                           processedTags    = 0;
            std::size_t                                           processedSamples = 0;
            std::vector<std::pair<std::size_t, gr::property_map>> tags{};
        } result;
        if constexpr (verbose) {
            fmt::print("=== starting matcher ===\n");
        }
        std::size_t trigger_index    = 0;
        std::size_t unmatched_events = 0; // number of events hat have to be adjusted based on the next trigger that is found
        while (result.processedTags < tags.size() || trigger_index < triggerSampleIndices.size()) {
            if (result.processedTags >= tags.size()) {
                const auto currentFlankIndex = triggerSampleIndices[trigger_index];
                const auto currentFlankTime  = local_acq_time + std::chrono::nanoseconds(static_cast<unsigned long>(1e9f / sampleRate * static_cast<float>(currentFlankIndex)));
                if (currentFlankIndex < nSamples - static_cast<std::size_t>(static_cast<float>(std::chrono::nanoseconds(maxDelay).count()) * 1e-9f * sampleRate)) {
                    result.tags.push_back({currentFlankIndex, gr::property_map{
                                                                  {gr::tag::TRIGGER_NAME.shortKey(), "UNKNOWN_EVENT"},
                                                                  {gr::tag::TRIGGER_TIME.shortKey(), static_cast<std::size_t>(currentFlankTime.count())},
                                                                  {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f},
                                                                  {"LOCAL-TIME", static_cast<std::size_t>(currentFlankTime.count())},
                                                                  {"HW-TRIGGER", false},
                                                              }});
                    trigger_index++;
                    continue;
                } else {
                    break;
                }
            }
            const std::size_t       tag_index  = result.processedTags + unmatched_events;
            const gr::property_map& currentTag = tags[tag_index];
            if (!checkValidTimingTag(currentTag)) {
                result.processedTags++;
                continue;
            }
            const auto currentTagLocalTime = std::chrono::nanoseconds(std::get<unsigned long>(currentTag.at("LOCAL-TIME")));
            const auto currentTagWRTime    = std::chrono::nanoseconds(std::get<unsigned long>(currentTag.at(gr::tag::TRIGGER_TIME.shortKey())));
            const auto currentTagOffset    = std::chrono::nanoseconds(static_cast<unsigned long>(std::get<float>(currentTag.at(gr::tag::TRIGGER_OFFSET.shortKey()))));
            if (trigger_index >= triggerSampleIndices.size()) {
                if (currentTagLocalTime + maxDelay < std::chrono::nanoseconds(static_cast<long>(static_cast<float>(nSamples) * (1e9f / sampleRate))) + local_acq_time) {
                    if constexpr (verbose) {
                        fmt::print("tag present but no more pulses and deadline already expired: {}, tag+max_delay {}, lastSample {}\n", currentTag, currentTagLocalTime + maxDelay, std::chrono::nanoseconds(static_cast<long>(static_cast<float>(nSamples) * (1e9f / sampleRate))) + local_acq_time);
                    }
                    if (lastMatchedTag) { // publish tags based on last matched trigger
                        auto lastIdx     = lastMatchedTag->first;
                        auto lastTime    = std::chrono::nanoseconds(lastMatchedTag->second);
                        auto deltaTime   = currentTagWRTime + currentTagOffset - lastTime;
                        auto delta       = static_cast<double>(deltaTime.count()) * 1e-9 * static_cast<double>(sampleRate);
                        auto deltaIdx    = static_cast<long>(delta);
                        auto deltaOffset = static_cast<float>(delta - static_cast<double>(deltaIdx));
                        auto idx         = lastIdx + deltaIdx;
                        result.tags.push_back({idx, currentTag});
                        result.tags.back().second.at(gr::tag::TRIGGER_OFFSET.shortKey()) = deltaOffset;
                    }
                    result.processedTags++;
                    continue;
                } else {
                    break;
                }
            }
            const auto currentFlankIndex = triggerSampleIndices[trigger_index];
            const auto currentFlankTime  = local_acq_time + std::chrono::nanoseconds(static_cast<unsigned long>(1e9f / sampleRate * static_cast<float>(currentFlankIndex)));

            if (currentTagLocalTime + maxDelay < local_acq_time) {
                if constexpr (verbose) {
                    fmt::print("outdated tag: {}\n", tags[result.processedTags]);
                }
                // result.tags.push_back({0uz, {}}); // TODO: two options: drop and publish warning tag or publish with negative offset?
                result.processedTags++;
                continue;
            }
            if (!std::get<bool>(currentTag.at("HW-TRIGGER")) || currentTagLocalTime + maxDelay < currentFlankTime) {
                if constexpr (verbose) {
                    fmt::print("tag without trigger: {}\n", currentTag);
                }
                if (lastMatchedTag) { // publish tags based on last matched trigger
                    auto lastIdx     = lastMatchedTag->first;
                    auto lastTime    = std::chrono::nanoseconds(lastMatchedTag->second);
                    auto deltaTime   = currentTagWRTime + currentTagOffset - lastTime;
                    auto delta       = static_cast<double>(deltaTime.count()) * 1e-9 * static_cast<double>(sampleRate);
                    auto deltaIdx    = static_cast<long>(delta);
                    auto deltaOffset = static_cast<float>(delta - static_cast<double>(deltaIdx));
                    auto idx         = lastIdx + deltaIdx;
                    result.tags.push_back({idx, currentTag});
                    result.tags.back().second.at(gr::tag::TRIGGER_OFFSET.shortKey()) = deltaOffset;
                    result.processedTags++;
                } else { // there is currently not matched tags, keep tags to attach after the next trigger was detected
                    unmatched_events++;
                }
                continue;
            }
            if ((currentFlankTime + maxDelay) < currentTagLocalTime) {
                if constexpr (verbose) {
                    fmt::print("trigger pulse with no event: {}, local: {}, offset: {}, next tag: {}\n", triggerSampleIndices[trigger_index], local_acq_time, static_cast<unsigned long>(1e-9f * sampleRate * static_cast<float>(triggerSampleIndices[trigger_index])), std::get<unsigned long>(tags[tag_index]["LOCAL-TIME"]));
                }
                result.tags.push_back({currentFlankIndex, gr::property_map{
                                                              {gr::tag::TRIGGER_NAME.shortKey(), "UNKNOWN_EVENT"},
                                                              {gr::tag::TRIGGER_TIME.shortKey(), static_cast<std::size_t>(currentFlankTime.count())},
                                                              {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f},
                                                              {"LOCAL-TIME", static_cast<std::size_t>(currentFlankTime.count())},
                                                              {"HW-TRIGGER", false},
                                                          }});
                trigger_index++; // skip outdated timing message(s)
                continue;
            }
            // regular case, just match next tags
            if constexpr (verbose) {
                fmt::print("regular tag at {} -> {}: {}\n", triggerSampleIndices[trigger_index], (local_acq_time + std::chrono::nanoseconds(static_cast<unsigned long>(1e9f / sampleRate * static_cast<float>(triggerSampleIndices[trigger_index]))) + maxDelay), tags[tag_index]);
            }

            auto deltaT    = static_cast<float>(currentTagOffset.count()) * (sampleRate / 1e9f);
            auto offsetIdx = static_cast<long>(deltaT);
            auto offset    = deltaT - static_cast<float>(offsetIdx);
            if (offset < 0.0f) {
                offsetIdx--;
                offset += 1.0f;
            }
            result.tags.push_back({currentFlankIndex + offsetIdx, currentTag});
            result.tags.back().second.at(gr::tag::TRIGGER_OFFSET.shortKey()) = offset;
            lastMatchedTag                                                   = {currentFlankIndex, std::chrono::nanoseconds(currentFlankTime).count()};
            unmatched_events                                                 = 0;
            result.processedTags++;
            trigger_index++;
        }
        result.processedSamples = std::max(triggerSampleIndices[trigger_index - 1], nSamples - static_cast<std::size_t>(static_cast<float>(std::chrono::nanoseconds(maxDelay).count()) * 1e-9f * sampleRate));

        if (verbose) {
            fmt::print("processed {} tags and {} samples, resultTags: \n", result.processedTags, result.processedSamples);
            std::ranges::for_each(result.tags, [](auto a) { fmt::print("  {} -> {{ {} }}\n", a.first, a.second); });
        }
        return result;
    }
};

} // namespace fair::picoscope::timingmatcher

#endif // GR_DIGITIZERS_TIMINGMATCHER_HPP
