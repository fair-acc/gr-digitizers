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

#ifndef INCLUDED_DIGITIZERS_CHI_SQUARE_FIT_IMPL_H
#define INCLUDED_DIGITIZERS_CHI_SQUARE_FIT_IMPL_H

#include <digitizers/chi_square_fit.h>
#include <TF1.h>
#include <TGraphErrors.h>

namespace gr {
  namespace digitizers {

    class chi_square_fit_impl : public chi_square_fit
    {
     private:
      int d_vec_len;
      int d_n_params;
      TF1 d_func;
      double d_chi_error;
      boost::shared_ptr<TGraphErrors> d_samps;
      std::vector<float> d_xvals;

     public:
      chi_square_fit_impl(int in_vec_size,
          std::string &func,
          double lim_up,
          double lim_dn,
          int n_params,
          std::vector<std::string> &par_name,
          std::vector<double> &par_init,
          std::vector<double> &par_err,
          std::vector<bool> &par_fit,
          std::vector<double> &par_lim_up,
          std::vector<double> &par_lim_dn,
          double chi_square_error);
      ~chi_square_fit_impl();

      void forecast(int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_CHI_SQUARE_FIT_IMPL_H */

