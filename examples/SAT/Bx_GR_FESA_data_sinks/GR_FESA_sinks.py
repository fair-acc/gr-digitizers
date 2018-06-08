#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Gr Fesa Sinks
# Generated: Fri Jun  8 17:36:29 2018
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
from gnuradio import gr
from gnuradio import qtgui
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import digitizers
import pmt
import sip
import sys
from gnuradio import qtgui


class GR_FESA_sinks(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Gr Fesa Sinks")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Gr Fesa Sinks")
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

        self.settings = Qt.QSettings("GNU Radio", "GR_FESA_sinks")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())


        ##################################################
        # Variables
        ##################################################
        self.wr_tag = wr_tag = gr.tag_utils.python_to_tag((4100, pmt.intern("wr_event"), pmt.make_tuple(pmt.string_to_symbol('foo'), pmt.from_uint64(2000000000), pmt.from_uint64(1900000000), pmt.from_bool(False), pmt.from_bool(True)), pmt.intern("src")))
        self.samp_rate = samp_rate = 100000
        self.nsamples__triggered = nsamples__triggered = 1000
        self.nsamples = nsamples = 200
        self.nbins_triggered = nbins_triggered = 1000
        self.nbins = nbins = 128
        self.edge_detect_1 = edge_detect_1 = gr.tag_utils.python_to_tag((4101, pmt.intern("edge_detect"), pmt.make_tuple(pmt.from_bool(True), pmt.from_uint64(2000000000), pmt.from_uint64(2000000000), pmt.from_uint64(0), pmt.from_uint64(0)), pmt.intern("src")))

        ##################################################
        # Blocks
        ##################################################
        self.qtgui_vector_sink_f_0_0 = qtgui.vector_sink_f(
            nbins_triggered,
            0,
            0.5/nbins,
            "norm. frequency [samp_rate]",
            "amplitude [dB]",
            'Triggerd Frequency-Domain',
            1 # Number of inputs
        )
        self.qtgui_vector_sink_f_0_0.set_update_time(0.1)
        self.qtgui_vector_sink_f_0_0.set_y_axis(-75, 5)
        self.qtgui_vector_sink_f_0_0.enable_autoscale(True)
        self.qtgui_vector_sink_f_0_0.enable_grid(True)
        self.qtgui_vector_sink_f_0_0.set_x_axis_units("samp_rate")
        self.qtgui_vector_sink_f_0_0.set_y_axis_units("[dB]")
        self.qtgui_vector_sink_f_0_0.set_ref_level(0)

        labels = ['Goertzel ', 'FFT', 'DFT', '', '',
                  '', '', '', '', '']
        widths = [2, 2, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
                  "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]
        for i in xrange(1):
            if len(labels[i]) == 0:
                self.qtgui_vector_sink_f_0_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_vector_sink_f_0_0.set_line_label(i, labels[i])
            self.qtgui_vector_sink_f_0_0.set_line_width(i, widths[i])
            self.qtgui_vector_sink_f_0_0.set_line_color(i, colors[i])
            self.qtgui_vector_sink_f_0_0.set_line_alpha(i, alphas[i])

        self._qtgui_vector_sink_f_0_0_win = sip.wrapinstance(self.qtgui_vector_sink_f_0_0.pyqwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_vector_sink_f_0_0_win, 1, 1, 1, 1)
        for r in range(1, 2):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(1, 2):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.qtgui_vector_sink_f_0 = qtgui.vector_sink_f(
            nbins,
            0,
            0.5/nbins,
            "norm. frequency [samp_rate]",
            "amplitude [dB]",
            'Continuous Frequency-Domain',
            1 # Number of inputs
        )
        self.qtgui_vector_sink_f_0.set_update_time(0.10)
        self.qtgui_vector_sink_f_0.set_y_axis(-75, 5)
        self.qtgui_vector_sink_f_0.enable_autoscale(False)
        self.qtgui_vector_sink_f_0.enable_grid(True)
        self.qtgui_vector_sink_f_0.set_x_axis_units("samp_rate")
        self.qtgui_vector_sink_f_0.set_y_axis_units("[dB]")
        self.qtgui_vector_sink_f_0.set_ref_level(0)

        labels = ['Goertzel ', 'FFT', 'DFT', '', '',
                  '', '', '', '', '']
        widths = [2, 2, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
                  "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]
        for i in xrange(1):
            if len(labels[i]) == 0:
                self.qtgui_vector_sink_f_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_vector_sink_f_0.set_line_label(i, labels[i])
            self.qtgui_vector_sink_f_0.set_line_width(i, widths[i])
            self.qtgui_vector_sink_f_0.set_line_color(i, colors[i])
            self.qtgui_vector_sink_f_0.set_line_alpha(i, alphas[i])

        self._qtgui_vector_sink_f_0_win = sip.wrapinstance(self.qtgui_vector_sink_f_0.pyqwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_vector_sink_f_0_win, 1, 0, 1, 1)
        for r in range(1, 2):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(0, 1):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.qtgui_time_sink_x_1_0 = qtgui.time_sink_f(
        	nsamples, #size
        	samp_rate, #samp_rate
        	'Continuous Time-Domain', #name
        	1 #number of inputs
        )
        self.qtgui_time_sink_x_1_0.set_update_time(0.10)
        self.qtgui_time_sink_x_1_0.set_y_axis(-0.1, 1.1)

        self.qtgui_time_sink_x_1_0.set_y_label('Amplitude', "")

        self.qtgui_time_sink_x_1_0.enable_tags(-1, True)
        self.qtgui_time_sink_x_1_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.qtgui_time_sink_x_1_0.enable_autoscale(False)
        self.qtgui_time_sink_x_1_0.enable_grid(True)
        self.qtgui_time_sink_x_1_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_1_0.enable_control_panel(False)
        self.qtgui_time_sink_x_1_0.enable_stem_plot(False)

        if not True:
          self.qtgui_time_sink_x_1_0.disable_legend()

        labels = ['', '', '', '', '',
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

        for i in xrange(1):
            if len(labels[i]) == 0:
                self.qtgui_time_sink_x_1_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_time_sink_x_1_0.set_line_label(i, labels[i])
            self.qtgui_time_sink_x_1_0.set_line_width(i, widths[i])
            self.qtgui_time_sink_x_1_0.set_line_color(i, colors[i])
            self.qtgui_time_sink_x_1_0.set_line_style(i, styles[i])
            self.qtgui_time_sink_x_1_0.set_line_marker(i, markers[i])
            self.qtgui_time_sink_x_1_0.set_line_alpha(i, alphas[i])

        self._qtgui_time_sink_x_1_0_win = sip.wrapinstance(self.qtgui_time_sink_x_1_0.pyqwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_time_sink_x_1_0_win, 0, 0, 1, 1)
        for r in range(0, 1):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(0, 1):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.qtgui_time_sink_x_1 = qtgui.time_sink_f(
        	nsamples__triggered, #size
        	samp_rate, #samp_rate
        	'Triggered Time-Domain', #name
        	1 #number of inputs
        )
        self.qtgui_time_sink_x_1.set_update_time(0.10)
        self.qtgui_time_sink_x_1.set_y_axis(-0.1, 1.1)

        self.qtgui_time_sink_x_1.set_y_label('Amplitude', "")

        self.qtgui_time_sink_x_1.enable_tags(-1, True)
        self.qtgui_time_sink_x_1.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.qtgui_time_sink_x_1.enable_autoscale(False)
        self.qtgui_time_sink_x_1.enable_grid(True)
        self.qtgui_time_sink_x_1.enable_axis_labels(True)
        self.qtgui_time_sink_x_1.enable_control_panel(False)
        self.qtgui_time_sink_x_1.enable_stem_plot(False)

        if not True:
          self.qtgui_time_sink_x_1.disable_legend()

        labels = ['', '', '', '', '',
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

        for i in xrange(1):
            if len(labels[i]) == 0:
                self.qtgui_time_sink_x_1.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_time_sink_x_1.set_line_label(i, labels[i])
            self.qtgui_time_sink_x_1.set_line_width(i, widths[i])
            self.qtgui_time_sink_x_1.set_line_color(i, colors[i])
            self.qtgui_time_sink_x_1.set_line_style(i, styles[i])
            self.qtgui_time_sink_x_1.set_line_marker(i, markers[i])
            self.qtgui_time_sink_x_1.set_line_alpha(i, alphas[i])

        self._qtgui_time_sink_x_1_win = sip.wrapinstance(self.qtgui_time_sink_x_1.pyqwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_time_sink_x_1_win, 0, 1, 1, 1)
        for r in range(0, 1):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(1, 2):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.qtgui_time_raster_sink_x_0 = qtgui.time_raster_sink_f(
        	samp_rate,
        	20,
        	nbins_triggered,
        	([]),
        	([]),
        	"",
        	1,
        	)

        self.qtgui_time_raster_sink_x_0.set_update_time(0.1)
        self.qtgui_time_raster_sink_x_0.set_intensity_range(-60, -20)
        self.qtgui_time_raster_sink_x_0.enable_grid(True)
        self.qtgui_time_raster_sink_x_0.enable_axis_labels(True)

        labels = ['', '', '', '', '',
                  '', '', '', '', '']
        colors = [0, 0, 0, 0, 0,
                  0, 0, 0, 0, 0]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]
        for i in xrange(1):
            if len(labels[i]) == 0:
                self.qtgui_time_raster_sink_x_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_time_raster_sink_x_0.set_line_label(i, labels[i])
            self.qtgui_time_raster_sink_x_0.set_color_map(i, colors[i])
            self.qtgui_time_raster_sink_x_0.set_line_alpha(i, alphas[i])

        self._qtgui_time_raster_sink_x_0_win = sip.wrapinstance(self.qtgui_time_raster_sink_x_0.pyqwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_time_raster_sink_x_0_win, 1, 3, 1, 1)
        for r in range(1, 2):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(3, 4):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.digitizers_time_domain_sink_0_0 = digitizers.time_domain_sink('test_signal', 'a.u.', samp_rate, 10000, 16, 0)
        self.digitizers_time_domain_sink_0 = digitizers.time_domain_sink('test_signal', 'a.u.', samp_rate, nsamples__triggered, 16, 0)
        self.digitizers_stft_algorithms_0_0 = digitizers.stft_algorithms(samp_rate, 0.01, nbins_triggered, firdes.WIN_HANN, 0, 0.0, samp_rate/2, 512)
        self.digitizers_stft_algorithms_0 = digitizers.stft_algorithms(samp_rate, 0.02, nbins, firdes.WIN_HANN, 0, 0.0, samp_rate/2, 512)
        self.digitizers_post_mortem_sink_0 = digitizers.post_mortem_sink('Signal_name', '[a.u.]', samp_rate, 1000000)

        self.digitizers_freq_sink_f_0_0 = digitizers.freq_sink_f('My Signal', samp_rate, nbins_triggered, 10, 1, 0)
        self.digitizers_freq_sink_f_0 = digitizers.freq_sink_f('My Signal', samp_rate, nbins, 10, 1, 1)
        self.digitizers_demux_ff_0_0 = digitizers.demux_ff(samp_rate, nbins_triggered*2+100000, nbins_triggered*2, 0)
        self.digitizers_demux_ff_0 = digitizers.demux_ff(samp_rate, nsamples__triggered+100000, nsamples__triggered, 0)
        self.blocks_vector_to_stream_0 = blocks.vector_to_stream(gr.sizeof_float*1, nbins_triggered)
        self.blocks_vector_source_x_0 = blocks.vector_source_f(([0] *8192) , True, 1, [wr_tag, edge_detect_1])
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_float*1, samp_rate,True)
        self.blocks_null_source_0 = blocks.null_source(gr.sizeof_float*1)
        self.blocks_null_sink_0_0 = blocks.null_sink(gr.sizeof_float*nbins_triggered)
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_float*nbins)
        self.blocks_nlog10_ff_0_0 = blocks.nlog10_ff(20, nbins_triggered, 0)
        self.blocks_nlog10_ff_0 = blocks.nlog10_ff(20, nbins, 0)
        self.blocks_multiply_xx_0 = blocks.multiply_vff(1)
        self.blocks_multiply_const_xx_0 = blocks.multiply_const_ff(1)
        self.blocks_multiply_const_vxx_0_0_0 = blocks.multiply_const_vff(([1.0/nbins_triggered] * nbins_triggered))
        self.blocks_multiply_const_vxx_0_0 = blocks.multiply_const_vff(([1.0/nbins] * nbins))
        self.blocks_add_xx_0 = blocks.add_vff(1)
        self.analog_sig_source_x_0_0 = analog.sig_source_f(samp_rate, analog.GR_TRI_WAVE, 4400, 1, 0)
        self.analog_sig_source_x_0 = analog.sig_source_f(samp_rate, analog.GR_SQR_WAVE, 2200, 1, 0)
        self.analog_fastnoise_source_x_0 = analog.fastnoise_source_f(analog.GR_GAUSSIAN, 0.05, 0, 10000)



        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_fastnoise_source_x_0, 0), (self.blocks_add_xx_0, 0))
        self.connect((self.analog_sig_source_x_0, 0), (self.blocks_multiply_xx_0, 0))
        self.connect((self.analog_sig_source_x_0_0, 0), (self.blocks_multiply_xx_0, 1))
        self.connect((self.blocks_add_xx_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0_0, 0), (self.blocks_nlog10_ff_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0_0_0, 0), (self.blocks_nlog10_ff_0_0, 0))
        self.connect((self.blocks_multiply_const_xx_0, 0), (self.digitizers_demux_ff_0_0, 0))
        self.connect((self.blocks_multiply_const_xx_0, 0), (self.digitizers_stft_algorithms_0, 0))
        self.connect((self.blocks_multiply_xx_0, 0), (self.blocks_add_xx_0, 1))
        self.connect((self.blocks_nlog10_ff_0, 0), (self.qtgui_vector_sink_f_0, 0))
        self.connect((self.blocks_nlog10_ff_0_0, 0), (self.blocks_vector_to_stream_0, 0))
        self.connect((self.blocks_nlog10_ff_0_0, 0), (self.qtgui_vector_sink_f_0_0, 0))
        self.connect((self.blocks_null_source_0, 0), (self.digitizers_post_mortem_sink_0, 1))
        self.connect((self.blocks_throttle_0, 0), (self.digitizers_post_mortem_sink_0, 0))
        self.connect((self.blocks_vector_source_x_0, 0), (self.blocks_add_xx_0, 2))
        self.connect((self.blocks_vector_to_stream_0, 0), (self.qtgui_time_raster_sink_x_0, 0))
        self.connect((self.digitizers_demux_ff_0, 1), (self.digitizers_time_domain_sink_0, 1))
        self.connect((self.digitizers_demux_ff_0, 0), (self.digitizers_time_domain_sink_0, 0))
        self.connect((self.digitizers_demux_ff_0, 0), (self.qtgui_time_sink_x_1, 0))
        self.connect((self.digitizers_demux_ff_0_0, 0), (self.digitizers_stft_algorithms_0_0, 0))
        self.connect((self.digitizers_post_mortem_sink_0, 0), (self.blocks_multiply_const_xx_0, 0))
        self.connect((self.digitizers_post_mortem_sink_0, 1), (self.digitizers_demux_ff_0, 1))
        self.connect((self.digitizers_post_mortem_sink_0, 0), (self.digitizers_demux_ff_0, 0))
        self.connect((self.digitizers_post_mortem_sink_0, 1), (self.digitizers_time_domain_sink_0_0, 1))
        self.connect((self.digitizers_post_mortem_sink_0, 0), (self.digitizers_time_domain_sink_0_0, 0))
        self.connect((self.digitizers_post_mortem_sink_0, 0), (self.qtgui_time_sink_x_1_0, 0))
        self.connect((self.digitizers_stft_algorithms_0, 0), (self.blocks_multiply_const_vxx_0_0, 0))
        self.connect((self.digitizers_stft_algorithms_0, 2), (self.blocks_null_sink_0, 0))
        self.connect((self.digitizers_stft_algorithms_0, 1), (self.blocks_null_sink_0, 1))
        self.connect((self.digitizers_stft_algorithms_0, 0), (self.digitizers_freq_sink_f_0, 0))
        self.connect((self.digitizers_stft_algorithms_0, 2), (self.digitizers_freq_sink_f_0, 2))
        self.connect((self.digitizers_stft_algorithms_0, 1), (self.digitizers_freq_sink_f_0, 1))
        self.connect((self.digitizers_stft_algorithms_0_0, 0), (self.blocks_multiply_const_vxx_0_0_0, 0))
        self.connect((self.digitizers_stft_algorithms_0_0, 2), (self.blocks_null_sink_0_0, 0))
        self.connect((self.digitizers_stft_algorithms_0_0, 1), (self.blocks_null_sink_0_0, 1))
        self.connect((self.digitizers_stft_algorithms_0_0, 0), (self.digitizers_freq_sink_f_0_0, 0))
        self.connect((self.digitizers_stft_algorithms_0_0, 2), (self.digitizers_freq_sink_f_0_0, 2))
        self.connect((self.digitizers_stft_algorithms_0_0, 1), (self.digitizers_freq_sink_f_0_0, 1))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "GR_FESA_sinks")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_wr_tag(self):
        return self.wr_tag

    def set_wr_tag(self, wr_tag):
        self.wr_tag = wr_tag
        self.blocks_vector_source_x_0.set_data(([0] *8192) , [self.wr_tag, self.edge_detect_1])

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.qtgui_time_sink_x_1_0.set_samp_rate(self.samp_rate)
        self.qtgui_time_sink_x_1.set_samp_rate(self.samp_rate)
        self.digitizers_stft_algorithms_0_0.set_samp_rate(self.samp_rate)
        self.digitizers_stft_algorithms_0_0.set_freqs(0.0, self.samp_rate/2)
        self.digitizers_stft_algorithms_0.set_samp_rate(self.samp_rate)
        self.digitizers_stft_algorithms_0.set_freqs(0.0, self.samp_rate/2)
        self.blocks_throttle_0.set_sample_rate(self.samp_rate)
        self.analog_sig_source_x_0_0.set_sampling_freq(self.samp_rate)
        self.analog_sig_source_x_0.set_sampling_freq(self.samp_rate)

    def get_nsamples__triggered(self):
        return self.nsamples__triggered

    def set_nsamples__triggered(self, nsamples__triggered):
        self.nsamples__triggered = nsamples__triggered

    def get_nsamples(self):
        return self.nsamples

    def set_nsamples(self, nsamples):
        self.nsamples = nsamples

    def get_nbins_triggered(self):
        return self.nbins_triggered

    def set_nbins_triggered(self, nbins_triggered):
        self.nbins_triggered = nbins_triggered
        self.qtgui_time_raster_sink_x_0.set_num_cols(self.nbins_triggered)
        self.digitizers_stft_algorithms_0_0.set_window_size(self.nbins_triggered)
        self.blocks_multiply_const_vxx_0_0_0.set_k(([1.0/self.nbins_triggered] * self.nbins_triggered))

    def get_nbins(self):
        return self.nbins

    def set_nbins(self, nbins):
        self.nbins = nbins
        self.qtgui_vector_sink_f_0_0.set_x_axis(0, 0.5/self.nbins)
        self.qtgui_vector_sink_f_0.set_x_axis(0, 0.5/self.nbins)
        self.digitizers_stft_algorithms_0.set_window_size(self.nbins)
        self.blocks_multiply_const_vxx_0_0.set_k(([1.0/self.nbins] * self.nbins))

    def get_edge_detect_1(self):
        return self.edge_detect_1

    def set_edge_detect_1(self, edge_detect_1):
        self.edge_detect_1 = edge_detect_1
        self.blocks_vector_source_x_0.set_data(([0] *8192) , [self.wr_tag, self.edge_detect_1])


def main(top_block_cls=GR_FESA_sinks, options=None):

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
