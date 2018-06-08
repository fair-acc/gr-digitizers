#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Digitizer Streaming Mode with Sink Tests
# Author: rstein
# Description: Basic Digitizer Streaming Mode Test with FESA Sinks
# Generated: Fri Jun  8 17:11:58 2018
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
import numpy
import sip
from gnuradio import qtgui


class basic_interface_test(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Digitizer Streaming Mode with Sink Tests")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Digitizer Streaming Mode with Sink Tests")
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

        self.settings = Qt.QSettings("GNU Radio", "basic_interface_test")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())


        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 6000000
        self.ch_b_f = ch_b_f = 50000
        self.ch_a_f = ch_a_f = 30000

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

        labels = ['D0', 'D1', 'D2', 'D3', '',
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
        	6 #number of inputs
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

        for i in xrange(6):
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
        	2 #number of inputs
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
        for i in xrange(2):
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
        self.digitizers_simulation_source_0 = digitizers.simulation_source()
        self.digitizers_simulation_source_0.set_trigger_once(True)
        self.digitizers_simulation_source_0.set_samp_rate(samp_rate)
        self.digitizers_simulation_source_0.set_auto_arm(True)
        self.digitizers_simulation_source_0.set_aichan('A', True, 20.0, True, 0)
        self.digitizers_simulation_source_0.set_aichan('B', True, 20.0, True, 0)
        self.digitizers_simulation_source_0.set_diport('port0', True, 0.7)

        if 'Digital' != 'None':
            if 'Digital' == 'Digital':
                self.digitizers_simulation_source_0.set_di_trigger(0, 0)
            else:
                self.digitizers_simulation_source_0.set_aichan_trigger('Digital', 0, 0.9)

        if 'Streaming' == 'Streaming':
            self.digitizers_simulation_source_0.set_buffer_size(8192)
            self.digitizers_simulation_source_0.set_nr_buffers(100)
            self.digitizers_simulation_source_0.set_driver_buffer_size(8192)
            self.digitizers_simulation_source_0.set_streaming(0.0005)
        else:
            self.digitizers_simulation_source_0.set_samples(4192, 4000)
            self.digitizers_simulation_source_0.set_rapid_block(1)

        self.digitizers_simulation_source_0.set_data((numpy.sin(2 * numpy.pi * numpy.arange(8192) * ch_a_f/samp_rate).astype(numpy.float32).tolist()), (numpy.cos(2 * numpy.pi * numpy.arange(8192) * ch_b_f/samp_rate).astype(numpy.float32).tolist()), (([0] *4000) + ([1] *192) + ([0] *4000)))

        self.digitizers_block_demux_0_1 = digitizers.block_demux(2)
        self.digitizers_block_demux_0_0_0 = digitizers.block_demux(3)
        self.digitizers_block_demux_0_0 = digitizers.block_demux(1)
        self.digitizers_block_demux_0 = digitizers.block_demux(0)
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_char*1, samp_rate,True)
        self.blocks_tag_debug_0 = blocks.tag_debug(gr.sizeof_char*1, 'D0 trigger', "trigger"); self.blocks_tag_debug_0.set_display(True)
        self.blocks_null_sink_0_0 = blocks.null_sink(gr.sizeof_float*1)
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_float*1)
        self.blocks_add_const_vxx_0_0_0_0 = blocks.add_const_vff((0.3, ))
        self.blocks_add_const_vxx_0_0_0 = blocks.add_const_vff((0.2, ))
        self.blocks_add_const_vxx_0_0 = blocks.add_const_vff((0.1, ))
        self.blocks_add_const_vxx_0 = blocks.add_const_vff((0, ))
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
        self.connect((self.blocks_add_const_vxx_0, 0), (self.qtgui_time_sink_x_0_0_0, 0))
        self.connect((self.blocks_add_const_vxx_0_0, 0), (self.qtgui_time_sink_x_0_0_0, 1))
        self.connect((self.blocks_add_const_vxx_0_0_0, 0), (self.qtgui_time_sink_x_0_0_0, 2))
        self.connect((self.blocks_add_const_vxx_0_0_0_0, 0), (self.qtgui_time_sink_x_0_0_0, 3))
        self.connect((self.blocks_throttle_0, 0), (self.blocks_tag_debug_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.digitizers_block_demux_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.digitizers_block_demux_0_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.digitizers_block_demux_0_0_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.digitizers_block_demux_0_1, 0))
        self.connect((self.digitizers_block_demux_0, 0), (self.blocks_add_const_vxx_0, 0))
        self.connect((self.digitizers_block_demux_0_0, 0), (self.blocks_add_const_vxx_0_0, 0))
        self.connect((self.digitizers_block_demux_0_0_0, 0), (self.blocks_add_const_vxx_0_0_0_0, 0))
        self.connect((self.digitizers_block_demux_0_1, 0), (self.blocks_add_const_vxx_0_0_0, 0))
        self.connect((self.digitizers_simulation_source_0, 0), (self.block_error_band_0, 0))
        self.connect((self.digitizers_simulation_source_0, 1), (self.block_error_band_0, 1))
        self.connect((self.digitizers_simulation_source_0, 2), (self.block_error_band_0_0, 0))
        self.connect((self.digitizers_simulation_source_0, 3), (self.block_error_band_0_0, 1))
        self.connect((self.digitizers_simulation_source_0, 2), (self.blocks_null_sink_0, 1))
        self.connect((self.digitizers_simulation_source_0, 3), (self.blocks_null_sink_0, 0))
        self.connect((self.digitizers_simulation_source_0, 0), (self.blocks_null_sink_0_0, 0))
        self.connect((self.digitizers_simulation_source_0, 1), (self.blocks_null_sink_0_0, 1))
        self.connect((self.digitizers_simulation_source_0, 4), (self.blocks_throttle_0, 0))
        self.connect((self.digitizers_simulation_source_0, 0), (self.qtgui_freq_sink_x_0, 0))
        self.connect((self.digitizers_simulation_source_0, 2), (self.qtgui_freq_sink_x_0, 1))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "basic_interface_test")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.qtgui_time_sink_x_0_0_0.set_samp_rate(self.samp_rate)
        self.qtgui_time_sink_x_0.set_samp_rate(self.samp_rate)
        self.qtgui_freq_sink_x_0.set_frequency_range(0, self.samp_rate)
        self.blocks_throttle_0.set_sample_rate(self.samp_rate)

    def get_ch_b_f(self):
        return self.ch_b_f

    def set_ch_b_f(self, ch_b_f):
        self.ch_b_f = ch_b_f

    def get_ch_a_f(self):
        return self.ch_a_f

    def set_ch_a_f(self, ch_a_f):
        self.ch_a_f = ch_a_f


def main(top_block_cls=basic_interface_test, options=None):

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
