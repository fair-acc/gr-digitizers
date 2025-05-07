#ifndef GR_DIGITIZERS_TIMINGMATCHER_HPP
#define GR_DIGITIZERS_TIMINGMATCHER_HPP
#include <gnuradio-4.0/Buffer.hpp>
#include <gnuradio-4.0/Tag.hpp>

namespace fair::picoscope::timingmatcher {
using namespace std::chrono_literals;

struct MatcherResult {
    std::size_t          processedTags    = 0;
    std::size_t          processedSamples = 0;
    std::vector<gr::Tag> tags{};
};

/**
 * matches each tag from a span of tags to a corresponding trigger pulse.
 *
 *  idx                  123456789012345678901234567890
 *                             ┌─┐       ┌─┐      ┌──┐
 *  pulse                ──────┘ └───────┘ └──────┘  └─
 *  triggerSampleIndices       2                  3
 *  tag                 x   x  x    x             xx       x
 *  tag0 ───────────────┘   │  │    │             ││       │  outdated tag: cannot be matched anymore and has to be dropped
 *  evtA ───────────────────┘  │    │             ││       │  tag without corresponding pulse, tag["HW-TRIGGER"] = false and no existing matched tag
 *  evt1 ──────────────────────┘    │             ││       │  regular tag with a corresponding hw edge
 *  evtB ───────────────────────────┘             ││       │  tag without corresponding pulse, tag["HW-TRIGGER"] = false but a tag was matched before
 *                                       !        ││       │  pulse without a tag metadata -> EVENT_UNKNOWN
 *  evt4 ─────────────────────────────────────────┘│       │  first of two overlapping hardware pulses can be treated like a regular pulse
 *  evt5 ──────────────────────────────────────────┘       │  trigger pulses overlap -> position tag relative to last matched tag
 *  evt6 ──────────────────────────────────────────────────┘  corresponding pulse not yet occurred, has to be retained for the next chunk of data
 */
struct TimingMatcher {
    std::chrono::nanoseconds maxDelay;
    float                    sampleRate = 1000.f;

    std::optional<std::pair<std::ptrdiff_t, std::uint64_t>> _lastMatchedTag; // last successfully matched tag: idx relative to the next unprocessed chunk -> time in ns

    /***
     * verifies that a timing tag contains all the required fields with correct types, so we do not have to have checks everywhere in the matcher code
     */
    static bool checkValidTimingTag(const gr::property_map& tag) {
        constexpr std::array requiredKeys{
            gr::tag::TRIGGER_TIME.shortKey(),
            gr::tag::TRIGGER_OFFSET.shortKey(),
            gr::tag::TRIGGER_NAME.shortKey(),
            gr::tag::TRIGGER_META_INFO.shortKey(),
        };
        if (!std::ranges::all_of(requiredKeys, [&tag](auto& k) { return tag.contains(k); })) {
            return false;
        }
        auto& metaVariant = tag.at(gr::tag::TRIGGER_META_INFO.shortKey());
        if (!std::holds_alternative<gr::property_map>(metaVariant)) {
            return false;
        }
        auto&                metaMap = std::get<gr::property_map>(metaVariant);
        constexpr std::array requiredMetaKeys{"LOCAL-TIME", "HW-TRIGGER"};
        if (!std::ranges::all_of(requiredMetaKeys, [&metaMap](auto& k) { return metaMap.contains(k); })) {
            return false;
        }
        return true;
    }

    static gr::Tag createUnknownEventTag(unsigned long index, std::chrono::nanoseconds currentFlankTime) {
        return {index, gr::property_map{
                           {gr::tag::TRIGGER_NAME.shortKey(), "UNKNOWN_EVENT"},
                           {gr::tag::TRIGGER_TIME.shortKey(), static_cast<std::size_t>(currentFlankTime.count())},
                           {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f},
                           {gr::tag::TRIGGER_META_INFO.shortKey(),
                               gr::property_map{
                                   {"LOCAL-TIME", static_cast<std::size_t>(currentFlankTime.count())},
                                   {"HW-TRIGGER", false},
                               }},
                       }};
    }

