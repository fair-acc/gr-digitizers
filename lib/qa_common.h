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


#ifndef INCLUDED_DIGITIZERS_QA_COMMON_H
#define INCLUDED_DIGITIZERS_QA_COMMON_H

#include <cppunit/TestAssert.h>
#include <vector>

namespace gr {
  namespace digitizers {


    inline std::vector<float>
    make_test_data(size_t size, float gain=1)
    {
      std::vector<float> data;
      for (size_t i=0; i<size; i++) {
        data.push_back(static_cast<float>(i) * gain);
      }
      return data;
    }

    typedef std::vector<float>::iterator float_vec_iter;

    inline void
    assert_float_vector(float_vec_iter expected_it, float_vec_iter expected_end, float_vec_iter actual_it)
    {
      for (; expected_it != expected_end; expected_it++, actual_it++) {
        CPPUNIT_ASSERT_EQUAL(*expected_it, *actual_it);
      }
    }

    inline void
    assert_float_vector(float expected, float_vec_iter actual_it, float_vec_iter actual_end)
    {
      for (; actual_it != actual_end; actual_it++) {
        CPPUNIT_ASSERT_EQUAL(expected, *actual_it);
      }
    }

    #define ASSERT_VECTOR_EQUAL(expected, actual_it, actual_end)  \
      ( assert_float_vector( (expected),                          \
                             (actual_it),                         \
                             (actual_end)) )
  }
}

#endif



