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


#ifndef INCLUDED_DIGITIZERS_CHI_SQUARE_FIT_H
#define INCLUDED_DIGITIZERS_CHI_SQUARE_FIT_H

#include <digitizers/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief It accepts vectors of samples in the time/freq domain
     * and tries to fit the given function from lim_dn to lim_up of the input vector.
     * The functions parameters will be adjusted in each iteration. The parameters of
     * the function are bordered in a hypercube with size of par_lim_dn -> par_lim_up.
     * Each given parameter has a corresponding flag, weather it is constant or not.
     *
     * The initial values of the parameter are supplied in par_init.
     *
     * \ingroup digitizers
     *
     */
    class DIGITIZERS_API chi_square_fit : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<chi_square_fit> sptr;

      /*!
       * \brief Create a chi square fitting block.
       *
       * \param in_vec_size Input vector length
       * \param func The function to fit.
       * \param lim_up upper limit of the samples. Effectively subsample of input vector
       * \param lim_dn lower limit of the samples. Effectively subsample of input vector
       * \param n_params number of parameters to be fitted.
       * \param par_name a naming convention of the parameters.
       * \param par_init initial values of the parameters.
       * \param par_err initial parameter error estimations.
       * \param par_fit Are the parameters adjustable or are they supposed to be constant.
       * \param par_lim_up hypercube sector of the function space(upper limit).
       * \param par_lim_dn hypercube sector of the function space(lower limit).
       * \param chi_square_error The allowable chi square error.
       */

      static sptr make(int in_vec_size,
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
          double chi_square_error);
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_CHI_SQUARE_FIT_H */

