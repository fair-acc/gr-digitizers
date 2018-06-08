#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: B2 Time Realignment Test
# Generated: Tue May  8 15:09:45 2018
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


class b2_time_realignment_test(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "B2 Time Realignment Test")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("B2 Time Realignment Test")
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

        self.settings = Qt.QSettings("GNU Radio", "b2_time_realignment_test")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())


        ##################################################
        # Variables
        ##################################################
        self.samples = samples = 9000
        self.samp_rate = samp_rate = 1000000
        self.pre_samples = pre_samples = 1000
        self.history = history = 1000000
        self.decimate = decimate = 1

        ##################################################
        # Blocks
        ##################################################
        self.qtgui_time_sink_x_0_0 = qtgui.time_sink_f(
        	(pre_samples+samples)/decimate, #size
        	samp_rate/decimate, #samp_rate
        	"Tags", #name
        	3 #number of inputs
        )
        self.qtgui_time_sink_x_0_0.set_update_time(0.10)
        self.qtgui_time_sink_x_0_0.set_y_axis(-1, 1)

        self.qtgui_time_sink_x_0_0.set_y_label('V', "")

        self.qtgui_time_sink_x_0_0.enable_tags(-1, True)
        self.qtgui_time_sink_x_0_0.set_trigger_mode(qtgui.TRIG_MODE_TAG, qtgui.TRIG_SLOPE_POS, 0.0, 0.005, 0, "acq_info")
        self.qtgui_time_sink_x_0_0.enable_autoscale(False)
        self.qtgui_time_sink_x_0_0.enable_grid(False)
        self.qtgui_time_sink_x_0_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0_0.enable_control_panel(False)
        self.qtgui_time_sink_x_0_0.enable_stem_plot(False)

        if not True:
          self.qtgui_time_sink_x_0_0.disable_legend()

        labels = ['baseline 500 kHz LP', 'baseline 100 kHz LP', 'baseline 10 kHz LP', '-', '',
                  '-', '-', '', '-', '']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "red", "red",
                  "red", "green", "green", "green", "blue"]
        styles = [0, 0, 0, 2, 1,
                  2, 2, 1, 2, 1]
        markers = [-1, -1, -1, -1, -1,
                   -1, -1, -1, -1, -1]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]

        for i in xrange(3):
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
        self.top_grid_layout.addWidget(self._qtgui_time_sink_x_0_0_win, 1, 0, 1, 1)
        for r in range(1, 2):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(0, 1):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.qtgui_time_sink_x_0 = qtgui.time_sink_f(
        	(pre_samples+samples)/decimate, #size
        	samp_rate/decimate, #samp_rate
        	"Signals", #name
        	9 #number of inputs
        )
        self.qtgui_time_sink_x_0.set_update_time(0.10)
        self.qtgui_time_sink_x_0.set_y_axis(-1, 2)

        self.qtgui_time_sink_x_0.set_y_label('V', "")

        self.qtgui_time_sink_x_0.enable_tags(-1, True)
        self.qtgui_time_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "acq_info")
        self.qtgui_time_sink_x_0.enable_autoscale(False)
        self.qtgui_time_sink_x_0.enable_grid(False)
        self.qtgui_time_sink_x_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0.enable_control_panel(False)
        self.qtgui_time_sink_x_0.enable_stem_plot(False)

        if not True:
          self.qtgui_time_sink_x_0.disable_legend()

        labels = ['-', 'baseline 500 kHz LP', '-', '-', 'baseline 100 kHz LP',
                  '-', '-', 'baseline 10 kHz LP', '-', '']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "blue", "blue", "red", "red",
                  "red", "green", "green", "green", "blue"]
        styles = [2, 1, 2, 2, 0,
                  2, 2, 1, 2, 1]
        markers = [-1, -1, -1, -1, -1,
                   -1, -1, -1, -1, -1]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]

        for i in xrange(9):
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
        self.digitizers_time_realignment_ff_0_0_0 = digitizers.time_realignment_ff(samp_rate, -0.00)
        self.digitizers_time_realignment_ff_0_0 = digitizers.time_realignment_ff(samp_rate, 0.00)
        self.digitizers_time_realignment_ff_0 = digitizers.time_realignment_ff(samp_rate, 0.0)
        self.digitizers_picoscope_3000a_0 = digitizers.picoscope_3000a('', True)
        self.digitizers_picoscope_3000a_0.set_trigger_once(False)
        self.digitizers_picoscope_3000a_0.set_samp_rate(samp_rate)
        self.digitizers_picoscope_3000a_0.set_downsampling(0, 16)
        self.digitizers_picoscope_3000a_0.set_aichan('A', True, 1, True, 0.0)
        self.digitizers_picoscope_3000a_0.set_aichan('B', False, 5, True, 0.0)
        self.digitizers_picoscope_3000a_0.set_aichan('C', False, 5, False, 5.0)
        self.digitizers_picoscope_3000a_0.set_aichan('D', False, 5.0, False, 0.0)

        self.digitizers_picoscope_3000a_0.set_diport('port0', False, 2.5)
        self.digitizers_picoscope_3000a_0.set_diport('port1', False, 2.5)

        if 'A' != 'None':
            if 'A' == 'Digital':
                self.digitizers_picoscope_3000a_0.set_di_trigger(0, 0)
            else:
                self.digitizers_picoscope_3000a_0.set_aichan_trigger('A', 0, 0.95)

        if 'Streaming' == 'Streaming':
            self.digitizers_picoscope_3000a_0.set_buffer_size(500000)
            self.digitizers_picoscope_3000a_0.set_nr_buffers(100)
            self.digitizers_picoscope_3000a_0.set_driver_buffer_size(100000)
            self.digitizers_picoscope_3000a_0.set_streaming(0.0005)
        else:
            self.digitizers_picoscope_3000a_0.set_samples(samples*2, pre_samples*2)
            self.digitizers_picoscope_3000a_0.set_rapid_block(2)

        self.digitizers_demux_ff_0_0_0 = digitizers.demux_ff(samp_rate, history, samples/decimate, pre_samples/decimate)
        self.digitizers_demux_ff_0_0 = digitizers.demux_ff(samp_rate, history, samples/decimate, pre_samples/decimate)
        self.digitizers_demux_ff_0 = digitizers.demux_ff(samp_rate, history, samples/decimate, pre_samples/decimate)
        self.digitizers_block_aggregation_0_1_0_0 = digitizers.block_aggregation(0, decimate, 0, (()), 0, 10000, 1000, (()), (()), samp_rate)
        self.digitizers_block_aggregation_0_1_0 = digitizers.block_aggregation(0, decimate, 0, (()), 0, 100000, 10000, (()), (()), samp_rate)
        self.digitizers_block_aggregation_0_1 = digitizers.block_aggregation(0, decimate, 0, (()), 0, 500000, 50000, (()), (()), samp_rate)
        self.blocks_tag_gate_0_2 = blocks.tag_gate(gr.sizeof_float * 1, False)
        self.blocks_tag_gate_0_2.set_single_key("")
        self.blocks_tag_gate_0_1_0_0 = blocks.tag_gate(gr.sizeof_float * 1, False)
        self.blocks_tag_gate_0_1_0_0.set_single_key("")
        self.blocks_tag_gate_0_1_0 = blocks.tag_gate(gr.sizeof_float * 1, False)
        self.blocks_tag_gate_0_1_0.set_single_key("")
        self.blocks_tag_gate_0_1 = blocks.tag_gate(gr.sizeof_float * 1, False)
        self.blocks_tag_gate_0_1.set_single_key("")
        self.blocks_tag_gate_0_0_0 = blocks.tag_gate(gr.sizeof_float * 1, False)
        self.blocks_tag_gate_0_0_0.set_single_key("")
        self.blocks_tag_gate_0_0 = blocks.tag_gate(gr.sizeof_float * 1, False)
        self.blocks_tag_gate_0_0.set_single_key("")
        self.blocks_tag_gate_0 = blocks.tag_gate(gr.sizeof_float * 1, False)
        self.blocks_tag_gate_0.set_single_key("")
        self.blocks_add_const_vxx_0_0 = blocks.add_const_vff((0.5, ))
        self.blocks_add_const_vxx_0 = blocks.add_const_vff((-0.5, ))
        self.block_error_band_0_0_0 = block_error_band()
        self.block_error_band_0_0 = block_error_band()
        self.block_error_band_0 = block_error_band()



        ##################################################
        # Connections
        ##################################################
        self.connect((self.block_error_band_0, 0), (self.blocks_tag_gate_0_1, 0))
        self.connect((self.block_error_band_0, 2), (self.blocks_tag_gate_0_1_0, 0))
        self.connect((self.block_error_band_0, 1), (self.blocks_tag_gate_0_1_0_0, 0))
        self.connect((self.block_error_band_0, 1), (self.qtgui_time_sink_x_0_0, 0))
        self.connect((self.block_error_band_0_0, 0), (self.blocks_tag_gate_0, 0))
        self.connect((self.block_error_band_0_0, 2), (self.blocks_tag_gate_0_0, 0))
        self.connect((self.block_error_band_0_0, 1), (self.qtgui_time_sink_x_0, 4))
        self.connect((self.block_error_band_0_0, 1), (self.qtgui_time_sink_x_0_0, 1))
        self.connect((self.block_error_band_0_0_0, 2), (self.blocks_tag_gate_0_0_0, 0))
        self.connect((self.block_error_band_0_0_0, 0), (self.blocks_tag_gate_0_2, 0))
        self.connect((self.block_error_band_0_0_0, 1), (self.qtgui_time_sink_x_0, 7))
        self.connect((self.block_error_band_0_0_0, 1), (self.qtgui_time_sink_x_0_0, 2))
        self.connect((self.blocks_add_const_vxx_0, 0), (self.block_error_band_0_0, 0))
        self.connect((self.blocks_add_const_vxx_0_0, 0), (self.block_error_band_0_0_0, 0))
        self.connect((self.blocks_tag_gate_0, 0), (self.qtgui_time_sink_x_0, 3))
        self.connect((self.blocks_tag_gate_0_0, 0), (self.qtgui_time_sink_x_0, 5))
        self.connect((self.blocks_tag_gate_0_0_0, 0), (self.qtgui_time_sink_x_0, 8))
        self.connect((self.blocks_tag_gate_0_1, 0), (self.qtgui_time_sink_x_0, 0))
        self.connect((self.blocks_tag_gate_0_1_0, 0), (self.qtgui_time_sink_x_0, 2))
        self.connect((self.blocks_tag_gate_0_1_0_0, 0), (self.qtgui_time_sink_x_0, 1))
        self.connect((self.blocks_tag_gate_0_2, 0), (self.qtgui_time_sink_x_0, 6))
        self.connect((self.digitizers_block_aggregation_0_1, 0), (self.digitizers_demux_ff_0, 0))
        self.connect((self.digitizers_block_aggregation_0_1, 1), (self.digitizers_demux_ff_0, 1))
        self.connect((self.digitizers_block_aggregation_0_1_0, 0), (self.digitizers_demux_ff_0_0, 0))
        self.connect((self.digitizers_block_aggregation_0_1_0, 1), (self.digitizers_demux_ff_0_0, 1))
        self.connect((self.digitizers_block_aggregation_0_1_0_0, 0), (self.digitizers_demux_ff_0_0_0, 0))
        self.connect((self.digitizers_block_aggregation_0_1_0_0, 1), (self.digitizers_demux_ff_0_0_0, 1))
        self.connect((self.digitizers_demux_ff_0, 0), (self.block_error_band_0, 0))
        self.connect((self.digitizers_demux_ff_0, 1), (self.block_error_band_0, 1))
        self.connect((self.digitizers_demux_ff_0_0, 1), (self.block_error_band_0_0, 1))
        self.connect((self.digitizers_demux_ff_0_0, 0), (self.blocks_add_const_vxx_0, 0))
        self.connect((self.digitizers_demux_ff_0_0_0, 1), (self.block_error_band_0_0_0, 1))
        self.connect((self.digitizers_demux_ff_0_0_0, 0), (self.blocks_add_const_vxx_0_0, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 0), (self.digitizers_time_realignment_ff_0, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 1), (self.digitizers_time_realignment_ff_0, 1))
        self.connect((self.digitizers_picoscope_3000a_0, 0), (self.digitizers_time_realignment_ff_0_0, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 1), (self.digitizers_time_realignment_ff_0_0, 1))
        self.connect((self.digitizers_picoscope_3000a_0, 0), (self.digitizers_time_realignment_ff_0_0_0, 0))
        self.connect((self.digitizers_picoscope_3000a_0, 1), (self.digitizers_time_realignment_ff_0_0_0, 1))
        self.connect((self.digitizers_time_realignment_ff_0, 1), (self.digitizers_block_aggregation_0_1, 1))
        self.connect((self.digitizers_time_realignment_ff_0, 0), (self.digitizers_block_aggregation_0_1, 0))
        self.connect((self.digitizers_time_realignment_ff_0_0, 1), (self.digitizers_block_aggregation_0_1_0, 1))
        self.connect((self.digitizers_time_realignment_ff_0_0, 0), (self.digitizers_block_aggregation_0_1_0, 0))
        self.connect((self.digitizers_time_realignment_ff_0_0_0, 1), (self.digitizers_block_aggregation_0_1_0_0, 1))
        self.connect((self.digitizers_time_realignment_ff_0_0_0, 0), (self.digitizers_block_aggregation_0_1_0_0, 0))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "b2_time_realignment_test")
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
        self.qtgui_time_sink_x_0_0.set_samp_rate(self.samp_rate/self.decimate)
        self.qtgui_time_sink_x_0.set_samp_rate(self.samp_rate/self.decimate)
        self.digitizers_block_aggregation_0_1_0_0.update_design(0, (()), 0, 10000, 1000, (()), (()), self.samp_rate)
        self.digitizers_block_aggregation_0_1_0.update_design(0, (()), 0, 100000, 10000, (()), (()), self.samp_rate)
        self.digitizers_block_aggregation_0_1.update_design(0, (()), 0, 500000, 50000, (()), (()), self.samp_rate)

    def get_pre_samples(self):
        return self.pre_samples

    def set_pre_samples(self, pre_samples):
        self.pre_samples = pre_samples

    def get_history(self):
        return self.history

    def set_history(self, history):
        self.history = history

    def get_decimate(self):
        return self.decimate

    def set_decimate(self, decimate):
        self.decimate = decimate
        self.qtgui_time_sink_x_0_0.set_samp_rate(self.samp_rate/self.decimate)
        self.qtgui_time_sink_x_0.set_samp_rate(self.samp_rate/self.decimate)


def main(top_block_cls=b2_time_realignment_test, options=None):

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
