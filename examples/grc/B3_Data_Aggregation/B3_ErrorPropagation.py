#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: B3 Errorpropagation
# Description: computes low-pass filter and decimation including computiation/propagation of error estimates. Algorithm ID selects filter implementation (0: FIR-LP, 1: FIR-BP, 2: FIR-CUSTOM, 3: FIR-CUSTOM_FFT, 4: IIR_LP, 5: IIR_HP, 6: IIR-CUSTOM)
# Generated: Tue May 22 13:51:38 2018
##################################################

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print "Warning: failed to XInitThreads()"

import os
import sys
sys.path.append(os.environ.get('GRC_HIER_PATH', os.path.expanduser('~/.grc_gnuradio')))

from PyQt4 import Qt
from block_error_band import block_error_band  # grc-generated hier_block
from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import qtgui
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from gnuradio.qtgui import Range, RangeWidget
from optparse import OptionParser
import digitizers
import pmt
import sip
from gnuradio import qtgui


class B3_ErrorPropagation(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "B3 Errorpropagation")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("B3 Errorpropagation")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except:
            pass
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("GNU Radio", "B3_ErrorPropagation")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())


        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 8000
        self.freq_transition = freq_transition = 100
        self.freq_max = freq_max = 1000
        self.out_delay = out_delay = 2
        self.input_noise_est = input_noise_est = 0.6
        self.freq_min = freq_min = 2
        self.fir_LP = fir_LP =  firdes.low_pass(1, samp_rate, freq_max, freq_transition, firdes.WIN_HAMMING, 6.76)
        self.filter_taps_b = filter_taps_b = 0.0009959859297622798, -0.003939740084930013, 0.005887765960493963, -0.003939740084930017, 0.0009959859297622804
        self.filter_taps_a = filter_taps_a = 1.0, -3.9692103976755453, 5.9090978836797685, -3.9105406695954064, 0.9706534726794167
        self.decimation = decimation = 10

        ##################################################
        # Blocks
        ##################################################
        self._out_delay_range = Range(0, 2*samp_rate, 10, 2, 200)
        self._out_delay_win = RangeWidget(self._out_delay_range, self.set_out_delay, "'Out' Path Delay", "counter", int)
        self.top_grid_layout.addWidget(self._out_delay_win, 5,3,1,1)
        self._input_noise_est_range = Range(0, 10, 0.1, 0.6, 200)
        self._input_noise_est_win = RangeWidget(self._input_noise_est_range, self.set_input_noise_est, 'Input Noise Estimate', "counter", float)
        self.top_grid_layout.addWidget(self._input_noise_est_win, 5,2,1,1)
        self._freq_transition_range = Range(10, 1000, 10, 100, 200)
        self._freq_transition_win = RangeWidget(self._freq_transition_range, self.set_freq_transition, 'Transition Width [Hz]', "counter_slider", float)
        self.top_grid_layout.addWidget(self._freq_transition_win, 4,2,1,2)
        self._freq_min_range = Range(1, 1000, 1, 2, 200)
        self._freq_min_win = RangeWidget(self._freq_min_range, self.set_freq_min, 'Min Frequency [Hz]', "counter_slider", float)
        self.top_grid_layout.addWidget(self._freq_min_win, 4,0,1,2)
        self._freq_max_range = Range(1, 1000, 1, 1000, 200)
        self._freq_max_win = RangeWidget(self._freq_max_range, self.set_freq_max, 'Max Frequency [Hz]', "counter_slider", float)
        self.top_grid_layout.addWidget(self._freq_max_win, 5,0,1,2)
        self.qtgui_time_sink_x_0_0 = qtgui.time_sink_f(
        	samp_rate, #size
        	samp_rate, #samp_rate
        	"", #name
        	2 #number of inputs
        )
        self.qtgui_time_sink_x_0_0.set_update_time(0.1)
        self.qtgui_time_sink_x_0_0.set_y_axis(-2, 2)

        self.qtgui_time_sink_x_0_0.set_y_label('Input Amplitude', "")

        self.qtgui_time_sink_x_0_0.enable_tags(-1, False)
        self.qtgui_time_sink_x_0_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.qtgui_time_sink_x_0_0.enable_autoscale(False)
        self.qtgui_time_sink_x_0_0.enable_grid(False)
        self.qtgui_time_sink_x_0_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0_0.enable_control_panel(False)

        if not True:
          self.qtgui_time_sink_x_0_0.disable_legend()

        labels = ['raw data', 'true y(t)', 'est: y(t)', 'est: y(t) - sigma(t)', 'true y(t)',
                  '', '', '', '', '']
        widths = [1, 2, 1, 1, 2,
                  1, 1, 1, 1, 1]
        colors = ["blue", "green", "red", "black", "green",
                  "magenta", "yellow", "dark red", "dark green", "blue"]
        styles = [2, 1, 1, 2, 1,
                  1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
                   -1, -1, -1, -1, -1]
        alphas = [1.0, 1.0, 1.0, 1.0, 1,
                  1.0, 1.0, 1.0, 1.0, 1.0]

        for i in xrange(2):
            if len(labels[i]) == 0:
                self.qtgui_time_sink_x_0_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_time_sink_x_0_0.set_line_label(i, labels[i])
            self.qtgui_time_sink_x_0_0.set_line_width(i, widths[i])
            self.qtgui_time_sink_x_0_0.set_line_color(i, colors[i])
            self.qtgui_time_sink_x_0_0.set_line_style(i, styles[i])
            self.qtgui_time_sink_x_0_0.set_line_marker(i, markers[i])
            self.qtgui_time_sink_x_0_0.set_line_alpha(i, alphas[i])

        self._qtgui_time_sink_x_0_0_win = sip.wrapinstance(self.qtgui_time_sink_x_0_0.pyqwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_time_sink_x_0_0_win, 0,0,2,4)
        self.digitizers_block_aggregation_0 = digitizers.block_aggregation(1, decimation, out_delay, (fir_LP), freq_min, freq_max, freq_transition, (filter_taps_a), (filter_taps_b), samp_rate)
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_float*1, samp_rate,True)
        self.blocks_tags_strobe_0 = blocks.tags_strobe(gr.sizeof_float*1, pmt.make_tuple(pmt.from_uint64(10000), pmt.from_double(1.0/samp_rate), pmt.from_double(0.0), pmt.from_double(0.0), pmt.from_long(0), pmt.from_long(0), pmt.from_long(0), pmt.from_uint64(0), pmt.from_uint64(5000), pmt.from_bool(True)), 4, pmt.intern("acq_info"))
        self.blocks_tag_debug_0 = blocks.tag_debug(gr.sizeof_float*1, '', ""); self.blocks_tag_debug_0.set_display(True)
        self.blocks_keep_one_in_n_0 = blocks.keep_one_in_n(gr.sizeof_float*1, decimation)
        self.blocks_add_xx_0_0 = blocks.add_vff(1)
        self.blocks_add_xx_0 = blocks.add_vff(1)
        self.block_error_band_0 = block_error_band()
        self.analog_sig_source_x_0 = analog.sig_source_f(samp_rate, analog.GR_SQR_WAVE, 2, 1, 0)
        self.analog_noise_source_x_0 = analog.noise_source_f(analog.GR_UNIFORM, 1, 0)
        self.analog_const_source_x_0 = analog.sig_source_f(0, analog.GR_CONST_WAVE, 0, 0, input_noise_est)
        self.Aggregation_Sample_Example = qtgui.time_sink_f(
        	samp_rate/decimation, #size
        	samp_rate/decimation, #samp_rate
        	"", #name
        	4 #number of inputs
        )
        self.Aggregation_Sample_Example.set_update_time(0.1)
        self.Aggregation_Sample_Example.set_y_axis(-2, 2)

        self.Aggregation_Sample_Example.set_y_label('Filtered Amplitude', "a.u.")

        self.Aggregation_Sample_Example.enable_tags(-1, False)
        self.Aggregation_Sample_Example.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.Aggregation_Sample_Example.enable_autoscale(True)
        self.Aggregation_Sample_Example.enable_grid(False)
        self.Aggregation_Sample_Example.enable_axis_labels(True)
        self.Aggregation_Sample_Example.enable_control_panel(False)

        if not True:
          self.Aggregation_Sample_Example.disable_legend()

        labels = ['est: y(t) + sigma(t)', 'est: y(t)', 'est: y(t) + sigma(t)', 'true y(t)', 'true y(t)',
                  '', '', '', '', '']
        widths = [1, 2, 1, 2, 2,
                  1, 1, 1, 1, 1]
        colors = ["red", "red", "red", "green", "green",
                  "magenta", "yellow", "dark red", "dark green", "blue"]
        styles = [2, 1, 2, 1, 1,
                  1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
                   -1, -1, -1, -1, -1]
        alphas = [1.0, 1.0, 1.0, 1.0, 1,
                  1.0, 1.0, 1.0, 1.0, 1.0]

        for i in xrange(4):
            if len(labels[i]) == 0:
                self.Aggregation_Sample_Example.set_line_label(i, "Data {0}".format(i))
            else:
                self.Aggregation_Sample_Example.set_line_label(i, labels[i])
            self.Aggregation_Sample_Example.set_line_width(i, widths[i])
            self.Aggregation_Sample_Example.set_line_color(i, colors[i])
            self.Aggregation_Sample_Example.set_line_style(i, styles[i])
            self.Aggregation_Sample_Example.set_line_marker(i, markers[i])
            self.Aggregation_Sample_Example.set_line_alpha(i, alphas[i])

        self._Aggregation_Sample_Example_win = sip.wrapinstance(self.Aggregation_Sample_Example.pyqwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._Aggregation_Sample_Example_win, 2,0,2,4)



        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_const_source_x_0, 0), (self.digitizers_block_aggregation_0, 1))
        self.connect((self.analog_noise_source_x_0, 0), (self.blocks_add_xx_0, 0))
        self.connect((self.analog_sig_source_x_0, 0), (self.blocks_add_xx_0, 1))
        self.connect((self.analog_sig_source_x_0, 0), (self.blocks_keep_one_in_n_0, 0))
        self.connect((self.analog_sig_source_x_0, 0), (self.qtgui_time_sink_x_0_0, 1))
        self.connect((self.block_error_band_0, 0), (self.Aggregation_Sample_Example, 0))
        self.connect((self.block_error_band_0, 2), (self.Aggregation_Sample_Example, 2))
        self.connect((self.block_error_band_0, 1), (self.Aggregation_Sample_Example, 1))
        self.connect((self.blocks_add_xx_0, 0), (self.blocks_add_xx_0_0, 0))
        self.connect((self.blocks_add_xx_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.blocks_add_xx_0_0, 0), (self.digitizers_block_aggregation_0, 0))
        self.connect((self.blocks_keep_one_in_n_0, 0), (self.Aggregation_Sample_Example, 3))
        self.connect((self.blocks_tags_strobe_0, 0), (self.blocks_add_xx_0_0, 1))
        self.connect((self.blocks_throttle_0, 0), (self.qtgui_time_sink_x_0_0, 0))
        self.connect((self.digitizers_block_aggregation_0, 0), (self.block_error_band_0, 0))
        self.connect((self.digitizers_block_aggregation_0, 1), (self.block_error_band_0, 1))
        self.connect((self.digitizers_block_aggregation_0, 0), (self.blocks_tag_debug_0, 0))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "B3_ErrorPropagation")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_fir_LP( firdes.low_pass(1, self.samp_rate, self.freq_max, self.freq_transition, firdes.WIN_HAMMING, 6.76))
        self.qtgui_time_sink_x_0_0.set_samp_rate(self.samp_rate)
        self.digitizers_block_aggregation_0.update_design(self.out_delay, (self.fir_LP), self.freq_min, self.freq_max, self.freq_transition, (self.filter_taps_a), (self.filter_taps_b), self.samp_rate)
        self.blocks_throttle_0.set_sample_rate(self.samp_rate)
        self.blocks_tags_strobe_0.set_value(pmt.make_tuple(pmt.from_uint64(10000), pmt.from_double(1.0/self.samp_rate), pmt.from_double(0.0), pmt.from_double(0.0), pmt.from_long(0), pmt.from_long(0), pmt.from_long(0), pmt.from_uint64(0), pmt.from_uint64(5000), pmt.from_bool(True)))
        self.analog_sig_source_x_0.set_sampling_freq(self.samp_rate)
        self.Aggregation_Sample_Example.set_samp_rate(self.samp_rate/self.decimation)

    def get_freq_transition(self):
        return self.freq_transition

    def set_freq_transition(self, freq_transition):
        self.freq_transition = freq_transition
        self.set_fir_LP( firdes.low_pass(1, self.samp_rate, self.freq_max, self.freq_transition, firdes.WIN_HAMMING, 6.76))
        self.digitizers_block_aggregation_0.update_design(self.out_delay, (self.fir_LP), self.freq_min, self.freq_max, self.freq_transition, (self.filter_taps_a), (self.filter_taps_b), self.samp_rate)

    def get_freq_max(self):
        return self.freq_max

    def set_freq_max(self, freq_max):
        self.freq_max = freq_max
        self.set_fir_LP( firdes.low_pass(1, self.samp_rate, self.freq_max, self.freq_transition, firdes.WIN_HAMMING, 6.76))
        self.digitizers_block_aggregation_0.update_design(self.out_delay, (self.fir_LP), self.freq_min, self.freq_max, self.freq_transition, (self.filter_taps_a), (self.filter_taps_b), self.samp_rate)

    def get_out_delay(self):
        return self.out_delay

    def set_out_delay(self, out_delay):
        self.out_delay = out_delay
        self.digitizers_block_aggregation_0.update_design(self.out_delay, (self.fir_LP), self.freq_min, self.freq_max, self.freq_transition, (self.filter_taps_a), (self.filter_taps_b), self.samp_rate)

    def get_input_noise_est(self):
        return self.input_noise_est

    def set_input_noise_est(self, input_noise_est):
        self.input_noise_est = input_noise_est
        self.analog_const_source_x_0.set_offset(self.input_noise_est)

    def get_freq_min(self):
        return self.freq_min

    def set_freq_min(self, freq_min):
        self.freq_min = freq_min
        self.digitizers_block_aggregation_0.update_design(self.out_delay, (self.fir_LP), self.freq_min, self.freq_max, self.freq_transition, (self.filter_taps_a), (self.filter_taps_b), self.samp_rate)

    def get_fir_LP(self):
        return self.fir_LP

    def set_fir_LP(self, fir_LP):
        self.fir_LP = fir_LP
        self.digitizers_block_aggregation_0.update_design(self.out_delay, (self.fir_LP), self.freq_min, self.freq_max, self.freq_transition, (self.filter_taps_a), (self.filter_taps_b), self.samp_rate)

    def get_filter_taps_b(self):
        return self.filter_taps_b

    def set_filter_taps_b(self, filter_taps_b):
        self.filter_taps_b = filter_taps_b
        self.digitizers_block_aggregation_0.update_design(self.out_delay, (self.fir_LP), self.freq_min, self.freq_max, self.freq_transition, (self.filter_taps_a), (self.filter_taps_b), self.samp_rate)

    def get_filter_taps_a(self):
        return self.filter_taps_a

    def set_filter_taps_a(self, filter_taps_a):
        self.filter_taps_a = filter_taps_a
        self.digitizers_block_aggregation_0.update_design(self.out_delay, (self.fir_LP), self.freq_min, self.freq_max, self.freq_transition, (self.filter_taps_a), (self.filter_taps_b), self.samp_rate)

    def get_decimation(self):
        return self.decimation

    def set_decimation(self, decimation):
        self.decimation = decimation
        self.blocks_keep_one_in_n_0.set_n(self.decimation)
        self.Aggregation_Sample_Example.set_samp_rate(self.samp_rate/self.decimation)


def main(top_block_cls=B3_ErrorPropagation, options=None):

    from distutils.version import StrictVersion
    if StrictVersion(Qt.qVersion()) >= StrictVersion("4.5.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()
    tb.start()
    tb.show()

    def quitting():
        tb.stop()
        tb.wait()
    qapp.connect(qapp, Qt.SIGNAL("aboutToQuit()"), quitting)
    qapp.exec_()


if __name__ == '__main__':
    main()