    gr::Tag alignTagRelativeToLastMatched(const gr::property_map& currentTag) {
        float      Ts                                     = 1e9f / sampleRate;
        const auto currentTagWRTime                       = std::chrono::nanoseconds(std::get<unsigned long>(currentTag.at(gr::tag::TRIGGER_TIME.shortKey())));
        const auto currentTagOffset                       = std::chrono::nanoseconds(static_cast<unsigned long>(std::get<float>(currentTag.at(gr::tag::TRIGGER_OFFSET.shortKey()))));
        auto [lastIdx, lastTime]                          = *_lastMatchedTag;
        auto             deltaTime                        = currentTagWRTime + currentTagOffset - std::chrono::nanoseconds(lastTime);
        auto             delta                            = static_cast<float>(deltaTime.count()) / Ts;
        auto             deltaIdx                         = static_cast<long>(delta);
        auto             deltaOffset                      = delta - static_cast<float>(deltaIdx);
        auto             idx                              = lastIdx + deltaIdx;
        gr::property_map matchedTag                       = currentTag;
        matchedTag.at(gr::tag::TRIGGER_OFFSET.shortKey()) = deltaOffset;
        return {static_cast<std::size_t>(idx), matchedTag};
    }

    gr::Tag getOffsetAdjustedTag(auto currentFlankIndex, auto currentTagOffset, const gr::property_map& tagMap) {
        gr::property_map matchedTagMap = tagMap;
        auto             deltaT        = static_cast<float>(currentTagOffset.count()) * (sampleRate / 1e9f);
        auto             offsetIdx     = static_cast<long>(deltaT);
        auto             offset        = deltaT - static_cast<float>(offsetIdx);
        if (offset < 0.0f) {
            offsetIdx--;
            offset += 1.0f;
        }
        matchedTagMap.at(gr::tag::TRIGGER_OFFSET.shortKey()) = offset;
        return {currentFlankIndex + offsetIdx, matchedTagMap};
    }

