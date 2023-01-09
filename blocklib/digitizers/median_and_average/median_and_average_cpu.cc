#include "median_and_average_cpu.h"
#include "median_and_average_cpu_gen.h"

namespace gr::digitizers {

median_and_average_cpu::median_and_average_cpu(const block_args& args)
    : INHERITED_CONSTRUCTORS
{
}

int compare_floats(const void* a, const void* b)
{
    float arg1 = *(const float*)a;
    float arg2 = *(const float*)b;

    if (arg1 < arg2)
        return -1;
    if (arg1 > arg2)
        return 1;
    return 0;

    // return (arg1 > arg2) - (arg1 < arg2); // possible shortcut
}

float median(float* buffer, int size)
{
    if (size <= 0)
        return 0;
    if (size <= 1)
        return buffer[0];

    qsort(buffer, size, sizeof(float), compare_floats);
    if (size % 2 == 0) {
        return 0.5 * (buffer[size / 2] + buffer[size / 2 - 1]);
    }
    else {
        return buffer[size / 2];
    }
}

float average(const float* buffer, int size)
{
    if (size <= 0)
        return 0;
    if (size <= 1)
        return buffer[0];

    float sum = 0.0;
    for (int i = 0; i < size; i++) {
        sum += buffer[i];
    }

    return sum / (float)size;
}

work_return_t median_and_average_cpu::work(work_io& wio)
{
    if (wio.inputs()[0].n_items == 0) {
        return work_return_t::INSUFFICIENT_INPUT_ITEMS;
    }

    if (wio.outputs()[0].n_items == 0) {
        return work_return_t::INSUFFICIENT_OUTPUT_ITEMS;
    }

    const auto in = wio.inputs()[0].items<float>();
    auto out = wio.outputs()[0].items<float>();

    const auto med_len = static_cast<int>(std::get<std::size_t>(*this->param_n_med));
    const auto avg_len = static_cast<int>(std::get<std::size_t>(*this->param_n_lp));
    const auto vec_len = static_cast<int>(std::get<std::size_t>(*this->param_vec_len));

    float buffer[2 * med_len + 1];
    float temp_buffer[vec_len];
    // calculate median of samples and average it.
    for (int i = 0; i < vec_len; i++) {
        if (med_len == 0) {
            temp_buffer[i] = in[i];
            // out[i] = in[i];
            continue;
        }
        int count = 0;
        for (int j = 0; j < (2 * med_len + 1); j++) {
            int k = i - med_len + j;
            if (k < 0) {
                k = 0;
            }
            else if (k >= vec_len) {
                k = vec_len - 1;
            }
            if (k != i) {
                buffer[count] = in[k];
                count++;
            }
        }
        temp_buffer[i] = median(buffer, count);
    }

    float buffer2[2 * avg_len + 1];
    for (int i = 0; i < vec_len; i++) {
        if (avg_len == 0) {
            out[i] = temp_buffer[i];
            continue;
        }
        int count = 0;
        for (int j = 0; j < (2 * avg_len + 1); j++) {
            int k = i - avg_len + j;
            if (k < 0) {
                k = 0;
            }
            else if (k >= vec_len) {
                k = vec_len - 1;
            }
            buffer2[count] = temp_buffer[k];
            count++;
        }

        out[i] = average(buffer2, count);
    }

    wio.consume_each(1);
    wio.produce_each(1);

    return work_return_t::OK;
}

} /* namespace gr::digitizers */
