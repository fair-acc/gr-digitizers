/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_CHI_SQUARE_FIT_IMPL_H
#define INCLUDED_DIGITIZERS_CHI_SQUARE_FIT_IMPL_H

#include <digitizers/chi_square_fit.h>
#include <TF1.h>
#include <TGraphErrors.h>
#include <boost/thread/mutex.hpp>

namespace gr {
  namespace digitizers {

    class chi_square_fit_impl : public chi_square_fit
    {
     private:
      // input/output constraints & statics
      int d_vec_len;
      int d_n_params;
      std::vector<std::string> d_par_names;

      // design
      std::string d_function;
      double d_function_lower_limit;
      double d_function_upper_limit;
      std::vector<double> d_par_initial_values;
      std::vector<double> d_par_initial_errors;
      std::vector<int> d_par_fittable;
      std::vector<double> d_par_lower_limit;
      std::vector<double> d_par_upper_limit;
      double d_max_chi_square_error;

      // used by the work function
      TF1 d_func;
      double d_chi_error; // snapshot
      boost::shared_ptr<TGraphErrors> d_samps;
      std::vector<float> d_xvals;

      boost::mutex d_mutex;
      bool d_design_updated;

     public:
      chi_square_fit_impl(int in_vec_size,
          const std::string &func,
          double lim_up,
          double lim_dn,
          int n_params,
          const std::vector<std::string> &par_name,
          const std::vector<double> &par_init,
          const std::vector<double> &par_err,
          const std::vector<int> &par_fit,
          const std::vector<double> &par_lim_up,
          const std::vector<double> &par_lim_dn,
          double chi_square_error);

      ~chi_square_fit_impl();

      void update_design(
          const std::string &func,
          double lim_up,
          double lim_dn,
          const std::vector<double> &par_init,
          const std::vector<double> &par_err,
          const std::vector<int> &par_fit,
          const std::vector<double> &par_lim_up,
          const std::vector<double> &par_lim_dn,
          double chi_square_error) override;

      void forecast(int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

     private:
      // updates all the member variables used for fitting
      void do_update_design();
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_CHI_SQUARE_FIT_IMPL_H */

