#include "time_domain_sink_cpu.h"
#include "time_domain_sink_cpu_gen.h"

namespace gr::digitizers {

template <class T>
time_domain_sink_cpu<T>::time_domain_sink_cpu(const typename time_domain_sink<T>::block_args& args)
    : INHERITED_CONSTRUCTORS(T)
{
}

template <class T>
work_return_t time_domain_sink_cpu<T>::work(work_io& wio)
{
    // TODO port

    return work_return_t::OK;

#ifdef PORT_DISABLED
    assert(ninput_items % d_output_package_size == 0);

    if(d_cb_copy_data == nullptr )
    {   // FIXME: uncomment when all sink types are supported by FESA
        //GR_LOG_WARN(d_logger, "Callback for sink '" + d_metadata.name + "' is not initialized");
        return ninput_items;
    }

    const float *input_values = static_cast<const float *>(input_items[0]);
    const float *input_errors = static_cast<const float *>(input_items[1]);

    std::size_t  input_errors_size = 0;
    if (input_items.size() > 1)
        input_errors_size = d_output_package_size;

    auto tag_index = nitems_read(0);

    std::vector<gr::tag_t> tags;

    // consume package by package
    for (int i = 0; i < ninput_items; i+= d_output_package_size)
    {
        /* get tags for this package */
        get_tags_in_range(tags, 0, tag_index, tag_index + d_output_package_size);
        tag_index += d_output_package_size;

        /* trigger callback of host application to copy the data*/
        d_cb_copy_data(&input_values[i],
                        d_output_package_size,
                        &input_errors[i],
                        input_errors_size,
                        tags,d_userdata);
    }

#endif
}

} /* namespace gr::digitizers */
