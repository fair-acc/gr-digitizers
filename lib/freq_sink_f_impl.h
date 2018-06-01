/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_FREQ_SINK_F_IMPL_H
#define INCLUDED_DIGITIZERS_FREQ_SINK_F_IMPL_H

#include <digitizers/freq_sink_f.h>
#include <digitizers/tags.h>

#include <boost/thread/mutex.hpp>
#include <vector>
#include "utils.h"

namespace gr {
  namespace digitizers {

    class freq_sink_f_impl : public freq_sink_f
    {
     private:
      freq_sink_mode_t d_mode;
      signal_metadata_t d_metadata;
      float d_samp_rate;

      // callback stuff
      data_available_cb_t d_callback;
      void *d_user_data;

      // sizing
      size_t d_nbins;
      size_t d_nmeasurements;
      size_t d_nbuffers;

      // Simple helper structure holding measurement data & metadata
      struct freq_domain_buffer_t
      {
        freq_domain_buffer_t(unsigned nmeasurements, unsigned nbins)
          : metadata(nmeasurements),
            freq(nmeasurements * nbins),
            magnitude(nmeasurements * nbins),
            phase(nmeasurements * nbins),
            nmeasurements(0)
        { }

        std::vector<spectra_measurement_t> metadata;
        std::vector<float> freq;
        std::vector<float> magnitude;
        std::vector<float> phase;

        // number of measurements stored in the buffer
        unsigned nmeasurements;
      };

      measurement_buffer_t<freq_domain_buffer_t> d_measurement_buffer;
      boost::circular_buffer<acq_info_t> d_acq_info_tags;
      unsigned d_lost_count;


     public:
      freq_sink_f_impl(std::string name, float samp_rate, size_t nbins,
              size_t nmeasurements, size_t nbuffers, freq_sink_mode_t mode);

      ~freq_sink_f_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items) override;

      signal_metadata_t get_metadata() override;

      size_t get_measurements(size_t nr_measurements,
         spectra_measurement_t *metadata, float *frequency, float *magnitude, float *phase) override;

      void set_callback(data_available_cb_t callback, void *ptr) override;

      size_t get_nbins() override;

      size_t get_nmeasurements() override;

      float get_sample_rate() override;

      freq_sink_mode_t get_sink_mode() override;

     private:

      /*!
       * \brief In fact there is nothing much we can do here. The acq_info tags (and other tags
       * as well) are attached to a vector and not to the individual item in the vector therefore
       * we cannot calculate timestamp with ns-level precision.
       */
      acq_info_t calculate_acq_info_for_vector(uint64_t offset) const;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_FREQ_SINK_F_IMPL_H */

