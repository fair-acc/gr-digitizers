#include "block_spectral_peaks_cpu.h"
#include "block_spectral_peaks_cpu_gen.h"

namespace gr::digitizers {

block_spectral_peaks_cpu::block_spectral_peaks_cpu(const block_args& args)
    : INHERITED_CONSTRUCTORS,
      d_med_avg(median_and_average::make({ args.fft_window, args.n_med, args.n_avg })),
      d_peaks(peak_detector::make({ args.samp_rate, args.fft_window, args.n_prox }))
{
    connect(self(), 0, d_med_avg, 0);
    connect(self(), 0, d_peaks, 0);
    connect(self(), 1, d_peaks, 2);
    connect(self(), 2, d_peaks, 3);
    connect(d_med_avg, 0, d_peaks, 1);
    connect(d_med_avg, 0, self(), 0);
    connect(d_peaks, 0, self(), 1);
    connect(d_peaks, 1, self(), 2);
}

} // namespace gr::digitizers
