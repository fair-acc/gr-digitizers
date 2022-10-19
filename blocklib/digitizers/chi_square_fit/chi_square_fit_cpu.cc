#include "chi_square_fit_cpu.h"
#include "chi_square_fit_cpu_gen.h"

#include <boost/tokenizer.hpp>

#include <algorithm>
#include <stdexcept>

namespace gr::digitizers {

static std::vector<std::string> parse_names(std::string str) {
    std::vector<std::string>                      names;
    boost::char_separator<char>                   sep(", ");
    boost::tokenizer<boost::char_separator<char>> tokens(str, sep);

    for (auto &t : tokens) {
        std::string name = t;
        if (name.at(0) == '"') {
            name = name.substr(1, name.length() - 2);
        }
        names.push_back(name);
    }

    return names;
}

template<class T>
chi_square_fit_cpu<T>::chi_square_fit_cpu(const typename chi_square_fit<T>::block_args &args)
    : INHERITED_CONSTRUCTORS(T)
    , d_par_names(parse_names(args.par_name))
    , d_par_initial_values(args.par_init)
    , d_par_fittable(args.par_fit)
    , d_par_upper_limit(args.par_lim_up)
    , d_par_lower_limit(args.par_lim_dn) {
    if (d_par_names.size() != args.n_params) {
        std::ostringstream message;
        message << "Exception in " << __FILE__ << ":" << __LINE__ << ": Parameter names do not match! must be of type:\"pName0\", \"pName1\", ..., \"pName[n_params]\"";
        throw std::invalid_argument(message.str());
    }

    do_update_design();
}

template<class T>
void chi_square_fit_cpu<T>::do_update_design() {
    const auto vec_len              = pmtf::get_as<std::size_t>(*this->param_in_vec_size);
    const auto function_upper_limit = pmtf::get_as<double>(*this->param_lim_up);
    const auto function_lower_limit = pmtf::get_as<double>(*this->param_lim_dn);
    const auto max_chi_square_error = pmtf::get_as<double>(*this->param_max_chi_square_error);
    const auto function             = pmtf::get_as<std::string>(*this->param_func);
    const auto n_params             = pmtf::get_as<std::size_t>(*this->param_n_params);

    d_func                          = TF1("func", function.c_str(), function_lower_limit, function_upper_limit);

    for (std::size_t i = 0; i < n_params; i++) {
        d_func.SetParName(i, d_par_names[i].c_str());
    }

    double step = (function_upper_limit - function_lower_limit) / (1.0 * vec_len - 1.0);
    d_xvals.clear();
    for (std::size_t i = 0; i < vec_len; i++) {
        d_xvals.push_back(function_lower_limit + static_cast<double>(i) * step);
    }

    // take snapshot of certain parameters
    d_chi_error = max_chi_square_error;
}

template<class T>
work_return_t chi_square_fit_cpu<T>::work(work_io &wio) {
    // check if input items is less than the minimal number of samples for a chi square fit
    if (wio.inputs()[0].n_items == 0) { // TODO(PORT) can this even happen in GR 4.0?
        return work_return_t::INSUFFICIENT_INPUT_ITEMS;
    }

    if (d_design_updated) {
        do_update_design();
        d_design_updated = false;
    }

    const auto vec_len  = pmtf::get_as<std::size_t>(*this->param_in_vec_size);
    const auto n_params = pmtf::get_as<std::size_t>(*this->param_n_params);

    for (std::size_t i = 0; i < n_params; i++) {
        d_func.SetParameter(i, d_par_initial_values[i]);
        d_func.SetParLimits(i, d_par_lower_limit[i], d_par_upper_limit[i]);

        // fix parameter, if the parameter range is zero or inverted
        if (d_par_lower_limit[i] >= d_par_upper_limit[i] || !d_par_fittable[i]) {
            d_func.FixParameter(i, d_par_initial_values[i]);
        }
    }

    const auto in     = wio.inputs()[0].items<float>();
    auto       params = wio.outputs()[0].items<float>();
    auto       errs   = wio.outputs()[1].items<float>();
    auto       chi_sq = wio.outputs()[2].items<float>();
    auto       valid  = wio.outputs()[3].items<uint8_t>();

    assert(d_xvals.size() == vec_len);
    d_samps = std::make_shared<TGraphErrors>(vec_len, d_xvals.data(), in);

    // Note a copy of the function object is made
    auto          func          = d_func;

    const Char_t *fitterOptions = "0NEQR";
    d_samps->Fit(&func, fitterOptions);

    for (std::size_t i = 0; i < n_params; i++) {
        params[i] = static_cast<float>(func.GetParameter(i));
        errs[i]   = static_cast<float>(func.GetParError(i));
    }

    double chiSquare = func.GetChisquare();
    int    NDF       = func.GetNDF();

    chi_sq[0]        = chiSquare / NDF;
    valid[0]         = std::abs(chi_sq[0] - 1.0) < d_chi_error ? 1 : 0;

    wio.consume_each(1);
    wio.produce_each(1);

    return work_return_t::OK;
}

template<class T>
std::vector<T> get_as_vector(const pmtf::pmt &pv) {
    const auto     v = pmtf::get_as<std::vector<pmtf::pmt>>(pv);
    std::vector<T> r;
    std::transform(v.begin(), v.end(), std::back_inserter(r), &pmtf::get_as<T>);
    return r;
}

template<class T>
void chi_square_fit_cpu<T>::on_parameter_change(param_action_sptr action) {
    block::on_parameter_change(action);

    d_design_updated = true;

    if (action->id() == chi_square_fit_cpu<T>::id_par_fit) {
        d_par_fittable = get_as_vector<int8_t>(*this->param_par_fit);
    } else if (action->id() == chi_square_fit_cpu<T>::id_par_lim_up) {
        d_par_upper_limit = get_as_vector<double>(*this->param_par_lim_up);
    } else if (action->id() == chi_square_fit_cpu<T>::id_par_lim_dn) {
        d_par_lower_limit = get_as_vector<double>(*this->param_par_lim_dn);
    }
}

} // namespace gr::digitizers
