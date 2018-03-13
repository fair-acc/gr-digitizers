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


#ifndef _QA_BLOCK_SCALING_OFFSET_H_
#define _QA_BLOCK_SCALING_OFFSET_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace digitizers {

    class qa_block_scaling_offset : public CppUnit::TestCase
    {
    public:
      CPPUNIT_TEST_SUITE(qa_block_scaling_offset);
      CPPUNIT_TEST(scale_and_offset);
      CPPUNIT_TEST_SUITE_END();

    private:
      void scale_and_offset();
    };

  } /* namespace digitizers */
} /* namespace gr */

#endif /* _QA_BLOCK_SCALING_OFFSET_H_ */

