#ifndef GR_DIGITIZERS_TIMINGMATCHER_HPP
#define GR_DIGITIZERS_TIMINGMATCHER_HPP
#include <gnuradio-4.0/Buffer.hpp>
#include <gnuradio-4.0/Tag.hpp>

namespace fair::picoscope::timingmatcher {
using namespace std::chrono_literals;

struct MatcherResult {
    std::size_t              processedTags    = 0;
    std::size_t              processedSamples = 0;
    std::vector<gr::Tag>     tags{};
    std::vector<std::string> messages{}; // diagnostic or error messages
};

/**
 * matches each tag from a span of tags to a corresponding trigger pulse.
 *
 *  idx                  123456789012345678901234567890
 *                             ┌─┐       ┌─┐      ┌──┐
 *  pulse                ──────┘ └───────┘ └──────┘  └─
 *  triggerSampleIndices       2                  3
 *  tag                 x   x  x    x             xx       x
 *  tag0 ───────────────┘   │  │    │             ││       │  outdated tag: cannot be matched any more and has to be dropped
 *  evtA ───────────────────┘  │    │             ││       │  tag without corresponding pulse, tag["HW-TRIGGER"] = false and no existing matched tag
 *  evt1 ──────────────────────┘    │             ││       │  regular tag with a corresponding hw edge
 *  evtB ───────────────────────────┘             ││       │  tag without corresponding pulse, tag["HW-TRIGGER"] = false but a tag was matched before
 *                                       !        ││       │  pulse without a tag metadata -> EVENT_UNKNOWN
 *  evt4 ─────────────────────────────────────────┘│       │  first of two overlapping hardware pulses can be treated like a regular pulse
 *  evt5 ──────────────────────────────────────────┘       │  trigger pulses overlap -> position tag relative to the last matched tag
 *  evt6 ──────────────────────────────────────────────────┘  corresponding pulse not yet occurred, has to be retained for the next chunk of data
 */
struct TimingMatcher {
    std::chrono::nanoseconds timeout;
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
        auto* metaMapPtr  = metaVariant.get_if<gr::property_map>();
        if (!metaMapPtr) {
            return false;
        }
        auto&                metaMap = *metaMapPtr;
        constexpr std::array requiredMetaKeys{"LOCAL-TIME", "HW-TRIGGER"};
        if (!std::ranges::all_of(requiredMetaKeys, [&metaMap](auto& k) { return metaMap.contains(k); })) {
            return false;
        }
        return true;
    }

    static gr::Tag createUnknownEventTag(unsigned long index, std::chrono::nanoseconds currentFlankTime) {
        return {index, gr::property_map{
                           {gr::tag::TRIGGER_NAME.shortKey(), "UNKNOWN_EVENT"},
                           // TODO: As for the unmatched trigger only the local time is known, the WR time would have to be realigned based on last_matched_tag
                           // {gr::tag::TRIGGER_TIME.shortKey(), static_cast<std::size_t>(currentFlankTime.count())},
                           {gr::tag::TRIGGER_OFFSET.shortKey(), 0.0f},
                           {gr::tag::TRIGGER_META_INFO.shortKey(),
                               gr::property_map{
                                   {"LOCAL-TIME", static_cast<std::size_t>(currentFlankTime.count())},
                                   {"HW-TRIGGER", false},
                               }},
                       }};
    }

