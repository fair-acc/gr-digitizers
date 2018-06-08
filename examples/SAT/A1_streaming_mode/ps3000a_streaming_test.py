#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Ps3000A Streaming Test
# Generated: Tue May  8 09:03:29 2018
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
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import qtgui
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import digitizers
import sip
from gnuradio import qtgui


class ps3000a_streaming_test(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Ps3000A Streaming Test")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Ps3000A Streaming Test")
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

        self.settings = Qt.QSettings("GNU Radio", "ps3000a_streaming_test")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())


        ##################################################
        # Variables
        ##################################################
        self.samples = samples = 8000
        self.samp_rate = samp_rate = 60000000
        self.pre_samples = pre_samples = 2000

        ##################################################
        # Blocks
        ##################################################
        self.qtgui_time_sink_x_0_0_0 = qtgui.time_sink_f(
        	40000, #size
        	samp_rate, #samp_rate
        	"Digital", #name
        	4 #number of inputs
        )
        self.qtgui_time_sink_x_0_0_0.set_update_time(0.5)
        self.qtgui_time_sink_x_0_0_0.set_y_axis(-1, 5)

        self.qtgui_time_sink_x_0_0_0.set_y_label('Decimal nr.', "")

        self.qtgui_time_sink_x_0_0_0.enable_tags(-1, False)
        self.qtgui_time_sink_x_0_0_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.qtgui_time_sink_x_0_0_0.enable_autoscale(True)
        self.qtgui_time_sink_x_0_0_0.enable_grid(False)
        self.qtgui_time_sink_x_0_0_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0_0_0.enable_control_panel(False)
        self.qtgui_time_sink_x_0_0_0.enable_stem_plot(False)

        if not True:
          self.qtgui_time_sink_x_0_0_0.disable_legend()

        labels = ['D0', 'D1', 'D7', 'D8', '',
                  '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "magenta", "cyan",
                  "magenta", "yellow", "dark red", "dark green", "blue"]
        styles = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
                   -1, -1, -1, -1, -1]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]

        for i in xrange(4):
            if len(labels[i]) == 0:
                self.qtgui_time_sink_x_0_0_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_time_sink_x_0_0_0.set_line_label(i, labels[i])
            self.qtgui_time_sink_x_0_0_0.set_line_width(i, widths[i])
            self.qtgui_time_sink_x_0_0_0.set_line_color(i, colors[i])
            self.qtgui_time_sink_x_0_0_0.set_line_style(i, styles[i])
            self.qtgui_time_sink_x_0_0_0.set_line_marker(i, markers[i])
            self.qtgui_time_sink_x_0_0_0.set_line_alpha(i, alphas[i])

        self._qtgui_time_sink_x_0_0_0_win = sip.wrapinstance(self.qtgui_time_sink_x_0_0_0.pyqwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_time_sink_x_0_0_0_win, 0, 1, 1, 1)
        for r in range(0, 1):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(1, 2):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.qtgui_time_sink_x_0 = qtgui.time_sink_f(
        	4000, #size
        	samp_rate, #samp_rate
        	"", #name
        	10 #number of inputs
        )
        self.qtgui_time_sink_x_0.set_update_time(0.5)
        self.qtgui_time_sink_x_0.set_y_axis(-5, 5)

        self.qtgui_time_sink_x_0.set_y_label('Amplitude', "")

        self.qtgui_time_sink_x_0.enable_tags(-1, False)
        self.qtgui_time_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.1, 0, 0, "")
        self.qtgui_time_sink_x_0.enable_autoscale(True)
        self.qtgui_time_sink_x_0.enable_grid(True)
        self.qtgui_time_sink_x_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0.enable_control_panel(False)
        self.qtgui_time_sink_x_0.enable_stem_plot(False)

        if not True:
          self.qtgui_time_sink_x_0.disable_legend()

        labels = ['-', 'channel A', '-', '-', 'channel B',
                  '-', '-', 'channel C', '-', 'channel D']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "blue", "blue", "red", "red",
                  "red", "green", "green", "green", "magenta"]
        styles = [2, 1, 2, 2, 1,
                  2, 2, 1, 2, 1]
        markers = [-1, -1, -1, -1, -1,
                   -1, -1, 2, -1, -1]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]

        for i in xrange(10):
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
        self.top_grid_layout.addWidget(self._qtgui_time_sink_x_0_win, 0, 0, 1, 1)
        for r in range(0, 1):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(0, 1):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.qtgui_freq_sink_x_0 = qtgui.freq_sink_f(
        	4096, #size
        	firdes.WIN_HANN, #wintype
        	0, #fc
        	samp_rate, #bw
        	"", #name
        	4 #number of inputs
        )
        self.qtgui_freq_sink_x_0.set_update_time(0.5)
        self.qtgui_freq_sink_x_0.set_y_axis(-140, 10)
        self.qtgui_freq_sink_x_0.set_y_label('Relative Gain', 'dB')
        self.qtgui_freq_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, 0.0, 0, "")
        self.qtgui_freq_sink_x_0.enable_autoscale(False)
        self.qtgui_freq_sink_x_0.enable_grid(False)
        self.qtgui_freq_sink_x_0.set_fft_average(1.0)
        self.qtgui_freq_sink_x_0.enable_axis_labels(True)
        self.qtgui_freq_sink_x_0.enable_control_panel(False)

        if not True:
          self.qtgui_freq_sink_x_0.disable_legend()

        if "float" == "float" or "float" == "msg_float":
          self.qtgui_freq_sink_x_0.set_plot_pos_half(not False)

        labels = ['Channel A', 'Channel B', 'Channel C', 'Channel D', '',
                  '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
                  "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]
        for i in xrange(4):
            if len(labels[i]) == 0:
                self.qtgui_freq_sink_x_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_freq_sink_x_0.set_line_label(i, labels[i])
            self.qtgui_freq_sink_x_0.set_line_width(i, widths[i])
            self.qtgui_freq_sink_x_0.set_line_color(i, colors[i])
            self.qtgui_freq_sink_x_0.set_line_alpha(i, alphas[i])

        self._qtgui_freq_sink_x_0_win = sip.wrapinstance(self.qtgui_freq_sink_x_0.pyqwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_freq_sink_x_0_win, 1, 0, 1, 2)
        for r in range(1, 2):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(0, 2):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.digitizers_picoscope_3000a_0 = digitizers.picoscope_3000a('FP635/012', True)
        self.digitizers_picoscope_3000a_0.set_trigger_once(False)
        self.digitizers_picoscope_3000a_0.set_samp_rate(samp_rate)
        self.digitizers_picoscope_3000a_0.set_downsampling(0, 10)
        self.digitizers_picoscope_3000a_0.set_aichan('A', True, 2, True, 0)
        self.digitizers_picoscope_3000a_0.set_aichan('B', False, 5, True, 0.0)
        self.digitizers_picoscope_3000a_0.set_aichan('C', False, 5, True, 0.0)
        self.digitizers_picoscope_3000a_0.set_aichan('D', False, 10, True, 0.0)

        self.digitizers_picoscope_3000a_0.set_diport('port0', True, 1)
        self.digitizers_picoscope_3000a_0.set_diport('port1', True, 0.5)

        if 'Digital' != 'None':
            if 'Digital' == 'Digital':
                self.digitizers_picoscope_3000a_0.set_di_trigger(0, 0)
            else:
                self.digitizers_picoscope_3000a_0.set_aichan_trigger('Digital', 0, 1.0)

        if 'Streaming' == 'Streaming':
            self.digitizers_picoscope_3000a_0.set_buffer_size(10000)
            self.digitizers_picoscope_3000a_0.set_nr_buffers(100)
            self.digitizers_picoscope_3000a_0.set_driver_buffer_size(100000)
            self.digitizers_picoscope_3000a_0.set_streaming(0.000)
        else:
            self.digitizers_picoscope_3000a_0.set_samples(samples, pre_samples)
            self.digitizers_picoscope_3000a_0.set_rapid_block(5)

        self.digitizers_block_demux_0_1 = digitizers.block_demux(0)
        self.digitizers_block_demux_0_0_0 = digitizers.block_demux(1)
        self.digitizers_block_demux_0_0 = digitizers.block_demux(1)
        self.digitizers_block_demux_0 = digitizers.block_demux(0)
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_float*1, samp_rate,True)
        self.blocks_null_sink_0_0 = blocks.null_sink(gr.sizeof_float*1)
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_float*1)
        self.blocks_add_const_vxx_0_0_0_0 = blocks.add_const_vff((0.3, ))
        self.blocks_add_const_vxx_0_0_0 = blocks.add_const_vff((0.2, ))
        self.blocks_add_const_vxx_0_0 = blocks.add_const_vff((0.1, ))
        self.blocks_add_const_vxx_0 = blocks.add_const_vff((0, ))
        self.block_error_band_0_0_0 = block_error_band()
        self.block_error_band_0_0 = block_error_band()
        self.block_error_band_0 = block_error_band()



        ##################################################
        # Connections
        ##################################################
        self.connect((self.block_error_band_0, 0), (self.qtgui_time_sink_x_0, 0))
        self.connect((self.block_error_band_0, 2), (self.qtgui_time_sink_x_0, 2))
        self.connect((self.block_error_band_0, 1), (self.qtgui_time_sink_x_0, 1))
        self.connect((self.block_error_band_0_0, 0), (self.qtgui_time_sink_x_0, 3))
        self.connect((self.block_error_band_0_0, 2), (self.qtgui_time_sink_x_0, 5))
        self.connect((self.block_error_band_0_0, 1), (self.qtgui_time_sink_x_0, 4))
        self.connect((self.block_error_band_0_0_0, 0), (self.qtgui_time_sink_x_0, 6))
        self.connect((self.block_error_band_0_0_0, 2), (self.qtgui_time_sink_x_0, 8))
        self.connect((self.block_error_band_0_0_0, 1), (self.qtgui_time_sink_x_0, 7))
        self.connect((self.blocks_add_const_vxx_0, 0), (self.qtgui_time_sink_x_0_0_0, 0))
        self.connect((self.blocks_add_const_vxx_0_0, 0), (self.qtgui_time_sink_x_0_0_0, 1))
        self.connect((self.blocks_add_const_vxx_0_0_0, 0), (self.qtgui_time_sink_x_0_0_0, 2))
        self.connect((self.blocks_add_const_vxx_0_0_0_0, 0), (self.qtgui_time_sink_x_0_0_0, 3))
        self.connect((self.blocks_throttle_0, 0), (self.qtgui_freq_sink_x_0, 0))
        self.connect((self.digitizers_block_demux_0, 0), (self.blocks_add_const_vxx_0, 0))
        self.connect((self.digitizers_block_demux_0_0, 0), (self.blocks_add_const_vxx_0_0, 0))
        self.connect((self.digitizers_block_demux_0_0_0, 0), (self.blocks_add_const_vxx_0_0_0_0, 0))
        self.connect((self.digitizers_block_demux_0_1, 0), (self.blocks_add_const_vxx_0_0_0, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 0), (self.block_error_band_0, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 1), (self.block_error_band_0, 1))
        self.connect((self.digitizers_picoscope_3000a_0, 2), (self.block_error_band_0_0, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 3), (self.block_error_band_0_0, 1))
        self.connect((self.digitizers_picoscope_3000a_0, 4), (self.block_error_band_0_0_0, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 5), (self.block_error_band_0_0_0, 1))
        self.connect((self.digitizers_picoscope_3000a_0, 1), (self.blocks_null_sink_0, 3))
        self.connect((self.digitizers_picoscope_3000a_0, 3), (self.blocks_null_sink_0, 2))
        self.connect((self.digitizers_picoscope_3000a_0, 5), (self.blocks_null_sink_0, 1))
        self.connect((self.digitizers_picoscope_3000a_0, 7), (self.blocks_null_sink_0, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 0), (self.blocks_null_sink_0_0, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 2), (self.blocks_null_sink_0_0, 1))
        self.connect((self.digitizers_picoscope_3000a_0, 4), (self.blocks_null_sink_0_0, 2))
        self.connect((self.digitizers_picoscope_3000a_0, 6), (self.blocks_null_sink_0_0, 3))
        self.connect((self.digitizers_picoscope_3000a_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 8), (self.digitizers_block_demux_0, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 8), (self.digitizers_block_demux_0_0, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 9), (self.digitizers_block_demux_0_0_0, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 9), (self.digitizers_block_demux_0_1, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 2), (self.qtgui_freq_sink_x_0, 1))
        self.connect((self.digitizers_picoscope_3000a_0, 4), (self.qtgui_freq_sink_x_0, 2))
        self.connect((self.digitizers_picoscope_3000a_0, 6), (self.qtgui_freq_sink_x_0, 3))
        self.connect((self.digitizers_picoscope_3000a_0, 6), (self.qtgui_time_sink_x_0, 9))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "ps3000a_streaming_test")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_samples(self):
        return self.samples

    def set_samples(self, samples):
        self.samples = samples

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.qtgui_time_sink_x_0_0_0.set_samp_rate(self.samp_rate)
        self.qtgui_time_sink_x_0.set_samp_rate(self.samp_rate)
        self.qtgui_freq_sink_x_0.set_frequency_range(0, self.samp_rate)
        self.blocks_throttle_0.set_sample_rate(self.samp_rate)

    def get_pre_samples(self):
        return self.pre_samples

    def set_pre_samples(self, pre_samples):
        self.pre_samples = pre_samples


def main(top_block_cls=ps3000a_streaming_test, options=None):

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
