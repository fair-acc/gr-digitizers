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


#ifndef _QA_PICOSCOPE_3000A_H_
#define _QA_PICOSCOPE_3000A_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>
#include <digitizers/picoscope_3000a.h>

namespace gr {
  namespace digitizers {

    class qa_picoscope_3000a : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_picoscope_3000a);
      CPPUNIT_TEST(open_close);
      CPPUNIT_TEST(rapid_block_basics);
      CPPUNIT_TEST(rapid_block_channels);
      CPPUNIT_TEST(rapid_block_continuous);
      CPPUNIT_TEST(rapid_block_downsampling_basics);
      CPPUNIT_TEST(rapid_block_downsampling);
      CPPUNIT_TEST(rapid_block_tags);
      CPPUNIT_TEST_SUITE_END();

    private:
      void run_rapid_block_downsampling(downsampling_mode_t mode);

      picoscope_3000a::sptr createAndInitRapidBlock();
      picoscope_3000a::sptr createAndInitStream();
      void setUpDevice();
      void open_close();
      void rapid_block_basics();
      void rapid_block_channels();
      void rapid_block_continuous();
      void rapid_block_downsampling_basics();
      void rapid_block_downsampling();
      void rapid_block_tags();
      void rapid_block_trigger();

      void streaming_basics();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_PICOSCOPE_3000A_H_ */

