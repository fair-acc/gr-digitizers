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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "chi_square_fit_impl.h"
#include <boost/tokenizer.hpp>
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

    std::vector<bool> parse_fixable(std::string str)
    {
      std::vector<bool> fixable;
      boost::char_separator<char> sep(", ");
      boost::tokenizer< boost::char_separator<char> > tokens(str, sep);
      for(auto& t: tokens) {
        if(std::toupper(t.at(0)) == 'T') {
          fixable.push_back(true);
        }
        else {
          fixable.push_back(false);
        }
      }
      return fixable;
    }

    chi_square_fit::sptr
    chi_square_fit::make(int in_vec_size,
        std::string func,
        double lim_up,
        double lim_dn,
        int n_params,
        std::string par_name,
        std::vector<double> par_init,
        std::vector<double> par_err,
        std::string par_fit,
        std::vector<double> par_lim_up,
        std::vector<double> par_lim_dn,
        double chi_square_error)
    {
      std::vector<std::string> names = parse_names(par_name);
      std::vector<bool> fixable = parse_fixable(par_fit);
      if(names.size() != static_cast<size_t>(n_params)) {
        throw std::invalid_argument("Parameter names do not match! must be of type:\"pName0\", \"pName1\", ..., \"pName[n_params]\"");
      }
      if(fixable.size() != static_cast<size_t>(n_params)) {
        throw std::invalid_argument("Parameter Fittable does not match! must be of type:True/False, True/False, ..., True/False[n_params]");
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
            fixable,
            par_lim_up,
            par_lim_dn,
            chi_square_error));
    }

    chi_square_fit_impl::chi_square_fit_impl(int in_vec_size,
      std::string &formula,
      double lim_up,
      double lim_dn,
      int n_params,
      std::vector<std::string> &par_name,
      std::vector<double> &par_init,
      std::vector<double> &par_err,
      std::vector<bool> &par_fit,
      std::vector<double> &par_lim_up,
      std::vector<double> &par_lim_dn,
      double chi_square_error)
      : gr::block("chi_square_fit",
              gr::io_signature::make(1, 1, sizeof(float)*in_vec_size),
              gr::io_signature::makev(4, 4,
                  std::vector<int>({
                    static_cast<int>(sizeof(float))*n_params,
                    static_cast<int>(sizeof(float))*n_params,
                    sizeof(float),
                    sizeof(char)}))),
              d_vec_len(in_vec_size),
              d_n_params(n_params),
              d_func("func", formula.c_str(), lim_dn, lim_up),
              d_chi_error(chi_square_error)
    {
      d_samps = NULL;
      for (int i=0; i < n_params; i++) {
        d_func.SetParName(i, par_name[i].c_str());
        d_func.SetParameter(i, par_init[i]);
        d_func.SetParLimits(i, par_lim_dn[i], par_lim_up[i]);

        // fix parameter, if the parameter range is zero or inverted
        if (par_lim_dn[i] >= par_lim_up[i] || !par_fit.at(i)) {
          //Info(functionName,"fixed parameter %i %f %f", i, parameterRangeMin[i], parameterRangeMin[i]);
          d_func.FixParameter(i, par_init[i]);
        }
        par_err[i] = 0.0;
      }
      d_xvals.reserve(d_vec_len);
      double step = (lim_up-lim_dn)/(1.0*d_vec_len-1.0);
      for(int i =0; i<d_vec_len;i++) { d_xvals.push_back(lim_dn + i*step); }
    }

    chi_square_fit_impl::~chi_square_fit_impl()
    {
    }

    void
    chi_square_fit_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = noutput_items;
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

      const float *in = (const float *) input_items[0];
      float *params = (float *) output_items[0];
      float *errs = (float *) output_items[1];
      float *chi_sq = (float *) output_items[2];
      char *valid = (char *) output_items[3];

      const Char_t *fitterOptions = "0NEQR";

      d_samps = boost::shared_ptr<TGraphErrors>(new TGraphErrors(d_vec_len, d_xvals.data(), in));
      auto func = d_func;
      d_samps->Fit(&func, fitterOptions);
      for(int i = 0; i < d_n_params; i++) {
        params[i]=static_cast<float>(func.GetParameter(i));
        errs[i]=static_cast<float>(func.GetParError(i));
      }
      double chiSquare = func.GetChisquare();
      int    NDF = func.GetNDF();
      chi_sq[0] = chiSquare/NDF;
      valid[0] = std::abs(chi_sq[0]-1.0)<d_chi_error? 1 : 0;

      consume_each(1);
      return 1;
    }

  } /* namespace digitizers */
} /* namespace gr */