    std::optional<gr::Tag> alignTagRelativeToLastMatched(const gr::property_map& currentTag) {
        if (!_lastMatchedTag.has_value()) {
            return std::nullopt;
        }
        float      Ts               = 1e9f / sampleRate;
        const auto maybeTriggerTime = currentTag.at(gr::tag::TRIGGER_TIME.shortKey()).get_if<unsigned long>();
        const auto maybeTagOffset   = currentTag.at(gr::tag::TRIGGER_OFFSET.shortKey()).get_if<float>();
        assert(maybeTriggerTime && maybeTagOffset);
        if (!maybeTriggerTime || !maybeTagOffset) {
            return std::nullopt;
        }
        const auto currentTagWRTime = std::chrono::nanoseconds(*maybeTriggerTime);
        const auto currentTagOffset = std::chrono::nanoseconds(static_cast<unsigned long>(*maybeTagOffset));
        auto [lastIdx, lastTime]    = *_lastMatchedTag;
        auto deltaTime              = currentTagWRTime + currentTagOffset - std::chrono::nanoseconds(lastTime);
        auto delta                  = static_cast<float>(deltaTime.count()) / Ts;
        auto deltaIdx               = static_cast<long>(std::floor(delta));
        auto deltaOffset            = (delta - static_cast<float>(deltaIdx)) / sampleRate;
        auto idx                    = lastIdx + deltaIdx;
        if (idx < 0) { // tag was before the current chunk of data
            return std::nullopt;
        };
        gr::property_map matchedTag                       = currentTag;
        matchedTag.at(gr::tag::TRIGGER_OFFSET.shortKey()) = deltaOffset;
        return gr::Tag{static_cast<std::size_t>(idx), matchedTag};
    }

    gr::Tag getOffsetAdjustedTag(auto currentFlankIndex, auto currentTagOffset, const gr::property_map& tagMap) {
        gr::property_map matchedTagMap = tagMap;
        const float      deltaT        = static_cast<float>(currentTagOffset.count()) * (sampleRate / 1e9f);
        auto             offsetIdx     = static_cast<long>(deltaT);
        float            offset        = deltaT - static_cast<float>(offsetIdx);
        if (offset < 0.0f) {
            --offsetIdx;
            offset += 1.0f;
        }
        matchedTagMap.at(gr::tag::TRIGGER_OFFSET.shortKey()) = offset;
        return {static_cast<std::size_t>(static_cast<long>(currentFlankIndex) + offsetIdx), matchedTagMap};
    }

