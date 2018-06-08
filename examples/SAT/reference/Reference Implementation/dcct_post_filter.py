#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Dcct Post Filter
# Generated: Tue Aug 15 10:19:44 2017
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
from optparse import OptionParser
import sip
import sys
from gnuradio import qtgui


class dcct_post_filter(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Dcct Post Filter")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Dcct Post Filter")
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

        self.settings = Qt.QSettings("GNU Radio", "dcct_post_filter")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())

        ##################################################
        # Variables
        ##################################################
        self.sqrt2 = sqrt2 = 1.4142135623731
        self.samp_rate = samp_rate = 100000
        self.freq_max = freq_max = 2000
        self.PI = PI = 3.14159265359
        self.freq_min = freq_min = 30
        self.freq_max_corr = freq_max_corr = freq_max/(2*(sqrt2-1))
        self.Ts = Ts = 1.0/(samp_rate)
        self.TWO_PI = TWO_PI = 2*PI
        self.decimate = decimate = 200
        self.alpha_low = alpha_low = TWO_PI*Ts*freq_min/(TWO_PI*Ts*freq_min+1)
        self.alpha = alpha = TWO_PI*Ts*freq_max_corr/(TWO_PI*Ts*freq_max_corr+1)

        ##################################################
        # Blocks
        ##################################################
        self.single_pole_iir_filter_xx_0_1 = filter.single_pole_iir_filter_ff(alpha_low, 1)
        self.single_pole_iir_filter_xx_0_0 = filter.single_pole_iir_filter_ff(alpha, 1)
        self.single_pole_iir_filter_xx_0 = filter.single_pole_iir_filter_ff(alpha, 1)
        self.qtgui_time_sink_x_0 = qtgui.time_sink_f(
        	samp_rate, #size
        	samp_rate, #samp_rate
        	"", #name
        	2 #number of inputs
        )
        self.qtgui_time_sink_x_0.set_update_time(0.10)
        self.qtgui_time_sink_x_0.set_y_axis(-1, 11)

        self.qtgui_time_sink_x_0.set_y_label('Amplitude', "")

        self.qtgui_time_sink_x_0.enable_tags(-1, False)
        self.qtgui_time_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.qtgui_time_sink_x_0.enable_autoscale(False)
        self.qtgui_time_sink_x_0.enable_grid(False)
        self.qtgui_time_sink_x_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0.enable_control_panel(False)

        if not True:
          self.qtgui_time_sink_x_0.disable_legend()

        labels = ['y(t)', 'dy(t)/dt', '', '', '',
                  '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
                  "magenta", "yellow", "dark red", "dark green", "blue"]
        styles = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
                   -1, -1, -1, -1, -1]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]

        for i in xrange(2):
            if len(labels[i]) == 0:
                self.qtgui_time_sink_x_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_time_sink_x_0.set_line_label(i, labels[i])
            self.qtgui_time_sink_x_0.set_line_width(i, widths[i])
            self.qtgui_time_sink_x_0.set_line_color(i, colors[i])
            self.qtgui_time_sink_x_0.set_line_style(i, styles[i])
            self.qtgui_time_sink_x_0.set_line_marker(i, markers[i])
            self.qtgui_time_sink_x_0.set_line_alpha(i, alphas[i])

        self._qtgui_time_sink_x_0_win = sip.wrapinstance(self.qtgui_time_sink_x_0.pyqwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_time_sink_x_0_win, 0,0)
        self.qtgui_freq_sink_x_0_0 = qtgui.freq_sink_f(
        	256, #size
        	firdes.WIN_BLACKMAN_hARRIS, #wintype
        	0, #fc
        	samp_rate/decimate, #bw
        	"", #name
        	2 #number of inputs
        )
        self.qtgui_freq_sink_x_0_0.set_update_time(0.10)
        self.qtgui_freq_sink_x_0_0.set_y_axis(-50, +15)
        self.qtgui_freq_sink_x_0_0.set_y_label('Relative Gain', 'dB')
        self.qtgui_freq_sink_x_0_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, 0.0, 0, "")
        self.qtgui_freq_sink_x_0_0.enable_autoscale(False)
        self.qtgui_freq_sink_x_0_0.enable_grid(True)
        self.qtgui_freq_sink_x_0_0.set_fft_average(0.1)
        self.qtgui_freq_sink_x_0_0.enable_axis_labels(True)
        self.qtgui_freq_sink_x_0_0.enable_control_panel(False)

        if not True:
          self.qtgui_freq_sink_x_0_0.disable_legend()

        if "float" == "float" or "float" == "msg_float":
          self.qtgui_freq_sink_x_0_0.set_plot_pos_half(not False)

        labels = ['y(t)', 'dy(t)/dt', '', '', '',
                  '', '', '', '', '']
        widths = [2, 2, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
                  "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]
        for i in xrange(2):
            if len(labels[i]) == 0:
                self.qtgui_freq_sink_x_0_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_freq_sink_x_0_0.set_line_label(i, labels[i])
            self.qtgui_freq_sink_x_0_0.set_line_width(i, widths[i])
            self.qtgui_freq_sink_x_0_0.set_line_color(i, colors[i])
            self.qtgui_freq_sink_x_0_0.set_line_alpha(i, alphas[i])

        self._qtgui_freq_sink_x_0_0_win = sip.wrapinstance(self.qtgui_freq_sink_x_0_0.pyqwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_freq_sink_x_0_0_win, 2,0)
        self.qtgui_freq_sink_x_0 = qtgui.freq_sink_f(
        	4096, #size
        	firdes.WIN_HANN, #wintype
        	0, #fc
        	samp_rate, #bw
        	"", #name
        	2 #number of inputs
        )
        self.qtgui_freq_sink_x_0.set_update_time(0.10)
        self.qtgui_freq_sink_x_0.set_y_axis(-100, +10)
        self.qtgui_freq_sink_x_0.set_y_label('Relative Gain', 'dB')
        self.qtgui_freq_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, 0.0, 0, "")
        self.qtgui_freq_sink_x_0.enable_autoscale(False)
        self.qtgui_freq_sink_x_0.enable_grid(True)
        self.qtgui_freq_sink_x_0.set_fft_average(1.0)
        self.qtgui_freq_sink_x_0.enable_axis_labels(True)
        self.qtgui_freq_sink_x_0.enable_control_panel(False)

        if not True:
          self.qtgui_freq_sink_x_0.disable_legend()

        if "float" == "float" or "float" == "msg_float":
          self.qtgui_freq_sink_x_0.set_plot_pos_half(not False)

        labels = ['y(t)', 'dy(t)/dt', '', '', '',
                  '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
                  "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]
        for i in xrange(2):
            if len(labels[i]) == 0:
                self.qtgui_freq_sink_x_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_freq_sink_x_0.set_line_label(i, labels[i])
            self.qtgui_freq_sink_x_0.set_line_width(i, widths[i])
            self.qtgui_freq_sink_x_0.set_line_color(i, colors[i])
            self.qtgui_freq_sink_x_0.set_line_alpha(i, alphas[i])

        self._qtgui_freq_sink_x_0_win = sip.wrapinstance(self.qtgui_freq_sink_x_0.pyqwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_freq_sink_x_0_win, 1,0)
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_float*1, samp_rate,True)
        self.blocks_sub_xx_0_0 = blocks.sub_ff(1)
        self.blocks_sub_xx_0 = blocks.sub_ff(1)
        self.blocks_multiply_const_vxx_0_0 = blocks.multiply_const_vff((-1, ))
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_vff((1, ))
        self.blocks_keep_one_in_n_0_0 = blocks.keep_one_in_n(gr.sizeof_float*1, decimate)
        self.blocks_keep_one_in_n_0 = blocks.keep_one_in_n(gr.sizeof_float*1, decimate)
        self.blocks_add_xx_0 = blocks.add_vff(1)
        self.analog_sig_source_x_0_0 = analog.sig_source_f(samp_rate, analog.GR_SIN_WAVE, 60, 0.2, 0)
        self.analog_sig_source_x_0 = analog.sig_source_f(samp_rate, analog.GR_SAW_WAVE, 3, 10, 0)
        self.analog_noise_source_x_0 = analog.noise_source_f(analog.GR_UNIFORM, 1, 0)
        self.analog_const_source_x_0 = analog.sig_source_f(0, analog.GR_CONST_WAVE, 0, 0, 10)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_const_source_x_0, 0), (self.blocks_sub_xx_0_0, 0))
        self.connect((self.analog_noise_source_x_0, 0), (self.blocks_add_xx_0, 0))
        self.connect((self.analog_sig_source_x_0, 0), (self.blocks_sub_xx_0_0, 1))
        self.connect((self.analog_sig_source_x_0_0, 0), (self.blocks_add_xx_0, 1))
        self.connect((self.blocks_add_xx_0, 0), (self.blocks_sub_xx_0, 0))
        self.connect((self.blocks_add_xx_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.blocks_add_xx_0, 0), (self.single_pole_iir_filter_xx_0_1, 0))
        self.connect((self.blocks_keep_one_in_n_0, 0), (self.qtgui_freq_sink_x_0_0, 1))
        self.connect((self.blocks_keep_one_in_n_0_0, 0), (self.qtgui_freq_sink_x_0_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.qtgui_freq_sink_x_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.qtgui_time_sink_x_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0_0, 0), (self.blocks_keep_one_in_n_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0_0, 0), (self.qtgui_freq_sink_x_0, 1))
        self.connect((self.blocks_multiply_const_vxx_0_0, 0), (self.qtgui_time_sink_x_0, 1))
        self.connect((self.blocks_sub_xx_0, 0), (self.single_pole_iir_filter_xx_0, 0))
        self.connect((self.blocks_sub_xx_0_0, 0), (self.blocks_add_xx_0, 2))
        self.connect((self.blocks_throttle_0, 0), (self.blocks_keep_one_in_n_0_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.blocks_multiply_const_vxx_0, 0))
        self.connect((self.single_pole_iir_filter_xx_0, 0), (self.single_pole_iir_filter_xx_0_0, 0))
        self.connect((self.single_pole_iir_filter_xx_0_0, 0), (self.blocks_multiply_const_vxx_0_0, 0))
        self.connect((self.single_pole_iir_filter_xx_0_1, 0), (self.blocks_sub_xx_0, 1))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "dcct_post_filter")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_sqrt2(self):
        return self.sqrt2

    def set_sqrt2(self, sqrt2):
        self.sqrt2 = sqrt2
        self.set_freq_max_corr(self.freq_max/(2*(self.sqrt2-1)))

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.qtgui_time_sink_x_0.set_samp_rate(self.samp_rate)
        self.qtgui_freq_sink_x_0_0.set_frequency_range(0, self.samp_rate/self.decimate)
        self.qtgui_freq_sink_x_0.set_frequency_range(0, self.samp_rate)
        self.blocks_throttle_0.set_sample_rate(self.samp_rate)
        self.analog_sig_source_x_0_0.set_sampling_freq(self.samp_rate)
        self.analog_sig_source_x_0.set_sampling_freq(self.samp_rate)
        self.set_Ts(1.0/(self.samp_rate))

    def get_freq_max(self):
        return self.freq_max

    def set_freq_max(self, freq_max):
        self.freq_max = freq_max
        self.set_freq_max_corr(self.freq_max/(2*(self.sqrt2-1)))

    def get_PI(self):
        return self.PI

    def set_PI(self, PI):
        self.PI = PI
        self.set_TWO_PI(2*self.PI)

    def get_freq_min(self):
        return self.freq_min

    def set_freq_min(self, freq_min):
        self.freq_min = freq_min
        self.set_alpha_low(self.TWO_PI*self.Ts*self.freq_min/(self.TWO_PI*self.Ts*self.freq_min+1))

    def get_freq_max_corr(self):
        return self.freq_max_corr

    def set_freq_max_corr(self, freq_max_corr):
        self.freq_max_corr = freq_max_corr
        self.set_alpha(self.TWO_PI*self.Ts*self.freq_max_corr/(self.TWO_PI*self.Ts*self.freq_max_corr+1))

    def get_Ts(self):
        return self.Ts

    def set_Ts(self, Ts):
        self.Ts = Ts
        self.set_alpha_low(self.TWO_PI*self.Ts*self.freq_min/(self.TWO_PI*self.Ts*self.freq_min+1))
        self.set_alpha(self.TWO_PI*self.Ts*self.freq_max_corr/(self.TWO_PI*self.Ts*self.freq_max_corr+1))

    def get_TWO_PI(self):
        return self.TWO_PI

    def set_TWO_PI(self, TWO_PI):
        self.TWO_PI = TWO_PI
        self.set_alpha_low(self.TWO_PI*self.Ts*self.freq_min/(self.TWO_PI*self.Ts*self.freq_min+1))
        self.set_alpha(self.TWO_PI*self.Ts*self.freq_max_corr/(self.TWO_PI*self.Ts*self.freq_max_corr+1))

    def get_decimate(self):
        return self.decimate

    def set_decimate(self, decimate):
        self.decimate = decimate
        self.qtgui_freq_sink_x_0_0.set_frequency_range(0, self.samp_rate/self.decimate)
        self.blocks_keep_one_in_n_0_0.set_n(self.decimate)
        self.blocks_keep_one_in_n_0.set_n(self.decimate)

    def get_alpha_low(self):
        return self.alpha_low

    def set_alpha_low(self, alpha_low):
        self.alpha_low = alpha_low
        self.single_pole_iir_filter_xx_0_1.set_taps(self.alpha_low)

    def get_alpha(self):
        return self.alpha

    def set_alpha(self, alpha):
        self.alpha = alpha
        self.single_pole_iir_filter_xx_0_0.set_taps(self.alpha)
        self.single_pole_iir_filter_xx_0.set_taps(self.alpha)


def main(top_block_cls=dcct_post_filter, options=None):

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
