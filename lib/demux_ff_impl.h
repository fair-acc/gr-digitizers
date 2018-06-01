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
      WaitTrigger        = 1,
      WaitEvent          = 2,
      WaitRealignmentTag = 3,
      CalcOutputRange    = 4,
      WaitAllData        = 5,
      OutputData         = 6
    };

    class demux_ff_impl : public demux_ff
    {
     private:
      float d_samp_rate;
      unsigned d_my_history;
      unsigned d_pre_trigger_window;
      unsigned d_post_trigger_window;

      extractor_state d_state;

      // absolute sample count where the last timing event appeared
      uint64_t d_last_trigger_offset;
      boost::optional<wr_event_t> d_last_wr_event;
      boost::optional<edge_detect_t> d_last_edge;

      uint64_t d_trigger_start_range;
      uint64_t d_trigger_end_range;

      boost::circular_buffer<wr_event_t> d_wr_events;
      boost::circular_buffer<edge_detect_t> d_realignment_events;

      boost::circular_buffer<acq_info_t> d_acq_info;

     public:
      demux_ff_impl(float samp_rate, unsigned history, unsigned post_trigger_window, unsigned pre_trigger_window);
      ~demux_ff_impl();

      bool start() override;

      void forecast(int noutput_items, gr_vector_int &ninput_items_required) override;

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items) override;

     private:
      double get_user_delay() const;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_DEMUX_FF_IMPL_H */

