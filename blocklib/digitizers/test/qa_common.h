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

    #define ASSERT_VECTOR_OF(vector, number)  \
      for (auto item : vector) { \
    CPPUNIT_ASSERT_EQUAL(item, number);}

    #define ASSERT_VECTOR_EQUAL(expected_begin, expected_end, actual_begin)  \
            { \
                int index = 0; \
                auto it_expected = expected_begin; \
                auto it_actual = actual_begin; \
                for (; it_expected != expected_end; it_expected++, it_actual++) \
                { \
                   char str[80]; \
                   sprintf(str, "at relative index: %d", index); \
                   CPPUNIT_ASSERT_EQUAL_MESSAGE(str,*it_expected, *it_actual); \
                   index++; \
                } \
            }

  }
}

#endif



