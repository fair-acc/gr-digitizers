#include "time_domain_sink_cpu.h"
#include "time_domain_sink_cpu_gen.h"

namespace gr::digitizers {

template <class T>
time_domain_sink_cpu<T>::time_domain_sink_cpu(
    const typename time_domain_sink<T>::block_args& args)
    : INHERITED_CONSTRUCTORS(T)
{
    if (args.output_package_size == 0) {
        throw std::runtime_error(
            fmt::format("Exception in:{}:{} Channel: {} cannot set output_multiple to 0",
                        __FILE__,
                        __LINE__,
                        args.signal_name));
    }

    try {
        // TODO(PORT) check if this actually throws
        this->set_output_multiple(args.output_package_size);
    } catch (const std::exception& ex) {
        throw std::runtime_error(fmt::format("Exception in:{}:{} Channel: {} Error: {}",
                                             __FILE__,
                                             __LINE__,
                                             args.signal_name,
                                             ex.what()));
    }

    this->set_tag_propagation_policy(tag_propagation_policy_t::TPP_DONT);
}

template <class T>
void time_domain_sink_cpu<T>::set_callback(
    std::function<
        void(std::vector<float>, std::vector<float>, std::vector<gr::tag_t>, void*)> cb,
    void* user_data)
{
    d_cb_copy_data = cb;
    d_userdata = user_data;
}

template <class T>
work_return_t time_domain_sink_cpu<T>::work(work_io& wio)
{
    const auto ninput_items = wio.inputs()[0].n_items;

    const auto output_package_size =
        pmtf::get_as<std::size_t>(*this->param_output_package_size);

    assert(ninput_items % output_package_size == 0);

    if (!d_cb_copy_data) { // FIXME: uncomment when all sink types are supported by FESA
        // GR_LOG_WARN(d_logger, "Callback for sink '" + d_metadata.name + "' is not
        // initialized");
        wio.consume_each(ninput_items);
        return work_return_t::OK;
    }

    const auto input_values = wio.inputs()[0].items<float>();

    // consume package by package
    for (std::size_t first = 0; first < ninput_items; first += output_package_size) {
        const auto last = first + output_package_size;

        /* get tags for this package */
        auto tags = wio.inputs()[0].tags_in_window(first, last);

        std::vector<float> values(&input_values[first], &input_values[last]);

        std::vector<float> errors;
        if (wio.inputs().size() > 1) {
            const auto input_errors = wio.inputs()[1].items<float>();
            errors = std::vector<float>(&input_errors[first], &input_errors[last]);
        }

        /* trigger callback of host application to copy the data*/
        d_cb_copy_data(std::move(values), std::move(errors), std::move(tags), d_userdata);
    }

    wio.consume_each(ninput_items);
    return work_return_t::OK;
}

} /* namespace gr::digitizers */
