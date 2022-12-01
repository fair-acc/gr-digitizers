#include "peak_detector_cpu.h"
#include "peak_detector_cpu_gen.h"
#include "utils.h"

/**
 *
 * interpolation using a Gaussian interpolation
 *
 * @param data, data array
 * @param index, 0< index < data.length
 * @return location of the to be interpolated peak [bins]
 */
static float
interpolateGaussian(const float* data, const int data_length, const int index)
{
    if ((index > 0) && (index < (data_length - 1))) {
        const float left = std::pow(data[index - 1], 1);
        const float center = std::pow(data[index - 0], 1);
        const float right = std::pow(data[index + 1], 1);

        float val = index;
        val +=
            0.5 * std::log(right / left) / std::log(std::pow(center, 2) / (left * right));
        return val;
    }
    else {
        return ((float)index);
    }
}

static float linearInterpolate(float x0, float x1, float y0, float y1, float y)
{
    return x0 + (y - y0) * (x1 - x0) / (y1 - y0);
}

/**
 *
 * compute simple Full-Width-Half-Maximum (no inter-bin interpolation)
 *
 * @param data, data array
 * @param index, 0< index < data.length
 * @return FWHM estimate [bins]
 */
static float computeFWHM(const float* data, const int data_length, const int index)
{
    if ((index > 0) && (index < (data_length - 1))) {
        float maxHalf = 0.5 * data[index];
        int lowerLimit;
        int upperLimit;
        for (upperLimit = index; upperLimit < data_length && data[upperLimit] > maxHalf;
             upperLimit++)
            ;
        for (lowerLimit = index; lowerLimit > 0 && data[lowerLimit] > maxHalf;
             lowerLimit--)
            ;

        return (upperLimit - lowerLimit);
    }
    else {
        return 1.0f;
    }
}

/**
 *
 * compute interpolated Full-Width-Half-Maximum
 *
 * @param data, data array
 * @param index, 0< index < data.length
 * @return FWHM estimate [bins]
 */
static float
computeInterpolatedFWHM(const float* data, const int data_length, const int index)
{
    if ((index > 0) && (index < (data_length - 1))) {
        float maxHalf = 0.5 * data[index];
        int lowerLimit;
        int upperLimit;
        for (upperLimit = index; upperLimit < data_length && data[upperLimit] > maxHalf;
             upperLimit++)
            ;
        for (lowerLimit = index; lowerLimit > 0 && data[lowerLimit] > maxHalf;
             lowerLimit--)
            ;

        float lowerLimitRefined = linearInterpolate(
            lowerLimit, lowerLimit + 1, data[lowerLimit], data[lowerLimit + 1], maxHalf);
        float upperLimitRefined = linearInterpolate(
            upperLimit - 1, upperLimit, data[upperLimit - 1], data[upperLimit], maxHalf);

        return (upperLimitRefined - lowerLimitRefined);
    }
    else {
        return 1.0f;
    }
}

namespace gr::digitizers {

peak_detector_cpu::peak_detector_cpu(const block_args& args) : INHERITED_CONSTRUCTORS {}

work_return_t peak_detector_cpu::work(work_io& wio)
{
    const auto actual = wio.inputs()[0].items<float>();
    const auto filtered = wio.inputs()[1].items<float>();
    const auto low_freq = wio.inputs()[2].items<float>();
    const auto up_freq = wio.inputs()[3].items<float>();

    auto max_sig = wio.outputs()[0].items<float>();
    auto width_sig = wio.outputs()[1].items<float>();

    const auto noutput_items = wio.outputs()[0].n_items;

    const auto prox = static_cast<int>(pmtf::get_as<std::size_t>(*this->param_proximity));
    const auto vec_len =
        static_cast<int>(pmtf::get_as<std::size_t>(*this->param_vec_len));
    const auto freq = static_cast<int>(pmtf::get_as<std::size_t>(*this->param_samp_rate));

    // TODO: verify bounds
    int d_start_bin = 2.0 * low_freq[0] / freq * vec_len;
    int d_end_bin = 2.0 * up_freq[0] / freq * vec_len;

    // find filtered maximum
    float max_fil = filtered[d_start_bin];
    int max_fil_i = d_start_bin;
    for (int i = d_start_bin + 1; i <= d_end_bin; i++) {
        if (max_fil < filtered[i]) {
            max_fil = filtered[i];
            max_fil_i = i;
        }
    }
    // find actual maximum in the proximity of the averaged maximum
    // initialize to something smaller than actual max.
    float max = max_fil;
    int max_i = max_fil_i;
    for (int i = 0; i < prox; i++) {
        if (max_fil_i + i < vec_len && max < actual[max_fil_i + i]) {
            max = actual[max_fil_i + i];
            max_i = max_fil_i + i;
        }
        if (max_fil_i - i >= 0 && max < actual[max_fil_i - i]) {
            max = actual[max_fil_i - i];
            max_i = max_fil_i - i;
        }
    }

    // find FWHM for stdev approx.
    double hm = actual[max_i] / 2.0;
    int whm_i = max_i;
    for (int i = 0; i < vec_len && whm_i == max_i; i++) {
        if (max_i + i < vec_len && hm > actual[max_i + i]) {
            whm_i = max_i + i;
        }
        if (max_i - i >= 0 && hm > actual[max_i - i]) {
            whm_i = max_i - i;
        }
    }
    double freq_whm = 0.0;
    if (whm_i > max_i) {
        double a = actual[whm_i] - actual[whm_i - 1];
        double b = actual[whm_i - 1];
        freq_whm = (b + (hm - b)) / a;
    }
    if (whm_i < max_i) {
        double a = actual[whm_i + 1] - actual[whm_i];
        double b = actual[whm_i];
        freq_whm = (b + (hm - b)) / a;
    }

    // freq_whm = computeFWHM(actual, d_vec_len, max_i);
    freq_whm = computeInterpolatedFWHM(actual, vec_len, max_i);

    // fix width to half maximum from bin count to frequency window
    freq_whm *= freq / (vec_len);

    // see CAS Reference in Common Spec:
    //
    float maxInterpolated = interpolateGaussian(actual, vec_len, max_i);

    max_sig[0] = (maxInterpolated * freq) / (2.0 * vec_len);
    width_sig[0] = freq_whm * whm2stdev;

    // TODO(PORT) was consume_each(noutput_items). Did that make any sense in GR3? (leads to extra calls
    // to work() with ginormous n_items for inputs 0 and 1.
    wio.consume_each(1);

    /*
    max_sig[0] = (max_i * d_freq) / (2.0 * d_vec_len);
    width_sig[0] = freq_whm * whm2stdev;
    consume_each (1);
    */

    wio.produce_each(1);

    return work_return_t::OK;
}

} /* namespace gr::digitizers */
