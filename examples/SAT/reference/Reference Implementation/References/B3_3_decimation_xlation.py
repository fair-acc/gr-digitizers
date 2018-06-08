#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: B3 3 Decimation Xlation
# Author: rstein
# Description: Demo to illustrate the signal down-mixing and decimation module
# Generated: Wed Aug 16 11:15:03 2017
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

from PyQt4 import Qt
from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio import qtgui
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from gnuradio.qtgui import Range, RangeWidget
from optparse import OptionParser
import sip
import sys
from gnuradio import qtgui


class B3_3_decimation_xlation(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "B3 3 Decimation Xlation")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("B3 3 Decimation Xlation")
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

        self.settings = Qt.QSettings("GNU Radio", "B3_3_decimation_xlation")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())

        ##################################################
        # Variables
        ##################################################
        self.transitionWidth = transitionWidth = 0.1
        self.samp_rate = samp_rate = 20000000
        self.freq = freq = 7.5
        self.decimation = decimation = 5
        self.dcCutOff = dcCutOff = 0.1
        self.centerFreq = centerFreq = 7
        self.bandWidth = bandWidth = 1

        ##################################################
        # Blocks
        ##################################################
        self._transitionWidth_range = Range(0.05, 1, 0.05, 0.1, 200)
        self._transitionWidth_win = RangeWidget(self._transitionWidth_range, self.set_transitionWidth, 'Transition Width [MHz]', "counter_slider", float)
        self.top_grid_layout.addWidget(self._transitionWidth_win, 4,2,2,2)
        self._freq_range = Range(0.1, 10, 0.1, 7.5, 200)
        self._freq_win = RangeWidget(self._freq_range, self.set_freq, 'Single Tone [MHz]', "counter_slider", float)
        self.top_grid_layout.addWidget(self._freq_win, 6,0,2,2)
        self._dcCutOff_range = Range(0, 10, 0.1, 0.1, 200)
        self._dcCutOff_win = RangeWidget(self._dcCutOff_range, self.set_dcCutOff, '(Lower) Bandwidth [MHz]', "counter_slider", float)
        self.top_grid_layout.addWidget(self._dcCutOff_win, 4,0,2,2)
        self._centerFreq_range = Range(0, 10, 0.1, 7, 200)
        self._centerFreq_win = RangeWidget(self._centerFreq_range, self.set_centerFreq, 'LO Frequency [MHz]', "counter_slider", float)
        self.top_grid_layout.addWidget(self._centerFreq_win, 7,0,2,2)
        self._bandWidth_range = Range(0, 10, 0.1, 1, 200)
        self._bandWidth_win = RangeWidget(self._bandWidth_range, self.set_bandWidth, '(Upper) Bandwidth [MHz]', "counter_slider", float)
        self.top_grid_layout.addWidget(self._bandWidth_win, 5,0,2,2)
        self.qtgui_freq_sink_x_0_0_0_0 = qtgui.freq_sink_f(
        	2048, #size
        	firdes.WIN_HANN, #wintype
        	0, #fc
        	samp_rate, #bw
        	"", #name
        	1 #number of inputs
        )
        self.qtgui_freq_sink_x_0_0_0_0.set_update_time(0.10)
        self.qtgui_freq_sink_x_0_0_0_0.set_y_axis(-140, 10)
        self.qtgui_freq_sink_x_0_0_0_0.set_y_label('Relative Gain', 'dB')
        self.qtgui_freq_sink_x_0_0_0_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, 0.0, 0, "")
        self.qtgui_freq_sink_x_0_0_0_0.enable_autoscale(True)
        self.qtgui_freq_sink_x_0_0_0_0.enable_grid(True)
        self.qtgui_freq_sink_x_0_0_0_0.set_fft_average(0.2)
        self.qtgui_freq_sink_x_0_0_0_0.enable_axis_labels(True)
        self.qtgui_freq_sink_x_0_0_0_0.enable_control_panel(False)

        if not True:
          self.qtgui_freq_sink_x_0_0_0_0.disable_legend()

        if "float" == "float" or "float" == "msg_float":
          self.qtgui_freq_sink_x_0_0_0_0.set_plot_pos_half(not False)

        labels = ['raw signal', 'low-pass', 'band-pass', '', '',
                  '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
                  "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]
        for i in xrange(1):
            if len(labels[i]) == 0:
                self.qtgui_freq_sink_x_0_0_0_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_freq_sink_x_0_0_0_0.set_line_label(i, labels[i])
            self.qtgui_freq_sink_x_0_0_0_0.set_line_width(i, widths[i])
            self.qtgui_freq_sink_x_0_0_0_0.set_line_color(i, colors[i])
            self.qtgui_freq_sink_x_0_0_0_0.set_line_alpha(i, alphas[i])

        self._qtgui_freq_sink_x_0_0_0_0_win = sip.wrapinstance(self.qtgui_freq_sink_x_0_0_0_0.pyqwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_freq_sink_x_0_0_0_0_win, 0,0,2,4)
        self.qtgui_freq_sink_x_0_0_0 = qtgui.freq_sink_c(
        	2048, #size
        	firdes.WIN_HANN, #wintype
        	0, #fc
        	samp_rate/decimation, #bw
        	"", #name
        	2 #number of inputs
        )
        self.qtgui_freq_sink_x_0_0_0.set_update_time(0.1)
        self.qtgui_freq_sink_x_0_0_0.set_y_axis(-140, 10)
        self.qtgui_freq_sink_x_0_0_0.set_y_label('Relative Gain', 'dB')
        self.qtgui_freq_sink_x_0_0_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, 0.0, 0, "")
        self.qtgui_freq_sink_x_0_0_0.enable_autoscale(True)
        self.qtgui_freq_sink_x_0_0_0.enable_grid(True)
        self.qtgui_freq_sink_x_0_0_0.set_fft_average(0.2)
        self.qtgui_freq_sink_x_0_0_0.enable_axis_labels(True)
        self.qtgui_freq_sink_x_0_0_0.enable_control_panel(False)

        if not True:
          self.qtgui_freq_sink_x_0_0_0.disable_legend()

        if "complex" == "float" or "complex" == "msg_float":
          self.qtgui_freq_sink_x_0_0_0.set_plot_pos_half(not False)

        labels = ['low_pass', 'band-pass', 'band-pass', '', '',
                  '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["red", "green", "green", "black", "cyan",
                  "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]
        for i in xrange(2):
            if len(labels[i]) == 0:
                self.qtgui_freq_sink_x_0_0_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_freq_sink_x_0_0_0.set_line_label(i, labels[i])
            self.qtgui_freq_sink_x_0_0_0.set_line_width(i, widths[i])
            self.qtgui_freq_sink_x_0_0_0.set_line_color(i, colors[i])
            self.qtgui_freq_sink_x_0_0_0.set_line_alpha(i, alphas[i])

        self._qtgui_freq_sink_x_0_0_0_win = sip.wrapinstance(self.qtgui_freq_sink_x_0_0_0.pyqwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_freq_sink_x_0_0_0_win, 2,0,2,4)
        self.freq_xlating_fir_filter_xxx_1_0 = filter.freq_xlating_fir_filter_fcf(decimation, (firdes.low_pass(1,samp_rate, bandWidth*1e6, transitionWidth*1e6)), centerFreq*1e6, samp_rate)
        self.freq_xlating_fir_filter_xxx_1 = filter.freq_xlating_fir_filter_fcf(decimation, (firdes.band_pass(1,samp_rate, dcCutOff*1e6, bandWidth*1e6, transitionWidth*1e6)), centerFreq*1e6, samp_rate)
        self.blocks_throttle_0_0 = blocks.throttle(gr.sizeof_float*1, samp_rate,True)
        self.blocks_add_xx_0 = blocks.add_vff(1)
        self.analog_sig_source_x_0_0_0 = analog.sig_source_f(samp_rate, analog.GR_COS_WAVE, freq*1e6, 10, 0)
        self.analog_noise_source_x_0_0 = analog.noise_source_f(analog.GR_GAUSSIAN, 0.01, 0)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_noise_source_x_0_0, 0), (self.blocks_add_xx_0, 0))
        self.connect((self.analog_sig_source_x_0_0_0, 0), (self.blocks_add_xx_0, 1))
        self.connect((self.blocks_add_xx_0, 0), (self.blocks_throttle_0_0, 0))
        self.connect((self.blocks_add_xx_0, 0), (self.freq_xlating_fir_filter_xxx_1, 0))
        self.connect((self.blocks_add_xx_0, 0), (self.freq_xlating_fir_filter_xxx_1_0, 0))
        self.connect((self.blocks_throttle_0_0, 0), (self.qtgui_freq_sink_x_0_0_0_0, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_1, 0), (self.qtgui_freq_sink_x_0_0_0, 1))
        self.connect((self.freq_xlating_fir_filter_xxx_1_0, 0), (self.qtgui_freq_sink_x_0_0_0, 0))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "B3_3_decimation_xlation")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_transitionWidth(self):
        return self.transitionWidth

    def set_transitionWidth(self, transitionWidth):
        self.transitionWidth = transitionWidth
        self.freq_xlating_fir_filter_xxx_1_0.set_taps((firdes.low_pass(1,self.samp_rate, self.bandWidth*1e6, self.transitionWidth*1e6)))
        self.freq_xlating_fir_filter_xxx_1.set_taps((firdes.band_pass(1,self.samp_rate, self.dcCutOff*1e6, self.bandWidth*1e6, self.transitionWidth*1e6)))

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.qtgui_freq_sink_x_0_0_0_0.set_frequency_range(0, self.samp_rate)
        self.qtgui_freq_sink_x_0_0_0.set_frequency_range(0, self.samp_rate/self.decimation)
        self.freq_xlating_fir_filter_xxx_1_0.set_taps((firdes.low_pass(1,self.samp_rate, self.bandWidth*1e6, self.transitionWidth*1e6)))
        self.freq_xlating_fir_filter_xxx_1.set_taps((firdes.band_pass(1,self.samp_rate, self.dcCutOff*1e6, self.bandWidth*1e6, self.transitionWidth*1e6)))
        self.blocks_throttle_0_0.set_sample_rate(self.samp_rate)
        self.analog_sig_source_x_0_0_0.set_sampling_freq(self.samp_rate)

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self.analog_sig_source_x_0_0_0.set_frequency(self.freq*1e6)

    def get_decimation(self):
        return self.decimation

    def set_decimation(self, decimation):
        self.decimation = decimation
        self.qtgui_freq_sink_x_0_0_0.set_frequency_range(0, self.samp_rate/self.decimation)

    def get_dcCutOff(self):
        return self.dcCutOff

    def set_dcCutOff(self, dcCutOff):
        self.dcCutOff = dcCutOff
        self.freq_xlating_fir_filter_xxx_1.set_taps((firdes.band_pass(1,self.samp_rate, self.dcCutOff*1e6, self.bandWidth*1e6, self.transitionWidth*1e6)))

    def get_centerFreq(self):
        return self.centerFreq

    def set_centerFreq(self, centerFreq):
        self.centerFreq = centerFreq
        self.freq_xlating_fir_filter_xxx_1_0.set_center_freq(self.centerFreq*1e6)
        self.freq_xlating_fir_filter_xxx_1.set_center_freq(self.centerFreq*1e6)

    def get_bandWidth(self):
        return self.bandWidth

    def set_bandWidth(self, bandWidth):
        self.bandWidth = bandWidth
        self.freq_xlating_fir_filter_xxx_1_0.set_taps((firdes.low_pass(1,self.samp_rate, self.bandWidth*1e6, self.transitionWidth*1e6)))
        self.freq_xlating_fir_filter_xxx_1.set_taps((firdes.band_pass(1,self.samp_rate, self.dcCutOff*1e6, self.bandWidth*1e6, self.transitionWidth*1e6)))


def main(top_block_cls=B3_3_decimation_xlation, options=None):

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
