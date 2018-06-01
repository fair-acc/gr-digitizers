/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#include <gnuradio/top_block.h>

#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_chi_square_fit.h"
#include <digitizers/chi_square_fit.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/vector_sink_f.h>
#include <gnuradio/blocks/vector_to_stream.h>
#include <gnuradio/blocks/null_sink.h>


namespace gr {
  namespace digitizers {

    void
    qa_chi_square_fit::test_chi_square_simple_fitting()
    {
      //actual function variables
      int signal_len = 30;
      float actual_gradient = 45;
      float actual_offset = 20;
      std::vector<float> signal;
      for(int i = 1; i <= signal_len; i++) {
        signal.push_back(i * actual_gradient + actual_offset);
      }

      //blocks
      auto top = gr::make_top_block("basic_connection");
      auto vec_src = blocks::vector_source_f::make(signal, false, signal_len);
      auto vec_snk0 = blocks::vector_sink_f::make(2);
      auto vec_snk1 = blocks::vector_sink_f::make(2);
      auto null_snk0 = blocks::null_sink::make(sizeof(float));
      auto null_snk1 = blocks::null_sink::make(sizeof(char));

      std::string names = "gradient, offset";
      std::vector<double> inits({actual_gradient+3, 0.0});
      std::vector<double> errs({0.0, 16.0});
      std::vector<double> search_limit_up({ 1.2 * actual_gradient,  1.2 * actual_offset});
      std::vector<double> search_limit_dn({-1.2 * actual_gradient, -1.2 * actual_offset});
      std::vector<int> fittable = {1, 1};
      std::string function = "x*[0] + 1.0*[1] ";
      auto fitter = digitizers::chi_square_fit::make(
          signal_len,
          function,
          signal_len,
          1.0,
          2,
          names,
          inits,
          errs,
          fittable,
          search_limit_up,
          search_limit_dn,
          0.001);

      //connections
      top->connect(vec_src, 0, fitter, 0);
      top->connect(fitter,0, vec_snk0, 0);
      top->connect(fitter,1, vec_snk1, 0);
      top->connect(fitter,2, null_snk0, 0);
      top->connect(fitter,3, null_snk1, 0);


      //run this circuit
      top->run();
      auto values = vec_snk0->data();
      auto errors = vec_snk1->data();

      //check correct results
      CPPUNIT_ASSERT(((int)values.size()) != 0);
      size_t min_correspondent = std::min(values.size(), errors.size());
      CPPUNIT_ASSERT_EQUAL(size_t(0), min_correspondent%2);
      for(size_t i =0; i < min_correspondent/2; i++) {
        CPPUNIT_ASSERT_DOUBLES_EQUAL(actual_gradient, values.at(2*i), 0.002);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(actual_offset, values.at(2*i+1), 0.002);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, errors.at(2*i), 0.002);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, errors.at(2*i+1), 0.002);
      }

    }

  } /* namespace digitizers */
} /* namespace gr */

