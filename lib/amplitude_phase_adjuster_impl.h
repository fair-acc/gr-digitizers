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

#ifndef INCLUDED_DIGITIZERS_AMPLITUDE_PHASE_ADJUSTER_IMPL_H
#define INCLUDED_DIGITIZERS_AMPLITUDE_PHASE_ADJUSTER_IMPL_H

#include <digitizers/amplitude_phase_adjuster.h>

namespace gr {
  namespace digitizers {

    class amplitude_phase_adjuster_impl : public amplitude_phase_adjuster
    {
     private:
      double d_ampl;
      double d_phi;
      double d_phi_fq_factor;

     public:
      amplitude_phase_adjuster_impl(double ampl_usr, double phi_usr, double phi_fq_usr);
      ~amplitude_phase_adjuster_impl();

      int work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items);

      void update_design(double amplitude_usr,
        double phi_usr,
        double phi_fq_usr);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_AMPLITUDE_PHASE_ADJUSTER_IMPL_H */

