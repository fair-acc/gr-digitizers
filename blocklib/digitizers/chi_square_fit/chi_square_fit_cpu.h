#pragma once

#include <gnuradio/digitizers/chi_square_fit.h>

#include <TF1.h>
#include <TGraphErrors.h>

namespace gr::digitizers {

class chi_square_fit_cpu : public chi_square_fit
{
public:
    explicit chi_square_fit_cpu(const block_args& args);

    work_return_t work(work_io& wio) override;

    void on_parameter_change(param_action_sptr action) override;

private:
    void do_update_design();

    std::vector<std::string> d_par_names;
    std::vector<double> d_par_initial_values;
    std::vector<int8_t> d_par_fittable;
    std::vector<double> d_par_upper_limit;
    std::vector<double> d_par_lower_limit;

    // used by the work function
    TF1 d_func;
    double d_chi_error = 0; // snapshot
    std::shared_ptr<TGraphErrors> d_samps;
    std::vector<float> d_xvals;

    bool d_design_updated = false;
};

} // namespace gr::digitizers
