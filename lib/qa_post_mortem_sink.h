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


#ifndef _QA_POST_MORTEM_SINK_H_
#define _QA_POST_MORTEM_SINK_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_post_mortem_sink : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_post_mortem_sink);
      CPPUNIT_TEST(basics);
      CPPUNIT_TEST(buffer_not_full);
      CPPUNIT_TEST(buffer_full);
      CPPUNIT_TEST(buffer_overflow);
      CPPUNIT_TEST(acq_info);
      CPPUNIT_TEST_SUITE_END();

    private:
      void basics();
      void buffer_not_full();
      void buffer_full();
      void buffer_overflow();
      void acq_info();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_POST_MORTEM_SINK_H_ */

