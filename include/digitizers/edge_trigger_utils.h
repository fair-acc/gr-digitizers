/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#ifndef INCLUDED_DIGITIZERS_EDGE_TRIGGER_UTILS_H
#define INCLUDED_DIGITIZERS_EDGE_TRIGGER_UTILS_H

#include <gnuradio/tags.h>
#include <digitizers/api.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <string>

namespace gr {
  namespace digitizers {

    char const * const edge_detect_tag_name = "edge_detect";
    /*!
     * \brief Convenience structure for encoding/decoding the edge detect datagram.
     *
     * See ::encode_edge_detect and ::decode_edge_detect method description.
     *
     * \ingroup digitizers
     *
     */
    struct DIGITIZERS_API edge_detect_t
    {
      int64_t timing_event_timestamp;         // UTC nanoseconds
      int64_t retrigger_event_timestamp;      // UTC nanoseconds
      int64_t delay_since_last_timing_event;  // nanoseconds
      int64_t samples_since_last_timing_event;
      float value;           // trigger value (normally in Volts)

      uint64_t offset;       // tag offset

      bool is_raising_edge;
    };


    /*!
     * \brief Helper function for encoding gr::digitizers::edge_detect_t structure into xml.
     *
     * The following format is assumed:
     *
     * \code
     * <edgeDetect
     *        edge="rising"
     *        val="0.6545"
     *        timingEventTimeStamp="<timestamp>"
     *        retriggerEventTimeStamp="<timestamp>"
     *        delaySinceLastTimingEvent="<delay in nanoseconds>"
     *        samplesSinceLastTimingEvent="<samples>" />
     * \endcode
     *
     * Note timestamp is assumed to be an integer, namely int64_t, and it represents number
     * of nanoseconds since UNIX epoch (UTC nanoseconds).
     *
     * \param edge_detect struct holding all the info for encoding
     * \return string holding an xml node.
     */
    inline std::string
    encode_edge_detect(const edge_detect_t &edge_detect)
    {
      boost::property_tree::ptree element;
      element.put<std::string>("edgeDetect.<xmlattr>.edge", edge_detect.is_raising_edge ? "rising" : "falling");
      element.put<float>("edgeDetect.<xmlattr>.val", edge_detect.value);
      element.put<int64_t>("edgeDetect.<xmlattr>.timingEventTimeStamp", edge_detect.timing_event_timestamp);
      element.put<int64_t>("edgeDetect.<xmlattr>.retriggerEventTimeStamp", edge_detect.retrigger_event_timestamp);
      element.put<int64_t>("edgeDetect.<xmlattr>.delaySinceLastTimingEvent", edge_detect.delay_since_last_timing_event);
      element.put<int64_t>("edgeDetect.<xmlattr>.samplesSinceLastTimingEvent", edge_detect.samples_since_last_timing_event);

      std::stringstream iostr;
      boost::property_tree::xml_writer_settings<char> settings;
      boost::property_tree::xml_parser::write_xml_element(iostr,
              boost::property_tree::ptree::key_type(), element, -1, settings);
      return iostr.str();
    };

    /*! \brief Helper function for decoding edge trigger.
     *
     * See also gr::digitizers::encode_edge_detect.
     *
     * \param payload string holding a payload
     * \param edge_detect a reference to the gr::digitizers::edge_detect_t structure
     * \return true if successfully decoded else it returns false
     */
    inline bool
    decode_edge_detect(const std::string &payload, edge_detect_t &edge_detect)
    {
      try {
        std::stringstream iostr(payload);
        boost::property_tree::ptree tree;

        // Parse the XML into the property tree.
        boost::property_tree::read_xml(iostr, tree);

        auto edge = tree.get<std::string>("edgeDetect.<xmlattr>.edge");
        if (edge == "rising") {
          edge_detect.is_raising_edge = true;
        }
        else if (edge == "falling") {
          edge_detect.is_raising_edge = false;
        }
        else {
          return false;
        }
        edge_detect.value = tree.get<float>("edgeDetect.<xmlattr>.val");
        edge_detect.timing_event_timestamp = tree.get<int64_t>("edgeDetect.<xmlattr>.timingEventTimeStamp");
        edge_detect.retrigger_event_timestamp = tree.get<int64_t>("edgeDetect.<xmlattr>.retriggerEventTimeStamp");
        edge_detect.delay_since_last_timing_event = tree.get<int64_t>("edgeDetect.<xmlattr>.delaySinceLastTimingEvent");
        edge_detect.samples_since_last_timing_event = tree.get<int64_t>("edgeDetect.<xmlattr>.samplesSinceLastTimingEvent");
      } catch (...) {
        return false;
      }

      return true;
    };

    inline tag_t
    make_edge_detect_tag(edge_detect_t &edge_detect)
    {
      tag_t edge_tag;
      edge_tag.key = pmt::mp(edge_detect_tag_name);
      edge_tag.offset = edge_detect.offset;
      edge_tag.value = pmt::make_tuple(
          pmt::from_bool(edge_detect.is_raising_edge),
          pmt::from_uint64(static_cast<uint64_t>(edge_detect.timing_event_timestamp)),
          pmt::from_uint64(static_cast<uint64_t>(edge_detect.retrigger_event_timestamp)),
          pmt::from_uint64(static_cast<uint64_t>(edge_detect.delay_since_last_timing_event)),
          pmt::from_uint64(static_cast<uint64_t>(edge_detect.samples_since_last_timing_event)));

      return edge_tag;
    }

    inline edge_detect_t
    decode_edge_detect_tag(const tag_t &tag)
    {
      assert(pmt::symbol_to_string(tag.key) == edge_detect_tag_name);

      if (!pmt::is_tuple(tag.value) || pmt::length(tag.value) != 5) {
        throw std::runtime_error("invalid edge detect tag format");
      }

      edge_detect_t edge;
      edge.offset = tag.offset;

      auto tag_tuple = pmt::to_tuple(tag.value);
      edge.is_raising_edge = pmt::to_bool(tuple_ref(tag_tuple, 0));
      edge.timing_event_timestamp = static_cast<int64_t>(pmt::to_uint64(tuple_ref(tag_tuple, 1)));
      edge.retrigger_event_timestamp = static_cast<int64_t>(pmt::to_uint64(tuple_ref(tag_tuple, 2)));
      edge.delay_since_last_timing_event = static_cast<int64_t>(pmt::to_uint64(tuple_ref(tag_tuple, 3)));
      edge.samples_since_last_timing_event = static_cast<int64_t>(pmt::to_uint64(tuple_ref(tag_tuple, 4)));

      return edge;
    }

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_EDGE_TRIGGER_UTILS_H */

