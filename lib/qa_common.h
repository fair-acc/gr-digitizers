/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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
    assert_vector(float expected, float_vec_iter actual_it, float_vec_iter actual_end)
    {
      for (; actual_it != actual_end; actual_it++) {
        CPPUNIT_ASSERT_EQUAL(expected, *actual_it);
      }
    }

    template <typename IterE, typename IterA> void
    assert_vector(IterE expected_it, IterE expected_end, IterA actual_it)
    {
      for (; expected_it != expected_end; expected_it++, actual_it++) {
        CPPUNIT_ASSERT_EQUAL(*expected_it, *actual_it);
      }
    }

    #define ASSERT_VECTOR_EQUAL(expected_begin, expected_end, actual_begin)  \
      ( assert_vector( (expected_begin),                       \
                       (expected_end),                         \
                       (actual_begin)) )
  }
}

#endif



