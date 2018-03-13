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
      std::string fittable = "true, true";
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

    void
    qa_chi_square_fit::test_chi_square_up_down_ramping()
    {
      //actual function variables
      int signal_len = 30;
      float actual_gradient = 45;
      float actual_offset = 20;
      std::vector<float> signal;
      //up ramp
      for(int i = 1; i <= signal_len; i++) {
        signal.push_back(i * actual_gradient + actual_offset);
      }
      //down ramp
      for(int i = 1; i <= signal_len; i++) {
        signal.push_back(-i * actual_gradient*0.5 - actual_offset);
      }

      //blocks
      auto top = gr::make_top_block("basic_connection");
      auto vec_src = blocks::vector_source_f::make(signal, true, signal_len*2);
      auto vec_manip = blocks::vector_to_stream::make(signal_len* sizeof(float), 2);
      auto vec_snk0 = blocks::vector_sink_f::make(2);
      auto vec_snk1 = blocks::vector_sink_f::make(2);
      auto null_snk0 = blocks::null_sink::make(sizeof(float));
      auto null_snk1 = blocks::null_sink::make(sizeof(char));
      std::string names = "gradient, offset";
      std::vector<double> inits({actual_gradient+3, 0.0});
      std::vector<double> errs({0.0, 16.0});
      std::vector<double> search_limit_up({ 1.2 * actual_gradient,  1.2 * actual_offset});
      std::vector<double> search_limit_dn({-1.2 * actual_gradient, -1.2 * actual_offset});
      std::string fittable = "true, true";
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
      top->connect(vec_src, 0, vec_manip, 0);
      top->connect(vec_manip, 0, fitter, 0);
      top->connect(fitter,0, vec_snk0, 0);
      top->connect(fitter,1, vec_snk1, 0);
      top->connect(fitter,2, null_snk0, 0);
      top->connect(fitter,3, null_snk1, 0);

      //run this circuit
      top->start();
      while(vec_snk0->data().size() == 0);//TODO:microsleep
      top->stop();
      auto values = vec_snk0->data();
      auto errors = vec_snk1->data();

      //check correct results
      CPPUNIT_ASSERT(((int)values.size()) != 0);
      size_t minCorrespondent = std::min(values.size(), errors.size());
      CPPUNIT_ASSERT_EQUAL(size_t(0), minCorrespondent%2);
      for(size_t i =0; i < minCorrespondent/2-1; i+= 2) {
        //up ramp
        CPPUNIT_ASSERT_DOUBLES_EQUAL(actual_gradient, values.at(2*i), 0.002);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(actual_offset, values.at(2*i+1), 0.002);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, errors.at(2*i), 0.002);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, errors.at(2*i+1), 0.002);
        //down ramp
        CPPUNIT_ASSERT_DOUBLES_EQUAL(-actual_gradient*0.5, values.at(2*i+2), 0.002);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(-actual_offset, values.at(2*i+3), 0.002);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, errors.at(2*i+2), 0.002);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, errors.at(2*i+3), 0.002);
      }

    }

  } /* namespace digitizers */
} /* namespace gr */

