/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
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
      typedef std::shared_ptr<chi_square_fit> sptr;

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
       * \param par_fit Are the parameters adjustable or are they supposed to be constant (0 means false, !=0 means yes).
       * \param par_lim_up hypercube sector of the function space(upper limit).
       * \param par_lim_dn hypercube sector of the function space(lower limit).
       * \param chi_square_error The allowable chi square error.
       */

      static sptr make(int in_vec_size,
          const std::string &func,
          double lim_up,
          double lim_dn,
          int n_params,
          const std::string &par_name,
          const std::vector<double> &par_init,
          const std::vector<double> &par_err,
          const std::vector<int> &par_fit, // bool_vec not supported...
          const std::vector<double> &par_lim_up,
          const std::vector<double> &par_lim_dn,
          double chi_square_error);

      /*!
       * \brief See constructor docs for more information.
       *
       * It should be noted the number of parameters is constant and cannot be changed at runtime.
       */
      virtual void update_design(
          const std::string &func,
          double lim_up,
          double lim_dn,
          const std::vector<double> &par_init,
          const std::vector<double> &par_err,
          const std::vector<int> &par_fit,
          const std::vector<double> &par_lim_up,
          const std::vector<double> &par_lim_dn,
          double chi_square_error) = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_CHI_SQUARE_FIT_H */

