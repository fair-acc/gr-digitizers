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


#ifndef _QA_FREQ_SINK_F_H_
#define _QA_FREQ_SINK_F_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_freq_sink_f : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_freq_sink_f);
      CPPUNIT_TEST(test_metadata);
      CPPUNIT_TEST(test_sink_no_tags);
      CPPUNIT_TEST(test_sink_tags);
      CPPUNIT_TEST(test_sink_callback);
      CPPUNIT_TEST(test_snapshot_mode);
      CPPUNIT_TEST_SUITE_END();

    private:
      void test_metadata();
      void test_sink_no_tags();
      void test_sink_tags();
      void test_sink_callback();
      void test_snapshot_mode();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_FREQ_SINK_F_H_ */

