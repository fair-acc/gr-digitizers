# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: power_calc_prepper_ff
# Author: neumann
# GNU Radio version: 3.10.1.1

from gnuradio import analog
from gnuradio import blocks
from gnuradio import filter
from gnuradio.filter import firdes
from gnuradio import gr
from gnuradio.fft import window
import sys
import signal







class power_calc_prepper_ff(gr.hier_block2):
    def __init__(self, bp_decimantion=20, bp_high_cut=80, bp_low_cut=20, bp_trans=10, current_correction_factor=2.5, in_samp_rate=2000000, lp_decimantion=1, out_samp_rate=100000, voltage_correction_factor=100):
        gr.hier_block2.__init__(
            self, "power_calc_prepper_ff",
                gr.io_signature.makev(2, 2, [gr.sizeof_float*1, gr.sizeof_float*1]),
                gr.io_signature.makev(5, 5, [gr.sizeof_float*1, gr.sizeof_float*1, gr.sizeof_float*1, gr.sizeof_float*1, gr.sizeof_float*1]),
        )

        ##################################################
        # Parameters
        ##################################################
        self.bp_decimantion = bp_decimantion
        self.bp_high_cut = bp_high_cut
        self.bp_low_cut = bp_low_cut
        self.bp_trans = bp_trans
        self.current_correction_factor = current_correction_factor
        self.in_samp_rate = in_samp_rate
        self.lp_decimantion = lp_decimantion
        self.out_samp_rate = out_samp_rate
        self.voltage_correction_factor = voltage_correction_factor

        ##################################################
        # Blocks
        ##################################################
        self.rational_resampler_xxx_0_0 = filter.rational_resampler_fff(
                interpolation=1,
                decimation=bp_decimantion,
                taps=[],
                fractional_bw=0)
        self.rational_resampler_xxx_0 = filter.rational_resampler_fff(
                interpolation=1,
                decimation=bp_decimantion,
                taps=[],
                fractional_bw=0)
        self.low_pass_filter_0_1_2 = filter.fir_filter_fff(
            lp_decimantion,
            firdes.low_pass(
                1,
                out_samp_rate,
                60,
                10,
                window.WIN_HAMMING,
                6.76))
        self.low_pass_filter_0_1_1 = filter.fir_filter_fff(
            lp_decimantion,
            firdes.low_pass(
                1,
                out_samp_rate,
                60,
                10,
                window.WIN_HAMMING,
                6.76))
        self.low_pass_filter_0_1_0 = filter.fir_filter_fff(
            lp_decimantion,
            firdes.low_pass(
                1,
                out_samp_rate,
                60,
                10,
                window.WIN_HAMMING,
                6.76))
        self.low_pass_filter_0_1 = filter.fir_filter_fff(
            lp_decimantion,
            firdes.low_pass(
                1,
                out_samp_rate,
                60,
                10,
                window.WIN_HAMMING,
                6.76))
        self.blocks_transcendental_0_0 = blocks.transcendental('atan', "float")
        self.blocks_transcendental_0 = blocks.transcendental('atan', "float")
        self.blocks_sub_xx_0 = blocks.sub_ff(1)
        self.blocks_multiply_xx_0_2 = blocks.multiply_vff(1)
        self.blocks_multiply_xx_0_1 = blocks.multiply_vff(1)
        self.blocks_multiply_xx_0_0 = blocks.multiply_vff(1)
        self.blocks_multiply_xx_0 = blocks.multiply_vff(1)
        self.blocks_multiply_const_vxx_0_0 = blocks.multiply_const_ff(current_correction_factor)
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_ff(voltage_correction_factor)
        self.blocks_divide_xx_0_0 = blocks.divide_ff(1)
        self.blocks_divide_xx_0 = blocks.divide_ff(1)
        self.band_pass_filter_0_0 = filter.fir_filter_fff(
            bp_decimantion,
            firdes.band_pass(
                1,
                in_samp_rate,
                bp_low_cut,
                bp_high_cut,
                bp_trans,
                window.WIN_HANN,
                6.76))
        self.band_pass_filter_0 = filter.fir_filter_fff(
            bp_decimantion,
            firdes.band_pass(
                1,
                in_samp_rate,
                bp_low_cut,
                bp_high_cut,
                bp_trans,
                window.WIN_HANN,
                6.76))
        self.analog_sig_source_x_0_1 = analog.sig_source_f(out_samp_rate, analog.GR_SIN_WAVE, 55, 1, 0, 0)
        self.analog_sig_source_x_0_0_0 = analog.sig_source_f(out_samp_rate, analog.GR_COS_WAVE, 55, 1, 0, 0)


        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_sig_source_x_0_0_0, 0), (self.blocks_multiply_xx_0_0, 1))
        self.connect((self.analog_sig_source_x_0_0_0, 0), (self.blocks_multiply_xx_0_1, 1))
        self.connect((self.analog_sig_source_x_0_1, 0), (self.blocks_multiply_xx_0, 1))
        self.connect((self.analog_sig_source_x_0_1, 0), (self.blocks_multiply_xx_0_2, 1))
        self.connect((self.band_pass_filter_0, 0), (self.blocks_multiply_xx_0_0, 0))
        self.connect((self.band_pass_filter_0, 0), (self.blocks_multiply_xx_0_2, 0))
        self.connect((self.band_pass_filter_0, 0), (self, 1))
        self.connect((self.band_pass_filter_0_0, 0), (self.blocks_multiply_xx_0, 0))
        self.connect((self.band_pass_filter_0_0, 0), (self.blocks_multiply_xx_0_1, 0))
        self.connect((self.band_pass_filter_0_0, 0), (self, 2))
        self.connect((self.blocks_divide_xx_0, 0), (self.blocks_transcendental_0, 0))
        self.connect((self.blocks_divide_xx_0_0, 0), (self.blocks_transcendental_0_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.band_pass_filter_0_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.rational_resampler_xxx_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0_0, 0), (self.band_pass_filter_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0_0, 0), (self.rational_resampler_xxx_0_0, 0))
        self.connect((self.blocks_multiply_xx_0, 0), (self.low_pass_filter_0_1, 0))
        self.connect((self.blocks_multiply_xx_0_0, 0), (self.low_pass_filter_0_1_0, 0))
        self.connect((self.blocks_multiply_xx_0_1, 0), (self.low_pass_filter_0_1_1, 0))
        self.connect((self.blocks_multiply_xx_0_2, 0), (self.low_pass_filter_0_1_2, 0))
        self.connect((self.blocks_sub_xx_0, 0), (self, 0))
        self.connect((self.blocks_transcendental_0, 0), (self.blocks_sub_xx_0, 0))
        self.connect((self.blocks_transcendental_0_0, 0), (self.blocks_sub_xx_0, 1))
        self.connect((self.low_pass_filter_0_1, 0), (self.blocks_divide_xx_0, 0))
        self.connect((self.low_pass_filter_0_1_0, 0), (self.blocks_divide_xx_0_0, 1))
        self.connect((self.low_pass_filter_0_1_1, 0), (self.blocks_divide_xx_0, 1))
        self.connect((self.low_pass_filter_0_1_2, 0), (self.blocks_divide_xx_0_0, 0))
        self.connect((self, 0), (self.blocks_multiply_const_vxx_0_0, 0))
        self.connect((self, 1), (self.blocks_multiply_const_vxx_0, 0))
        self.connect((self.rational_resampler_xxx_0, 0), (self, 4))
        self.connect((self.rational_resampler_xxx_0_0, 0), (self, 3))


    def get_bp_decimantion(self):
        return self.bp_decimantion

    def set_bp_decimantion(self, bp_decimantion):
        self.bp_decimantion = bp_decimantion

    def get_bp_high_cut(self):
        return self.bp_high_cut

    def set_bp_high_cut(self, bp_high_cut):
        self.bp_high_cut = bp_high_cut
        self.band_pass_filter_0.set_taps(firdes.band_pass(1, self.in_samp_rate, self.bp_low_cut, self.bp_high_cut, self.bp_trans, window.WIN_HANN, 6.76))
        self.band_pass_filter_0_0.set_taps(firdes.band_pass(1, self.in_samp_rate, self.bp_low_cut, self.bp_high_cut, self.bp_trans, window.WIN_HANN, 6.76))

    def get_bp_low_cut(self):
        return self.bp_low_cut

    def set_bp_low_cut(self, bp_low_cut):
        self.bp_low_cut = bp_low_cut
        self.band_pass_filter_0.set_taps(firdes.band_pass(1, self.in_samp_rate, self.bp_low_cut, self.bp_high_cut, self.bp_trans, window.WIN_HANN, 6.76))
        self.band_pass_filter_0_0.set_taps(firdes.band_pass(1, self.in_samp_rate, self.bp_low_cut, self.bp_high_cut, self.bp_trans, window.WIN_HANN, 6.76))

    def get_bp_trans(self):
        return self.bp_trans

    def set_bp_trans(self, bp_trans):
        self.bp_trans = bp_trans
        self.band_pass_filter_0.set_taps(firdes.band_pass(1, self.in_samp_rate, self.bp_low_cut, self.bp_high_cut, self.bp_trans, window.WIN_HANN, 6.76))
        self.band_pass_filter_0_0.set_taps(firdes.band_pass(1, self.in_samp_rate, self.bp_low_cut, self.bp_high_cut, self.bp_trans, window.WIN_HANN, 6.76))

    def get_current_correction_factor(self):
        return self.current_correction_factor

    def set_current_correction_factor(self, current_correction_factor):
        self.current_correction_factor = current_correction_factor
        self.blocks_multiply_const_vxx_0_0.set_k(self.current_correction_factor)

    def get_in_samp_rate(self):
        return self.in_samp_rate

    def set_in_samp_rate(self, in_samp_rate):
        self.in_samp_rate = in_samp_rate
        self.band_pass_filter_0.set_taps(firdes.band_pass(1, self.in_samp_rate, self.bp_low_cut, self.bp_high_cut, self.bp_trans, window.WIN_HANN, 6.76))
        self.band_pass_filter_0_0.set_taps(firdes.band_pass(1, self.in_samp_rate, self.bp_low_cut, self.bp_high_cut, self.bp_trans, window.WIN_HANN, 6.76))

    def get_lp_decimantion(self):
        return self.lp_decimantion

    def set_lp_decimantion(self, lp_decimantion):
        self.lp_decimantion = lp_decimantion

    def get_out_samp_rate(self):
        return self.out_samp_rate

    def set_out_samp_rate(self, out_samp_rate):
        self.out_samp_rate = out_samp_rate
        self.analog_sig_source_x_0_0_0.set_sampling_freq(self.out_samp_rate)
        self.analog_sig_source_x_0_1.set_sampling_freq(self.out_samp_rate)
        self.low_pass_filter_0_1.set_taps(firdes.low_pass(1, self.out_samp_rate, 60, 10, window.WIN_HAMMING, 6.76))
        self.low_pass_filter_0_1_0.set_taps(firdes.low_pass(1, self.out_samp_rate, 60, 10, window.WIN_HAMMING, 6.76))
        self.low_pass_filter_0_1_1.set_taps(firdes.low_pass(1, self.out_samp_rate, 60, 10, window.WIN_HAMMING, 6.76))
        self.low_pass_filter_0_1_2.set_taps(firdes.low_pass(1, self.out_samp_rate, 60, 10, window.WIN_HAMMING, 6.76))

    def get_voltage_correction_factor(self):
        return self.voltage_correction_factor

    def set_voltage_correction_factor(self, voltage_correction_factor):
        self.voltage_correction_factor = voltage_correction_factor
        self.blocks_multiply_const_vxx_0.set_k(self.voltage_correction_factor)

