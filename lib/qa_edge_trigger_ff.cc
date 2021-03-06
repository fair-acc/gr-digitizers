/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */


#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_edge_trigger_ff.h"
#include <digitizers/edge_trigger_ff.h>
#include <digitizers/edge_trigger_utils.h>

namespace gr {
  namespace digitizers {

    void
    qa_edge_trigger_ff::decode()
    {
      std::string payload = "<edgeDetect"
                " edge=\"rising\""
                " val=\"0.6545\""
                " timingEventTimeStamp=\"-1\""
                " retriggerEventTimeStamp=\"9876543210\""
                " delaySinceLastTimingEvent=\"123456\""
                " samplesSinceLastTimingEvent=\"654321\" />";

      edge_detect_t edge;
      auto decoded = decode_edge_detect(payload, edge);
      CPPUNIT_ASSERT_EQUAL(true, decoded);

      CPPUNIT_ASSERT_EQUAL(true, edge.is_raising_edge);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(0.6545, edge.value, 1E-6);
      CPPUNIT_ASSERT_EQUAL(int64_t{-1},         edge.timing_event_timestamp);
      CPPUNIT_ASSERT_EQUAL(int64_t{9876543210}, edge.retrigger_event_timestamp);
      CPPUNIT_ASSERT_EQUAL(int64_t{123456},     edge.delay_since_last_timing_event);
      CPPUNIT_ASSERT_EQUAL(int64_t{654321},     edge.samples_since_last_timing_event);
    }

    void
    qa_edge_trigger_ff::encode_decode()
    {
      edge_detect_t test_edge;
      test_edge.is_raising_edge = false;
      test_edge.value = -2.33;
      test_edge.retrigger_event_timestamp = 1234567890;
      test_edge.delay_since_last_timing_event = 1000;
      test_edge.samples_since_last_timing_event = 50;

      std::string payload;
      CPPUNIT_ASSERT_NO_THROW(
        payload = encode_edge_detect(test_edge);
      );

      edge_detect_t edge;
      auto decoded = decode_edge_detect(payload, edge);
      CPPUNIT_ASSERT_EQUAL(true, decoded);

      CPPUNIT_ASSERT_EQUAL(test_edge.is_raising_edge, edge.is_raising_edge);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(test_edge.value, edge.value, 1E-6);
      CPPUNIT_ASSERT_EQUAL(test_edge.timing_event_timestamp, edge.timing_event_timestamp);
      CPPUNIT_ASSERT_EQUAL(test_edge.retrigger_event_timestamp, edge.retrigger_event_timestamp);
      CPPUNIT_ASSERT_EQUAL(test_edge.delay_since_last_timing_event, edge.delay_since_last_timing_event);
      CPPUNIT_ASSERT_EQUAL(test_edge.samples_since_last_timing_event, edge.samples_since_last_timing_event);
    }

  } /* namespace digitizers */
} /* namespace gr */