    MatcherResult match(const std::span<const gr::property_map> tags, const std::span<const std::size_t>& triggerSampleIndices, const std::size_t nSamples, const std::chrono::nanoseconds localAcqTime) {
        MatcherResult result;
        std::size_t   triggerIndex    = 0;
        std::size_t   unmatchedEvents = 0; // number of events hat have to be adjusted based on the next trigger that is found
        float         Ts              = 1e9f / sampleRate;
        while (result.processedTags < tags.size() || triggerIndex < triggerSampleIndices.size()) { // keep processing as long as there is either unprocessed pulses or hw edges
            if (result.processedTags >= tags.size()) {                                             // all event tags processed, checking for potential hw edges
                const auto currentFlankIndex = triggerSampleIndices[triggerIndex];
                const auto currentFlankTime  = localAcqTime + std::chrono::nanoseconds(static_cast<unsigned long>(1e9f / sampleRate * static_cast<float>(currentFlankIndex)));
                if (currentFlankIndex < nSamples - static_cast<std::size_t>(static_cast<float>(std::chrono::nanoseconds(maxDelay).count()) * 1e-9f * sampleRate)) {
                    // unmatched hw edge found and deadline for tag to arrive has passed -> publish UNKNOWN_EVENT // evtX
                    result.tags.push_back(createUnknownEventTag(currentFlankIndex, currentFlankTime));
                    triggerIndex++;
                    continue;
                } else { // hw-edge is too recent -> stop processing and check in the next invocation if we have a matching event tag
                    break;
                }
            }
            const std::size_t       tagIndex   = result.processedTags + unmatchedEvents;
            const gr::property_map& currentTag = tags[tagIndex];
            if (!checkValidTimingTag(currentTag)) {
                result.processedTags++;
                continue;
            }
            auto&      metaMap             = std::get<gr::property_map>(currentTag.at(gr::tag::TRIGGER_META_INFO.shortKey()));
            const auto currentTagLocalTime = std::chrono::nanoseconds(std::get<unsigned long>(metaMap.at("LOCAL-TIME")));
            const auto currentTagWRTime    = std::chrono::nanoseconds(std::get<unsigned long>(currentTag.at(gr::tag::TRIGGER_TIME.shortKey())));
            const auto currentTagOffset    = std::chrono::nanoseconds(static_cast<unsigned long>(std::get<float>(currentTag.at(gr::tag::TRIGGER_OFFSET.shortKey()))));
            if (triggerIndex >= triggerSampleIndices.size()) {                                                                                            // there are remaining events, but no more hw edges to match
                if ((currentTagLocalTime + maxDelay) < (localAcqTime + std::chrono::nanoseconds(static_cast<long>(static_cast<float>(nSamples) * Ts)))) { // we are sure the hw edge cannot still arrive
                    if (_lastMatchedTag) {                                                                                                                // publish tags based on last matched trigger
                        result.tags.push_back(alignTagRelativeToLastMatched(currentTag));
                    }
                    result.processedTags++;
                    continue;
                } else { // tag will not be consumed and handled in the next iteration
                    break;
                }
            }
            // below we still have tags and hw edges to match
            const auto currentFlankIndex = triggerSampleIndices[triggerIndex];
            const auto currentFlankTime  = localAcqTime + std::chrono::nanoseconds(static_cast<unsigned long>(1e9f / sampleRate * static_cast<float>(currentFlankIndex)));

            if ((currentTagLocalTime + maxDelay) < localAcqTime) { // outdated tag -> drop // evt0 in diagram
                result.processedTags++;
                continue;
            }

            if (!std::get<bool>(metaMap.at("HW-TRIGGER")) || (currentTagLocalTime + maxDelay) < currentFlankTime) {
                // event either explicitly has no hardware trigger or the next hw edge is too far away
                if (_lastMatchedTag) { // publish tags based on last matched trigger // evtB or evt5 in diagram
                    result.tags.push_back(alignTagRelativeToLastMatched(currentTag));
                    result.processedTags++;
                } else { // there is currently no previous matched tag, keep tags to attach after the next trigger was detected // evtA in diagram
                    unmatchedEvents++;
                }
                continue;
            }

            if ((currentFlankTime + maxDelay) < currentTagLocalTime) { // hw edge without corresponding event
                result.tags.push_back(createUnknownEventTag(currentFlankIndex, currentFlankTime));
                triggerIndex++; // skip outdated timing message(s)
                continue;
            }

            // regular case, next hw edge belongs to the next tag
            _lastMatchedTag = {currentFlankIndex, std::chrono::nanoseconds(currentFlankTime).count()};
            while (unmatchedEvents > 0) { // align all previously unaligned tags relative to this one
                result.tags.push_back(alignTagRelativeToLastMatched(tags[tagIndex - unmatchedEvents]));
                unmatchedEvents--;
            }
            result.tags.emplace_back(getOffsetAdjustedTag(currentFlankIndex, currentTagOffset, currentTag));
            result.processedTags++;
            triggerIndex++;
        } // (result.processedTags < tags.size() || triggerIndex < triggerSampleIndices.size())

        // calculate the sample index up until where all matching is finished
        auto maxDelaySamples    = static_cast<std::size_t>(static_cast<float>(std::chrono::nanoseconds(maxDelay).count()) * 1e-9f * sampleRate);
        result.processedSamples = 0;
        if (triggerSampleIndices.size() > triggerIndex - 1) {
            result.processedSamples = std::max(result.processedSamples, triggerSampleIndices[triggerIndex - 1] - 1); // process at least until the sample before the last matched trigger
        }
        if (nSamples > maxDelaySamples) {
            result.processedSamples = std::max(result.processedSamples, nSamples - maxDelaySamples); // consume all samples up to the deadline
        }

        if (_lastMatchedTag) { // re-adjust the last matched tag index relative to the start of the next sample chunk
            _lastMatchedTag->first -= static_cast<long>(result.processedSamples);
        }

        return result;
    }
};

} // namespace fair::picoscope::timingmatcher

#endif // GR_DIGITIZERS_TIMINGMATCHER_HPP
