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


#ifndef _QA_EXTRACTOR_H_
#define _QA_EXTRACTOR_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {
    class qa_extractor : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_extractor);
      CPPUNIT_TEST(test_no_trigger);
      CPPUNIT_TEST(test_single_trigger);
      CPPUNIT_TEST(test_single_trigger_no_pre);
      CPPUNIT_TEST(test_user_delay);
      CPPUNIT_TEST(test_realignment_delay);
      CPPUNIT_TEST(test_not_enough_trigger_samples);
      CPPUNIT_TEST_SUITE_END();
    private:
      void test_no_trigger();
      void test_single_trigger();
      void test_single_trigger_no_pre();
      void test_user_delay();
      void test_realignment_delay();
      void test_not_enough_trigger_samples();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_EXTRACTOR_H_ */

