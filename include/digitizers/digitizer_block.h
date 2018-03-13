/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


#ifndef INCLUDED_DIGITIZERS_DIGITIZER_BLOCK_H
#define INCLUDED_DIGITIZERS_DIGITIZER_BLOCK_H

#include <digitizers/api.h>
#include <digitizers/range.h>
#include <digitizers/error.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace digitizers {

    /*!
     * \brief An enum representing acquisition mode
     * \ingroup digitizers
     */
    enum DIGITIZERS_API acquisition_mode_t
    {
      RAPID_BLOCK,
      STREAMING
    };

    /*!
     * \brief Specifies a trigger mechanism
     * \ingroup digitizers
     */
    enum DIGITIZERS_API trigger_direction_t
    {
        TRIGGER_DIRECTION_RISING,
        TRIGGER_DIRECTION_FALLING,
        TRIGGER_DIRECTION_LOW,
        TRIGGER_DIRECTION_HIGH
    };

    /*!
     * \brief Downsampling mode
     * \ingroup digitizers
     */
    enum DIGITIZERS_API downsampling_mode_t
    {
      DOWNSAMPLING_MODE_NONE,
      DOWNSAMPLING_MODE_MIN_MAX_AGG,
      DOWNSAMPLING_MODE_DECIMATE,
      DOWNSAMPLING_MODE_AVERAGE,
    };

    /*! 
     * \brief Base class for digitizer blocks
     *
     * Note, both the value and the error estimate output needs to be connected for enabled
     * channels. If error estimate is not required it can be connected to a null sink.
     *
     *
     *
     * \ingroup digitizers
     */
    class DIGITIZERS_API digitizer_block : virtual public gr::sync_block
    {

    public:

      /*!
       * \brief Gets acquisition mode.
       */
      virtual acquisition_mode_t get_acquisition_mode() = 0;

      /*!
       * \brief Configure number of pre- and post-trigger samples.
       *
       * Note in streaming mode pre- and post-trigger samples are used only when
       * a trigger is enabled, that a triggered_data tag is added allowing other
       * blocks to extracts trigger data if desired so.
       *
       * \param samples the number of samples to acquire before the trigger event
       * \param pre_samples the number of samples to acquire
       */
      virtual void set_samples(int samples, int pre_samples = 0) = 0;

      /*!
       * \brief Sets maximum buffer size in samples per channel.
       *
       * Applicable for streaming mode only.
       * \param buffer_size the buffer size in samples
       */
      virtual void set_buffer_size(int buffer_size) = 0;

      /*!
       * \brief If auto arm is set then this block will automatically arm or rearm
       * the device, that is initially on start and afterwards whenever a desired
       * number of blocks is collected.
       *
       * \param auto_arm
       */
      virtual void set_auto_arm(bool auto_arm) = 0;

      /*!
       * \brief Arm device only once.
       *
       * Useful in combination with rapid-block mode only.
       *
       * \param auto_arm
       */
      virtual void set_trigger_once(bool auto_arm) = 0;

      /*! 
       * \brief Set rapid block mode.
       * 
       * \param nr_waveforms
       */
      virtual void set_rapid_block(int nr_waveforms) = 0;
      
      /*!
       * \brief Set streaming mode.
       *
       * \param poll_rate
       */
      virtual void set_streaming(double poll_rate=0.001) = 0;

      /*!
       * \brief Set downsampling mode and downsampling factor.
       *
       * If downsampling equals 0, i.e. no downsampling, the factor is disregarded.
       *
       * \param mode The type of downsampling the user wishes for the data acquisition.
       * \param downsample_factor The number of samples to be squished into a downsampled
       * sample.
       */
      virtual void set_downsampling(downsampling_mode_t mode, int downsample_factor=0) = 0;

      /*! 
       * \brief Set the sample rate.
       * \param rate a new rate in Sps
       */
      virtual void set_samp_rate(double rate) = 0;

      /*!
       * \brief Get the sample rate for this device.
       * This is the actual sample rate and may differ from the rate set.
       * \return the actual rate in Sps
       */
      virtual double get_samp_rate() = 0;

      /*! 
       * \brief Get AI channel names.
       * \return a vector of channel names, e.g. "A", "B", ...
       */
      virtual std::vector<std::string> get_aichan_ids() = 0;

      /*!
       * \brief Get available AI ranges.
       * \return available ranges
       */
      virtual meta_range_t get_aichan_ranges() = 0;

      /*! 
       * \brief Configure an AI channel
       *
       * Note, the underlying code is not thread safe. Disarm the device before changing
       * configuration.
       *
       * \param id Channel name e.g. "A", "B", "C", "D", ...
       * \param enabled Set desired state. Enabled or disabled channel
       * \param range desired voltage range in Volts
       * \param dc_coupling the coupling type
       * \param range_offset desired voltage offset in Volts
       */
      virtual void set_aichan(const std::string &id, bool enabled, double range, bool dc_coupling, double range_offset = 0) = 0;

      /*!
       * \brief Configure an AI channel with user defined range and offset.
       *
       * \param range desired voltage range in Volts
       * \param range_offset desired voltage offset in Volts
       */
      virtual void set_aichan_range(const std::string &id, double range, double range_offset = 0) = 0;

      /*!
       * \brief Configure an AI channel trigger
       * \param id Channel name e.g. "A", "B", "C", "D", ...
       * \param direction Trigger direction
       * \param threshold Triggering voltage. The channel has to be over it for the device to trigger.
       */
      virtual void set_aichan_trigger(const std::string &id, trigger_direction_t direction, double threshold) = 0;

      /**
       * \brief Set up a digital port, with user defined threshold and triggering mask.
       *
       * \param id port id
       * \param enabled Set desired state. Enabled or disabled port
       * \param thresh_voltage desired threshold voltage of the logic value switch(0<==>1)
       */
      virtual void set_diport(const std::string &id, bool enabled, double thresh_voltage) = 0;

      /*!
       * \brief Set up a digital input trigger.
       *
       * Note this interface assumes that digital inputs are numbered from 0..MAX_DI_CHANNELS
       * and it does not distinguish between ports. For example port 0 might contain 8 DI
       * channels and port 1 another 8 channels. It is assumed that the first pin of port 0
       * has a label 0 and last pin on second port label 15.
       *
       */
      virtual void set_di_trigger(uint32_t pin, trigger_direction_t direction) = 0;

      /*!
       * \brief Disable triggers if enabled.
       *
       * This function is forseen to be used mostly for testing purposes where it is desidred
       * to have an potion for changing configuration run-time.
       */
      virtual void disable_triggers() = 0;

      /*!
       * \brief explicitly initialize connection to the device
       */
      virtual void initialize() = 0;

      /*!
       * \brief Applies configuration
       */
      virtual void configure() = 0;

      /*!
       * \brief Arms the device with the given settings
       */
      virtual void arm() = 0;

      /*!
       * \brief returns the arm state of the device
       */
      virtual bool is_armed() = 0;

      /*!
       * \brief Disarms the device.
       */
      virtual void disarm() = 0;

      /*!
       * \brief Closes the device. Disarms if necessary
       */
      virtual void close() = 0;

      /*!
       * \brief Gets the list of the last 128 errors or warnings that
       * have occurred organized by time, with their corresponding timestamp.
       */
      virtual std::vector<err::error_info_t> get_errors() = 0;
    };

  } // namespace digitizers
} // namespace gr

#endif /* INCLUDED_DIGITIZERS_DIGITIZER_BLOCK_H */