    MatcherResult match(const std::span<const gr::property_map> tags, const std::span<const std::size_t>& triggerSampleIndices, const std::size_t nSamples, const std::chrono::nanoseconds localAcqTime) {
        MatcherResult     result;
        std::size_t       triggerIndex    = 0;
        std::size_t       unmatchedEvents = 0; // number of events that have to be adjusted based on the next trigger that is found
        float             Ts              = 1e9f / sampleRate;
        const auto        maxDelaySamples = static_cast<std::size_t>(static_cast<float>(std::chrono::nanoseconds(timeout).count()) * 1e-9f * sampleRate);
        const std::size_t safeSamples     = nSamples > maxDelaySamples ? nSamples - maxDelaySamples : 0;
        result.processedSamples           = safeSamples;                                                             // consume at least all samples up to the deadline
        while (result.processedTags + unmatchedEvents < tags.size() || triggerIndex < triggerSampleIndices.size()) { // keep processing as long as there is either unprocessed pulses or hw edges
            if (result.processedTags + unmatchedEvents >= tags.size()) {                                             // all event tags processed, checking for potential hw edges
                const auto currentFlankIndex = triggerSampleIndices[triggerIndex];
                const auto currentFlankTime  = localAcqTime + std::chrono::nanoseconds(static_cast<unsigned long>(Ts * static_cast<float>(currentFlankIndex)));
                if (currentFlankIndex < safeSamples) {
                    // unmatched hw edge found and deadline for tag to arrive has passed -> publish UNKNOWN_EVENT // evtX
                    if (!result.tags.empty() && result.tags.back().index > currentFlankIndex) {
                        // this should normally not happen, but there are some cases where a hardware event is published based on realigning it instead of the hardware edge and so the hardware edge is not consumed
                        result.messages.emplace_back(std::format("Cannot publish UNKNOWN_EVENT at {}, there have already been tags published before at {}", currentFlankIndex, result.tags.back().index));
                    } else {
                        result.tags.push_back(createUnknownEventTag(currentFlankIndex, currentFlankTime));
                    }
                    triggerIndex++;
                    continue;
                } else {
                    break; // hw-edge is too recent -> stop processing and check in the next invocation if we have a matching event tag
                }
            }
            const std::size_t       tagIndex   = result.processedTags + unmatchedEvents;
            const gr::property_map& currentTag = tags[tagIndex];
            if (!checkValidTimingTag(currentTag)) {
                result.messages.emplace_back(std::format("Invalid timing tag at index {}: {}", tagIndex, currentTag));
                result.processedTags++;
                continue;
            }
            auto* maybeMetaMap = currentTag.at(gr::tag::TRIGGER_META_INFO.shortKey()).get_if<gr::property_map>();
            if (!maybeMetaMap) {
                result.messages.emplace_back(std::format("Invalid type for TRIGGER_META_INFO value, expected property map, at index {}: {}", tagIndex, currentTag));
                continue;
            }
            auto& metaMap            = *maybeMetaMap;
            auto* maybeTagLocalTime  = metaMap.at("LOCAL-TIME").get_if<unsigned long>();
            auto* maybeTriggerTime   = currentTag.at(gr::tag::TRIGGER_TIME.shortKey()).get_if<unsigned long>();
            auto* maybeTriggerOffset = currentTag.at(gr::tag::TRIGGER_OFFSET.shortKey()).get_if<float>();
            if (!maybeTagLocalTime || !maybeTriggerTime || !maybeTriggerOffset) {
                result.messages.emplace_back(std::format("Invalid type for LOCAL-TIME/TRIGGER_TIME/TRIGGER_OFFSET, expected {}, at index {}: {}", gr::meta::type_name<unsigned long>(), tagIndex, currentTag));
                continue;
            }
            const auto currentTagLocalTime = std::chrono::nanoseconds(*maybeTagLocalTime);
            const auto currentTagWRTime    = std::chrono::nanoseconds(*maybeTriggerTime);
            const auto currentTagOffset    = std::chrono::nanoseconds(static_cast<unsigned long>(*maybeTriggerOffset));
            if (triggerIndex >= triggerSampleIndices.size()) {                                                                                                                                      // there are remaining events, but no more hw edges to match
                if (!metaMap.at("HW-TRIGGER").holds<bool>() || (currentTagLocalTime + timeout) < (localAcqTime + std::chrono::nanoseconds(static_cast<long>(static_cast<float>(nSamples) * Ts)))) { // we are sure the hw edge cannot still arrive
                    if (_lastMatchedTag) {                                                                                                                                                          // publish tags based on last matched trigger
                        std::optional<gr::Tag> realignedTag = alignTagRelativeToLastMatched(currentTag);
                        if (realignedTag && realignedTag->index >= result.processedSamples) {
                            break; // tag will be moved outside the current data chunk and has to be handled in the next iteration
                        }
                        if (realignedTag) {
                            result.processedSamples = std::max(result.processedSamples, realignedTag->index);
                            result.tags.push_back(std::move(*realignedTag));
                        } else {
                            result.messages.emplace_back(std::format("Failed to realign tag relative to last matched trigger: {}", currentTag));
                        }
                        result.processedTags++;
                    } else { // there is no previous tag, look at the next tag, but look at this one again before publishing the next sample
                        unmatchedEvents++;
                    }
                    continue;
                } else { // tag will not be consumed and handled in the next iteration
                    break;
                }
            }
            // below we still have tags and hw edges to match
            const auto currentFlankIndex = triggerSampleIndices[triggerIndex];
            const auto currentFlankTime  = localAcqTime + std::chrono::nanoseconds(static_cast<unsigned long>(1e9f / sampleRate * static_cast<float>(currentFlankIndex)));

            if ((currentTagLocalTime + timeout) < localAcqTime) { // outdated tag -> drop // evt0 in diagram
                result.processedTags++;
                result.messages.emplace_back(std::format("dropping outdated tag1: {}", currentTag));
                continue;
            }

            auto* maybeHWTrigger = metaMap.at("HW-TRIGGER").get_if<bool>();
            assert(maybeHWTrigger && "HW-TRIGGER should be bool");
            if (!maybeHWTrigger) {
                continue;
            }
            if (!(*maybeHWTrigger) || (currentTagLocalTime + timeout) < currentFlankTime) {
                // event either explicitly has no hardware trigger or the next hw edge is too far away
                if (_lastMatchedTag) { // publish tags based on the last-matched trigger // evtB or evt5 in the diagram
                    if (std::optional<gr::Tag> realignedTag = alignTagRelativeToLastMatched(currentTag)) {
                        result.processedSamples = std::max(result.processedSamples, realignedTag->index);
                        result.tags.push_back(std::move(*realignedTag));
                    } else {
                        result.messages.emplace_back(std::format("Failed to realign tag relative to last matched trigger: {}", currentTag));
                    }
                    result.processedTags++;
                } else { // there is currently no previous matched tag, keep tags to attach after the next trigger was detected // evtA in diagram
                    unmatchedEvents++;
                }
                continue;
            }

            if ((currentFlankTime + timeout) < currentTagLocalTime) { // hw edge without corresponding event
                result.tags.push_back(createUnknownEventTag(currentFlankIndex, currentFlankTime));
                result.processedSamples = std::max(result.processedSamples, currentFlankIndex);
                triggerIndex++; // skip outdated timing message(s)
                continue;
            }

            if (std::optional<gr::Tag> realignedDiagTag = alignTagRelativeToLastMatched(currentTag)) {
                constexpr std::size_t indexTolerance = 3;
                if (std::max(realignedDiagTag->index, currentFlankIndex) - std::min(realignedDiagTag->index, currentFlankIndex) > indexTolerance) {
                    result.messages.emplace_back(std::format("Possible wrong matching, lastMatchedTag can be wrongly assigned. Difference between currentTagIndex:{} and currentFlankIndex:{} is more than tolerance ({})", //
                        realignedDiagTag->index, currentFlankIndex, indexTolerance));
                }
            }

            // regular case, next hw edge belongs to the next tag
            _lastMatchedTag = {currentFlankIndex, std::chrono::nanoseconds(currentTagWRTime).count()};
            while (unmatchedEvents > 0) { // align all previously unaligned tags relative to this one
                if (std::optional<gr::Tag> realignedTag = alignTagRelativeToLastMatched(tags[tagIndex - unmatchedEvents])) {
                    result.tags.push_back(std::move(*realignedTag));
                } else {
                    result.messages.emplace_back(std::format("Failed to realign tag relative to last matched trigger: {}", currentTag));
                }
                unmatchedEvents--;
                result.processedTags++;
            }
            result.tags.emplace_back(getOffsetAdjustedTag(currentFlankIndex, currentTagOffset, currentTag));
            result.processedSamples = std::max(result.processedSamples, currentFlankIndex);
            result.processedTags++;
            triggerIndex++;
        } // (result.processedTags < tags.size() || triggerIndex < triggerSampleIndices.size())

        if (_lastMatchedTag) { // re-adjust the last matched tag index relative to the start of the next sample chunk
            _lastMatchedTag->first -= static_cast<long>(result.processedSamples);
        }
        while (unmatchedEvents > 0) { // drop outdated unmatched tags
            auto& unconsumedTag = tags[result.processedTags];
            auto* maybeMetaMap  = unconsumedTag.at(gr::tag::TRIGGER_META_INFO.shortKey()).get_if<gr::property_map>();
            if (!maybeMetaMap) {
                result.messages.emplace_back(std::format("Invalid type for trigger tag, expected property map"));
                continue;
            }
            auto& metaMap             = *maybeMetaMap;
            auto* maybeLocalTimeValue = metaMap.at("LOCAL-TIME").get_if<unsigned long>();
            if (!maybeLocalTimeValue) {
                result.messages.emplace_back(std::format("Invalid type for local time, expected unsigned long"));
                continue;
            }
            const auto unconsumedTagLocalTime = std::chrono::nanoseconds(*maybeLocalTimeValue);
            const auto lastSampleTime         = localAcqTime + std::chrono::nanoseconds(static_cast<uint64_t>(static_cast<float>(nSamples) * Ts));
            if (unconsumedTagLocalTime < lastSampleTime - 2 * timeout) { // needs twice the timeout, since it is already contained once in the unpublished samples
                result.processedTags++;
                result.messages.emplace_back(std::format("dropping outdated(lastSampletTime={}, timeout={}) tag3: {}", lastSampleTime, timeout, unconsumedTag));
            } else {
                break;
            }
            unmatchedEvents--;
        }

        return result;
    }

    void reset() { _lastMatchedTag = std::nullopt; }
};

} // namespace fair::picoscope::timingmatcher

#endif // GR_DIGITIZERS_TIMINGMATCHER_HPP
