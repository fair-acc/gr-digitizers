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


#ifndef INCLUDED_DIGITIZERS_RANGE_H
#define INCLUDED_DIGITIZERS_RANGE_H

#include <iostream>
#include <limits>
#include <vector>
#include <boost/math/special_functions/round.hpp>
#include <digitizers/api.h>

namespace gr {
namespace digitizers{


     static const double default_step = 0.0000001;

    /*!
     * \brief Represents available ranges.
     * 
     * \ingroup digitizers
     * 
     * Based on: https://github.com/EttusResearch/uhd/blob/maint/host/lib/types/ranges.cpp
     */ 
    class DIGITIZERS_API range_t
    {
    public:

      range_t(double value = 0.0)
          : d_start(value), d_stop(value), d_step(0.0) {}

      range_t(double start, double stop, double step = default_step)
          : d_start(start), d_stop(stop), d_step(step) {
        if (stop < start) {
        	throw std::invalid_argument("stop should be larger or equal to start, start: "
        		+ std::to_string(start)+ ", stop: "  + std::to_string(stop));
        }
        if (step <= 0.0) {
        	throw std::invalid_argument("invalid step: " + std::to_string(step));
        }
      }
     
      double start() const { return d_start; };
    
      double stop() const { return d_stop; };
    
      double step() const { return d_step; };

    private:
      double d_start;
      double d_stop;
	  double d_step;
    };
    
    class DIGITIZERS_API meta_range_t : public std::vector<range_t>
    {
    public:
      meta_range_t() { }
    
      template <typename InputIterator>
      meta_range_t(InputIterator first, InputIterator last)
          : std::vector<range_t>(first, last) { /* NOP */ }
    
      meta_range_t(double start, double stop, double step = default_step)
          : std::vector<range_t> (1, range_t(start, stop, step)) { }
    
      double start() const
      {
        check_meta_range_monotonic();
        return back().start();
      }
    
      double stop() const {
        check_meta_range_monotonic();
        return front().stop();
      }
    
      double clip(double value) const {
        check_meta_range_monotonic();

        // less or equal to min
        if (value <= front().start()) {
          return front().start();
        }

        // Greater or equal to max
        if (value >= back().stop()) {
          return back().stop();
        }

        // find appropriate range, always clip up! Note reverse iterator is used in order to
        // simplify the implementation...
        auto it = std::find_if(begin(), end(), [value] (const range_t &r)
        {
          return value <= r.start();
        });

        // this should not happen.... ever.
        if (it == end()) {
          throw std::runtime_error("failed to find appropriate range for the value " + std::to_string(value));
        }
           
        // lest find setting
        if (it->start()  == it->stop()) {
           return it->start();
        }
        else if (it->step() != 0.0) {
          return boost::math::round((value - it->start()) / it->step()) * it->step() + it->start();
        }
        else {
          // lower-bound check done as part of find_if
          assert (value >= it->start());
          return value >= it->stop() ? it->stop() : value;
        }
      };

    private:

      void check_meta_range_monotonic() const
      {
        if (empty()) {
            throw std::runtime_error("meta-range cannot be empty");
        }
        for (size_type i = 1; i < size(); i++) {
          if (at(i).start() < at(i-1).stop()) {
            throw std::runtime_error("meta-range is not monotonic");
          }
        }
      }
    };
}
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_RANGE_H */

