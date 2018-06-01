/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "chi_square_fit_impl.h"
#include <boost/tokenizer.hpp>
#include <boost/make_shared.hpp>
#include <stdexcept>

namespace gr {
  namespace digitizers {

    std::vector<std::string> parse_names(std::string str)
    {
      std::vector<std::string> names;
      boost::char_separator<char> sep(", ");
      boost::tokenizer< boost::char_separator<char> > tokens(str, sep);

      for(auto& t: tokens) {
        std::string name = t;
        if(name.at(0) == '"') {
          name = name.substr(1, name.length()-2);
        }
        names.push_back(name);
      }

      return names;
    }

    chi_square_fit::sptr
    chi_square_fit::make(int in_vec_size,
        const std::string &func,
        double lim_up,
        double lim_dn,
        int n_params,
        const std::string &par_name,
        const std::vector<double> &par_init,
        const std::vector<double> &par_err,
        const std::vector<int> &par_fit,
        const std::vector<double> &par_lim_up,
        const std::vector<double> &par_lim_dn,
        double chi_square_error)
    {
      std::vector<std::string> names = parse_names(par_name);

      if(names.size() != static_cast<size_t>(n_params)) {
        throw std::invalid_argument("Parameter names do not match! must be of "
                "type:\"pName0\", \"pName1\", ..., \"pName[n_params]\"");
      }

      return gnuradio::get_initial_sptr
        (new chi_square_fit_impl(in_vec_size,
            func,
            lim_up,
            lim_dn,
            n_params,
            names,
            par_init,
            par_err,
            par_fit,
            par_lim_up,
            par_lim_dn,
            chi_square_error));
    }

    chi_square_fit_impl::chi_square_fit_impl(int in_vec_size,
        const std::string &formula,
        double lim_up,
        double lim_dn,
        int n_params,
        const std::vector<std::string> &par_name,
        const std::vector<double> &par_init,
        const std::vector<double> &par_err,
        const std::vector<int> &par_fit,
        const std::vector<double> &par_lim_up,
        const std::vector<double> &par_lim_dn,
        double chi_square_error)
      : gr::block("chi_square_fit",
              gr::io_signature::make(1, 1, sizeof(float) * in_vec_size),
              gr::io_signature::makev(4, 4,
                  std::vector<int>({
                    static_cast<int>(sizeof(float))*n_params,
                    static_cast<int>(sizeof(float))*n_params,
                    sizeof(float),
                    sizeof(char)}))),
       d_vec_len(in_vec_size),
       d_n_params(n_params),
       d_par_names(par_name)
    {
      d_xvals.reserve(d_vec_len);

      if ((int)d_par_names.size() < d_n_params) {
          throw std::invalid_argument("invalid parameter configuration");
      }

      update_design(formula, lim_up, lim_dn, par_init, par_err,
              par_fit, par_lim_up, par_lim_dn, chi_square_error);
    }

    chi_square_fit_impl::~chi_square_fit_impl()
    {
    }

    void
    chi_square_fit_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
    {
      for (auto & inrequired : ninput_items_required) {
        inrequired = noutput_items;
      }
    }

    void chi_square_fit_impl::update_design(
        const std::string &func,
        double lim_up,
        double lim_dn,
        const std::vector<double> &par_init,
        const std::vector<double> &par_err,
        const std::vector<int> &par_fit,
        const std::vector<double> &par_lim_up,
        const std::vector<double> &par_lim_dn,
        double chi_square_error)
    {
      // sanity check
      if (std::min({par_init.size(), par_err.size(), par_fit.size(), par_lim_up.size(), par_lim_dn.size()})
          != static_cast<size_t>(d_n_params))
      {
        throw std::invalid_argument("invalid parameter configuration supplied");
      }

      boost::mutex::scoped_lock lg(d_mutex);

      // save provided parameters
      d_function = func;
      d_function_lower_limit = lim_dn;
      d_function_upper_limit = lim_up;
      d_par_initial_values = par_init;
      d_par_initial_errors = par_err;
      d_par_fittable = par_fit;
      d_par_lower_limit = par_lim_dn;
      d_par_upper_limit = par_lim_up;
      d_max_chi_square_error = chi_square_error;

      d_design_updated = true;
    }

    void chi_square_fit_impl::do_update_design()
    {
      boost::mutex::scoped_lock lg(d_mutex);

      d_func = TF1("func", d_function.c_str(), d_function_lower_limit, d_function_upper_limit);

      for (int i = 0; i < d_n_params; i++) {
        d_func.SetParName(i, d_par_names[i].c_str());
      }

      double step = (d_function_upper_limit - d_function_lower_limit) / (1.0 * d_vec_len - 1.0);
      d_xvals.clear();
      for (int i = 0; i < d_vec_len; i++)
      {
        d_xvals.push_back(d_function_lower_limit + static_cast<double>(i) * step);
      }

      // take snapshot of certain parameters
      d_chi_error = d_max_chi_square_error;
    }

    int
    chi_square_fit_impl::general_work (int noutput_items,
      gr_vector_int &ninput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items)
    {
      //check if input items is less than the minimal number of samples for a chi square fit
      int in_items = *std::min_element(ninput_items.begin(), ninput_items.end());
      if(in_items == 0) {
        return 0;
      }

      if (d_design_updated) {
        do_update_design();
        d_design_updated = false;
      }
      for (int i = 0; i < d_n_params; i++) {
        d_func.SetParameter(i, d_par_initial_values[i]);
        d_func.SetParLimits(i, d_par_lower_limit[i], d_par_upper_limit[i]);

        // fix parameter, if the parameter range is zero or inverted
        if (d_par_lower_limit[i] >= d_par_upper_limit[i] || !d_par_fittable[i]) {
          d_func.FixParameter(i, d_par_initial_values[i]);
        }
      }

      const float *in = (const float *) input_items[0];
      float *params = (float *) output_items[0];
      float *errs = (float *) output_items[1];
      float *chi_sq = (float *) output_items[2];
      char *valid = (char *) output_items[3];

      assert((int)d_xvals.size() == d_vec_len);
      d_samps = boost::make_shared<TGraphErrors>(d_vec_len, d_xvals.data(), in);

      // Note a copy of the function object is made
      auto func = d_func;

      const Char_t *fitterOptions = "0NEQR";
      d_samps->Fit(&func, fitterOptions);

      for(int i = 0; i < d_n_params; i++) {
        params[i] = static_cast<float>(func.GetParameter(i));
        errs[i] = static_cast<float>(func.GetParError(i));
      }

      double chiSquare = func.GetChisquare();
      int    NDF = func.GetNDF();

      chi_sq[0] = chiSquare / NDF;
      valid[0] = std::abs(chi_sq[0] - 1.0) < d_chi_error ? 1 : 0;

      consume_each(1);
      return 1;
    }

  } /* namespace digitizers */
} /* namespace gr */

