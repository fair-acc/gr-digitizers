#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: B.1)  Input Scaling and Offset
# Description: should be either static or BPC multiplexed
# Generated: Tue May  8 15:03:39 2018
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
from block_calibration import block_calibration  # grc-generated hier_block
from block_error_band import block_error_band  # grc-generated hier_block
from gnuradio import analog
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


class B1_calibration(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "B.1)  Input Scaling and Offset")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("B.1)  Input Scaling and Offset")
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

        self.settings = Qt.QSettings("GNU Radio", "B1_calibration")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())


        ##################################################
        # Variables
        ##################################################
        self.samples = samples = 2000
        self.samp_rate = samp_rate = 10000
        self.offset = offset = 1
        self.cal = cal = 10

        ##################################################
        # Blocks
        ##################################################
        self.qtgui_time_sink_x_0_0 = qtgui.time_sink_f(
        	samples, #size
        	samp_rate, #samp_rate
        	"", #name
        	6 #number of inputs
        )
        self.qtgui_time_sink_x_0_0.set_update_time(0.10)
        self.qtgui_time_sink_x_0_0.set_y_axis(-5, 15)

        self.qtgui_time_sink_x_0_0.set_y_label('cal Amplitude', "")

        self.qtgui_time_sink_x_0_0.enable_tags(-1, False)
        self.qtgui_time_sink_x_0_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.qtgui_time_sink_x_0_0.enable_autoscale(False)
        self.qtgui_time_sink_x_0_0.enable_grid(True)
        self.qtgui_time_sink_x_0_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0_0.enable_control_panel(False)
        self.qtgui_time_sink_x_0_0.enable_stem_plot(False)

        if not True:
          self.qtgui_time_sink_x_0_0.disable_legend()

        labels = ['-', 'reference', '-', '-', 'implementation under test',
                  '-', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "blue", "blue", "red", "red",
                  "red", "yellow", "dark red", "dark green", "blue"]
        styles = [2, 1, 2, 2, 1,
                  2, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
                   -1, -1, -1, -1, -1]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]

        for i in xrange(6):
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
        	samples, #size
        	samp_rate, #samp_rate
        	"", #name
        	3 #number of inputs
        )
        self.qtgui_time_sink_x_0.set_update_time(0.10)
        self.qtgui_time_sink_x_0.set_y_axis(-0.5, 1.5)

        self.qtgui_time_sink_x_0.set_y_label('raw Amplitude', "")

        self.qtgui_time_sink_x_0.enable_tags(-1, True)
        self.qtgui_time_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.qtgui_time_sink_x_0.enable_autoscale(False)
        self.qtgui_time_sink_x_0.enable_grid(True)
        self.qtgui_time_sink_x_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0.enable_control_panel(False)
        self.qtgui_time_sink_x_0.enable_stem_plot(False)

        if not True:
          self.qtgui_time_sink_x_0.disable_legend()

        labels = ['-', 'input signal', '-', '', '',
                  '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "blue", "blue", "black", "cyan",
                  "magenta", "yellow", "dark red", "dark green", "blue"]
        styles = [2, 1, 2, 1, 1,
                  1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
                   -1, -1, -1, -1, -1]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]

        for i in xrange(3):
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
        self.digitizers_block_scaling_offset_0 = digitizers.block_scaling_offset(cal, offset)
        self.blocks_throttle_0_0 = blocks.throttle(gr.sizeof_float*1, samp_rate,True)
        self.blocks_add_xx_0 = blocks.add_vff(1)
        self.block_error_band_0_0_0 = block_error_band()
        self.block_error_band_0_0 = block_error_band()
        self.block_error_band_0 = block_error_band()
        self.block_calibration_0_0 = block_calibration(
            cal=cal,
            offset=0,
        )
        self.block_calibration_0 = block_calibration(
            cal=cal,
            offset=offset,
        )
        self.analog_sig_source_x_0_0_0 = analog.sig_source_f(samp_rate, analog.GR_SAW_WAVE, 10, 1, 0.1)
        self.analog_sig_source_x_0 = analog.sig_source_f(samp_rate, analog.GR_CONST_WAVE, 1000, 0.1, 0)
        self.analog_noise_source_x_0_0 = analog.noise_source_f(analog.GR_GAUSSIAN, 0.001, 0)



        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_noise_source_x_0_0, 0), (self.blocks_add_xx_0, 0))
        self.connect((self.analog_sig_source_x_0, 0), (self.block_calibration_0_0, 0))
        self.connect((self.analog_sig_source_x_0, 0), (self.block_error_band_0, 1))
        self.connect((self.analog_sig_source_x_0, 0), (self.digitizers_block_scaling_offset_0, 1))
        self.connect((self.analog_sig_source_x_0_0_0, 0), (self.blocks_add_xx_0, 1))
        self.connect((self.block_calibration_0, 0), (self.block_error_band_0_0, 0))
        self.connect((self.block_calibration_0_0, 0), (self.block_error_band_0_0, 1))
        self.connect((self.block_error_band_0, 0), (self.qtgui_time_sink_x_0, 0))
        self.connect((self.block_error_band_0, 2), (self.qtgui_time_sink_x_0, 2))
        self.connect((self.block_error_band_0, 1), (self.qtgui_time_sink_x_0, 1))
        self.connect((self.block_error_band_0_0, 0), (self.qtgui_time_sink_x_0_0, 0))
        self.connect((self.block_error_band_0_0, 2), (self.qtgui_time_sink_x_0_0, 2))
        self.connect((self.block_error_band_0_0, 1), (self.qtgui_time_sink_x_0_0, 1))
        self.connect((self.block_error_band_0_0_0, 0), (self.qtgui_time_sink_x_0_0, 3))
        self.connect((self.block_error_band_0_0_0, 2), (self.qtgui_time_sink_x_0_0, 5))
        self.connect((self.block_error_band_0_0_0, 1), (self.qtgui_time_sink_x_0_0, 4))
        self.connect((self.blocks_add_xx_0, 0), (self.block_calibration_0, 0))
        self.connect((self.blocks_add_xx_0, 0), (self.blocks_throttle_0_0, 0))
        self.connect((self.blocks_add_xx_0, 0), (self.digitizers_block_scaling_offset_0, 0))
        self.connect((self.blocks_throttle_0_0, 0), (self.block_error_band_0, 0))
        self.connect((self.digitizers_block_scaling_offset_0, 1), (self.block_error_band_0_0_0, 1))
        self.connect((self.digitizers_block_scaling_offset_0, 0), (self.block_error_band_0_0_0, 0))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "B1_calibration")
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
        self.qtgui_time_sink_x_0_0.set_samp_rate(self.samp_rate)
        self.qtgui_time_sink_x_0.set_samp_rate(self.samp_rate)
        self.blocks_throttle_0_0.set_sample_rate(self.samp_rate)
        self.analog_sig_source_x_0_0_0.set_sampling_freq(self.samp_rate)
        self.analog_sig_source_x_0.set_sampling_freq(self.samp_rate)

    def get_offset(self):
        return self.offset

    def set_offset(self, offset):
        self.offset = offset
        self.digitizers_block_scaling_offset_0.update_design(self.cal, self.offset)
        self.block_calibration_0.set_offset(self.offset)

    def get_cal(self):
        return self.cal

    def set_cal(self, cal):
        self.cal = cal
        self.digitizers_block_scaling_offset_0.update_design(self.cal, self.offset)
        self.block_calibration_0_0.set_cal(self.cal)
        self.block_calibration_0.set_cal(self.cal)


def main(top_block_cls=B1_calibration, options=None):

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
