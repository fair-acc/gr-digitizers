/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
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


#ifndef _QA_TIME_DOMAIN_SINK_H_
#define _QA_TIME_DOMAIN_SINK_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_time_domain_sink : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_time_domain_sink);
      CPPUNIT_TEST(stream_basics);
      CPPUNIT_TEST(stream_values_no_tags);
      CPPUNIT_TEST(stream_values);
      CPPUNIT_TEST(stream_overflow);
      CPPUNIT_TEST(stream_callback);
      CPPUNIT_TEST(stream_acq_info_tag);
      CPPUNIT_TEST(stream_partial_readout);
      CPPUNIT_TEST(fast_basics);
      CPPUNIT_TEST(fast_values_no_tags);
      CPPUNIT_TEST(fast_values);
      CPPUNIT_TEST(fast_callback);
      CPPUNIT_TEST_SUITE_END();

    private:
      void stream_basics();
      void stream_values_no_tags();
      void stream_values();
      void stream_overflow();
      void stream_callback();
      void stream_acq_info_tag();
      void stream_partial_readout();
      void fast_basics();
      void fast_values_no_tags();
      void fast_values();
      void fast_callback();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_TIME_DOMAIN_SINK_H_ */

