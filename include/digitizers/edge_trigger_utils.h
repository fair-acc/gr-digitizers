/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


#ifndef INCLUDED_DIGITIZERS_EDGE_TRIGGER_UTILS_H
#define INCLUDED_DIGITIZERS_EDGE_TRIGGER_UTILS_H

#include <digitizers/api.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <string>

namespace gr {
  namespace digitizers {

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
     * \param edge_detec struct holding all the info for encoding
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
     * See also ::encode_edge_detect.
     *
     * \param payload string holding a payload
     * \param edge_detect a reference to the ::edge_detect_t structure
     * \return true if successfuly decoded else it returns false
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

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_EDGE_TRIGGER_UTILS_H */

