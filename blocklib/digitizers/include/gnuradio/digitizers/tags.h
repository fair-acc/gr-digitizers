#ifndef INCLUDED_DIGITIZERS_TAGS_H
#define INCLUDED_DIGITIZERS_TAGS_H

#include <gnuradio/digitizers/api.h>
#include <gnuradio/tag.h>

#include <pmtv/pmt.hpp>

#include <fmt/format.h>

#include <cassert>
#include <chrono>

namespace gr::digitizers {

// ################################################################################################################
// ################################################################################################################

/*!
 * \brief A convenience structure holding information about the measurement.
 * \ingroup digitizers
 *
 * General use:
 *
 * Digitizers are expected to attach the 'acq_info' tag to all the enabled channels
 * including digital ports. In streaming mode this should be done whenever a new chunk of
 * data is obtained from the device.
 *
 * If trigger detection is enabled in streaming mode, then the acq_info tag should be
 * attached to the output streams whenever a trigger is detected. In this case the
 * triggered_data field should be set to True allowing other post-processing modules (i.e.
 * B.2 Demux) to work with triggered data only.
 *
 *
 * The acq_info tag contains two delays, namely the user_delay and the actual_delay. The
 * following ascii figure depicts relation between different terms:
 *
 * user_delay                 |--->          (3 us)
 * realignment_delay          |--------->    (9 us)
 * actual_delay               |------------> (3 + 9 = 12 us)
 *
 * Those delays are used for two purposes: (TODO: This doc. is outdated, fix it)
 *
 * 1) Time synchronization:
 *    User delay is added to the actual timestamp of a given sample the tag is attached
 * to. Field timestamp therefore contains timestamp calculated like this:
 *
 *    timestamp = <actual acquisition timestamp> + user_delay
 *
 * 2) Extraction of triggered data
 *    Actual delay, that is user delay and edge-trigger based delay synchronization is
 * accounted for when extracting triggered data. See B.2 block description for more
 * details (extractor.h).
 */

char const* const acq_info_tag_name = "acq_info";
struct DIGITIZERS_API acq_info_t {
    int64_t timestamp;   // timestamp (UTC nanoseconds), used as fallback if no trigger is
                         // available
    double timebase;     // distance between samples in seconds
    double user_delay;   // see description above
    double actual_delay; // see description above
    uint32_t status;     // acquisition status
};

inline gr::tag_t make_acq_info_tag(const acq_info_t& acq_info, uint64_t offset)
{
    return { offset,
             { { acq_info_tag_name,
                 std::vector<pmtv::pmt>{ static_cast<uint64_t>(acq_info.timestamp),
                                         static_cast<double>(acq_info.timebase),
                                         static_cast<double>(acq_info.user_delay),
                                         static_cast<double>(acq_info.actual_delay),
                                         static_cast<long>(acq_info.status) } } } };
}

inline acq_info_t decode_acq_info_tag(const gr::tag_t& tag)
{
    const auto tag_value = tag.get(acq_info_tag_name);

    if (!tag_value) {
        throw std::runtime_error(
            fmt::format("Exception in {}:{}: tag does not contain '{}'",
                        __FILE__,
                        __LINE__,
                        acq_info_tag_name));
    }

    const auto tag_vector = pmtv::get_vector<pmtv::pmt>(tag_value->get());

    if (tag_vector.size() != 5) {
        throw std::runtime_error(fmt::format(
            "Exception in {}:{}: invalid acq_info tag format", __FILE__, __LINE__));
    }

    return { .timestamp = static_cast<int64_t>(std::get<uint64_t>(tag_vector[0])),
             .timebase = std::get<double>(tag_vector[1]),
             .user_delay = std::get<double>(tag_vector[2]),
             .actual_delay = std::get<double>(tag_vector[3]),
             .status = static_cast<uint32_t>(std::get<long>(tag_vector[4])) };
}

// ################################################################################################################
// ################################################################################################################

struct DIGITIZERS_API trigger_t {
    std::string name;
    std::chrono::nanoseconds timestamp;
    std::chrono::nanoseconds offset;
};

inline gr::tag_t make_trigger_tag(uint64_t tag_offset,
                                  std::string name,
                                  std::chrono::nanoseconds timestamp_ns,
                                  std::chrono::nanoseconds trigger_offset)
{
    return { tag_offset,
             { { tag::TRIGGER_NAME, std::move(name) },
               { tag::TRIGGER_TIME, timestamp_ns.count() },
               { tag::TRIGGER_OFFSET,
                 std::chrono::duration<double>(trigger_offset).count() } } };
}

inline gr::tag_t make_trigger_tag(const trigger_t& trigger_tag_data, uint64_t offset)
{
    return make_trigger_tag(offset,
                            trigger_tag_data.name,
                            trigger_tag_data.timestamp,
                            trigger_tag_data.offset);
}

inline gr::tag_t make_trigger_tag(uint64_t offset)
{
    using namespace std::chrono_literals;
    return make_trigger_tag(offset, {}, 0ns, 0ns);
}

namespace detail {
template <typename T>
inline constexpr std::chrono::nanoseconds convert_to_ns(T ns)
{
    using namespace std::chrono;
    return round<nanoseconds>(duration<T, std::nano>{ ns });
}
} // namespace detail

inline trigger_t decode_trigger_tag(const gr::tag_t& tag)
{
    const auto name = tag.get(tag::TRIGGER_NAME.key());
    if (!name) {
        throw std::runtime_error(
            fmt::format("Exception in {}:{}: tag does not contain '{}'",
                        __FILE__,
                        __LINE__,
                        tag::TRIGGER_NAME.key()));
    }

    const auto timestamp = tag.get(tag::TRIGGER_TIME.key());
    if (!timestamp) {
        throw std::runtime_error(
            fmt::format("Exception in {}:{}: tag does not contain '{}'",
                        __FILE__,
                        __LINE__,
                        tag::TRIGGER_TIME.key()));
    }

    const auto offset = tag.get(tag::TRIGGER_OFFSET.key());
    if (!offset) {
        throw std::runtime_error(
            fmt::format("Exception in {}:{}: tag does not contain '{}'",
                        __FILE__,
                        __LINE__,
                        tag::TRIGGER_OFFSET.key()));
    }

    return { .name = pmtf::get_as<std::string>(name->get()),
             .timestamp = detail::convert_to_ns(std::get<int64_t>(timestamp->get())),
             .offset = detail::convert_to_ns(std::get<double>(offset->get())) };
}

// ################################################################################################################
// ################################################################################################################

char const* const timebase_info_tag_name = "timebase_info";

/*!
 * \brief Factory function for creating timebase_info tags.
 */
inline gr::tag_t make_timebase_info_tag(double timebase)
{
    return { 0, { { timebase_info_tag_name, timebase } } };
}

/*!
 * \brief Returns timebase stored within the timebase_info tag.
 */
inline double decode_timebase_info_tag(const gr::tag_t& tag)
{
    const auto tag_value = tag.get(timebase_info_tag_name);
    if (!tag_value) {
        throw std::runtime_error(
            fmt::format("Exception in {}:{}: tag does not contain '{}'",
                        __FILE__,
                        __LINE__,
                        timebase_info_tag_name));
    }

    return std::get<double>(tag_value->get());
}

// ################################################################################################################
// ################################################################################################################

/*!
 * \brief Name of the WR event tag.
 */
char const* const wr_event_tag_name = "wr_event";

/*!
 * \brief A convenience structure holding information about the WR timing event.
 * \ingroup digitizers

 */
struct DIGITIZERS_API wr_event_t {
    std::string event_id;
    int64_t wr_trigger_stamp;     // timestamp of the wr-event (TAI nanoseconds)
    int64_t wr_trigger_stamp_utc; // timestamp of the wr-event (UTC nanoseconds)
};

/*!
 * \brief Factory function for creating wr event tags.
 */
inline gr::tag_t make_wr_event_tag(const wr_event_t& event, uint64_t offset)
{
    const auto value =
        std::vector<pmtv::pmt>{ event.event_id,
                                static_cast<uint64_t>(event.wr_trigger_stamp),
                                static_cast<uint64_t>(event.wr_trigger_stamp_utc) };
    return { offset, { { wr_event_tag_name, value } } };
}

/*!
 * \brief Converts wr event tag into wr_event_t struct.
 */
inline wr_event_t decode_wr_event_tag(const gr::tag_t& tag)
{
    const auto tag_value = tag.get(wr_event_tag_name);

    if (!tag_value) {
        throw std::runtime_error(
            fmt::format("Exception in {}:{}: tag does not contain '{}'",
                        __FILE__,
                        __LINE__,
                        wr_event_tag_name));
    }

    const auto tag_vector = pmtv::get_vector<pmtv::pmt>(tag_value->get());

    if (tag_vector.size() != 3) {
        throw std::runtime_error(fmt::format(
            "Exception in {}:{}: invalid wr_event tag format", __FILE__, __LINE__));
    }

    return { .event_id = std::get<std::string>(tag_vector[0]),
             .wr_trigger_stamp = static_cast<int64_t>(std::get<uint64_t>(tag_vector[1])),
             .wr_trigger_stamp_utc =
                 static_cast<int64_t>(std::get<uint64_t>(tag_vector[2])) };
}

// ################################################################################################################
// ################################################################################################################

} // namespace gr::digitizers

#endif /* INCLUDED_DIGITIZERS_TAGS_H */
