/* -*- c++ -*- */
/* Copyright (C) 2018 GSI Darmstadt, Germany - All Rights Reserved
 * co-developed with: Cosylab, Ljubljana, Slovenia and CERN, Geneva, Switzerland
 * You may use, distribute and modify this code under the terms of the GPL v.3  license.
 */

#ifndef INCLUDED_DIGITIZERS_DEMUX_FF_IMPL_H
#define INCLUDED_DIGITIZERS_DEMUX_FF_IMPL_H

#include <digitizers/demux_ff.h>
#include <digitizers/tags.h>
#include <digitizers/edge_trigger_utils.h>
#include <boost/circular_buffer.hpp>
#include <boost/optional.hpp>

#include "utils.h"

namespace gr {
  namespace digitizers {

    enum class extractor_state
    {
      WaitTrigger,
      CalcOutputRange,
      WaitAllData,
      OutputData
    };

    class demux_ff_impl : public demux_ff
    {
     private:
      unsigned d_my_history;
      unsigned d_pre_trigger_window;
      unsigned d_post_trigger_window;

      // <tag, offset-relative-to-trigger-tag>
      std::vector<std::pair<acq_info_t, int64_t> > d_acq_info_tags;

      extractor_state d_state;

      // absolute sample count where the last timing event appeared
      trigger_t d_trigger_tag_data;
      uint64_t d_last_trigger_offset;

      uint64_t d_trigger_start_range;
      uint64_t d_trigger_end_range;

     public:
      demux_ff_impl(unsigned post_trigger_window, unsigned pre_trigger_window);
      ~demux_ff_impl();

      bool start() override;

//      void forecast(int noutput_items, gr_vector_int &ninput_items_required) override;

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items) override;

    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_DEMUX_FF_IMPL_H */

