#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Not titled yet
# Author: p01900
# GNU Radio version: 3.10.1.1

from packaging.version import Version as StrictVersion

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print("Warning: failed to XInitThreads()")

from PyQt5 import Qt
from gnuradio import qtgui
from gnuradio.filter import firdes
import sip
from gnuradio import gr
from gnuradio.fft import window
import sys
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
import pulsed_power_daq



from gnuradio import qtgui

class test_PS_20220303(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Not titled yet", catch_exceptions=True)
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Not titled yet")
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

        self.settings = Qt.QSettings("GNU Radio", "test_PS_20220303")

        try:
            if StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
                self.restoreGeometry(self.settings.value("geometry").toByteArray())
            else:
                self.restoreGeometry(self.settings.value("geometry"))
        except:
            pass

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 32000

        ##################################################
        # Blocks
        ##################################################
        self.qtgui_time_sink_x_0_0 = qtgui.time_sink_f(
            1024, #size
            samp_rate, #samp_rate
            "", #name
            6, #number of inputs
            None # parent
        )
        self.qtgui_time_sink_x_0_0.set_update_time(0.10)
        self.qtgui_time_sink_x_0_0.set_y_axis(-1, 1)

        self.qtgui_time_sink_x_0_0.set_y_label('Amplitude', "")

        self.qtgui_time_sink_x_0_0.enable_tags(True)
        self.qtgui_time_sink_x_0_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.qtgui_time_sink_x_0_0.enable_autoscale(False)
        self.qtgui_time_sink_x_0_0.enable_grid(False)
        self.qtgui_time_sink_x_0_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0_0.enable_control_panel(False)
        self.qtgui_time_sink_x_0_0.enable_stem_plot(False)


        labels = ['Signal 1', 'Signal 2', 'Signal 3', 'Signal 4', 'Signal 5',
            'Signal 6', 'Signal 7', 'Signal 8', 'Signal 9', 'Signal 10']
        widths = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        colors = ['blue', 'red', 'green', 'black', 'cyan',
            'magenta', 'yellow', 'dark red', 'dark green', 'dark blue']
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
            1.0, 1.0, 1.0, 1.0, 1.0]
        styles = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1]


        for i in range(6):
            if len(labels[i]) == 0:
                self.qtgui_time_sink_x_0_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_time_sink_x_0_0.set_line_label(i, labels[i])
            self.qtgui_time_sink_x_0_0.set_line_width(i, widths[i])
            self.qtgui_time_sink_x_0_0.set_line_color(i, colors[i])
            self.qtgui_time_sink_x_0_0.set_line_style(i, styles[i])
            self.qtgui_time_sink_x_0_0.set_line_marker(i, markers[i])
            self.qtgui_time_sink_x_0_0.set_line_alpha(i, alphas[i])

        self._qtgui_time_sink_x_0_0_win = sip.wrapinstance(self.qtgui_time_sink_x_0_0.qwidget(), Qt.QWidget)
        self.top_layout.addWidget(self._qtgui_time_sink_x_0_0_win)
        self.qtgui_time_sink_x_0 = qtgui.time_sink_f(
            1024, #size
            samp_rate, #samp_rate
            "", #name
            10, #number of inputs
            None # parent
        )
        self.qtgui_time_sink_x_0.set_update_time(0.10)
        self.qtgui_time_sink_x_0.set_y_axis(-1, 1)

        self.qtgui_time_sink_x_0.set_y_label('Amplitude', "")

        self.qtgui_time_sink_x_0.enable_tags(True)
        self.qtgui_time_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.qtgui_time_sink_x_0.enable_autoscale(False)
        self.qtgui_time_sink_x_0.enable_grid(False)
        self.qtgui_time_sink_x_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0.enable_control_panel(False)
        self.qtgui_time_sink_x_0.enable_stem_plot(False)


        labels = ['Signal 1', 'Signal 2', 'Signal 3', 'Signal 4', 'Signal 5',
            'Signal 6', 'Signal 7', 'Signal 8', 'Signal 9', 'Signal 10']
        widths = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        colors = ['blue', 'red', 'green', 'black', 'cyan',
            'magenta', 'yellow', 'dark red', 'dark green', 'dark blue']
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
            1.0, 1.0, 1.0, 1.0, 1.0]
        styles = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1]


        for i in range(10):
            if len(labels[i]) == 0:
                self.qtgui_time_sink_x_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_time_sink_x_0.set_line_label(i, labels[i])
            self.qtgui_time_sink_x_0.set_line_width(i, widths[i])
            self.qtgui_time_sink_x_0.set_line_color(i, colors[i])
            self.qtgui_time_sink_x_0.set_line_style(i, styles[i])
            self.qtgui_time_sink_x_0.set_line_marker(i, markers[i])
            self.qtgui_time_sink_x_0.set_line_alpha(i, alphas[i])

        self._qtgui_time_sink_x_0_win = sip.wrapinstance(self.qtgui_time_sink_x_0.qwidget(), Qt.QWidget)
        self.top_layout.addWidget(self._qtgui_time_sink_x_0_win)
        self.pulsed_power_daq_picoscope_4000a_source_0 = pulsed_power_daq.picoscope_4000a_source('', True)
        self.pulsed_power_daq_picoscope_4000a_source_0.set_trigger_once(False)
        self.pulsed_power_daq_picoscope_4000a_source_0.set_samp_rate(samp_rate)
        self.pulsed_power_daq_picoscope_4000a_source_0.set_downsampling(0, 1)
        self.pulsed_power_daq_picoscope_4000a_source_0.set_aichan_a(True, 5, 1, 0.0)
        self.pulsed_power_daq_picoscope_4000a_source_0.set_aichan_b(True, 1, 1, 0.0)
        self.pulsed_power_daq_picoscope_4000a_source_0.set_aichan_c( False, 5.0, 0, 5.0)
        self.pulsed_power_daq_picoscope_4000a_source_0.set_aichan_d( False, 5.0, 0, 0.0)
        self.pulsed_power_daq_picoscope_4000a_source_0.set_aichan_e( False, 5, 0, 0.0)
        self.pulsed_power_daq_picoscope_4000a_source_0.set_aichan_f( False, 5, 0, 0.0)
        self.pulsed_power_daq_picoscope_4000a_source_0.set_aichan_g( False, 5.0, 0, 5.0)
        self.pulsed_power_daq_picoscope_4000a_source_0.set_aichan_h( False, 5.0, 0, 0.0)

        if 'None' != 'None':
            self.pulsed_power_daq_picoscope_4000a_source_0.set_aichan_trigger('None', 0, 2.5)

        if 'Rapid Block' == 'Streaming':
            self.pulsed_power_daq_picoscope_4000a_source_0.set_nr_buffers(64)
            self.pulsed_power_daq_picoscope_4000a_source_0.set_driver_buffer_size(102400)
            self.pulsed_power_daq_picoscope_4000a_source_0.set_streaming(0.0005)
        else:
            self.pulsed_power_daq_picoscope_4000a_source_0.set_samples(500000, 10000)
            self.pulsed_power_daq_picoscope_4000a_source_0.set_rapid_block(5)



        ##################################################
        # Connections
        ##################################################
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 2), (self.qtgui_time_sink_x_0, 2))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 5), (self.qtgui_time_sink_x_0, 5))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 9), (self.qtgui_time_sink_x_0, 9))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 8), (self.qtgui_time_sink_x_0, 8))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 3), (self.qtgui_time_sink_x_0, 3))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 4), (self.qtgui_time_sink_x_0, 4))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 7), (self.qtgui_time_sink_x_0, 7))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 0), (self.qtgui_time_sink_x_0, 0))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 1), (self.qtgui_time_sink_x_0, 1))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 6), (self.qtgui_time_sink_x_0, 6))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 14), (self.qtgui_time_sink_x_0_0, 4))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 10), (self.qtgui_time_sink_x_0_0, 0))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 11), (self.qtgui_time_sink_x_0_0, 1))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 13), (self.qtgui_time_sink_x_0_0, 3))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 12), (self.qtgui_time_sink_x_0_0, 2))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 15), (self.qtgui_time_sink_x_0_0, 5))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "test_PS_20220303")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.pulsed_power_daq_picoscope_4000a_source_0.set_samp_rate(self.samp_rate)
        self.qtgui_time_sink_x_0.set_samp_rate(self.samp_rate)
        self.qtgui_time_sink_x_0_0.set_samp_rate(self.samp_rate)




def main(top_block_cls=test_PS_20220303, options=None):

    if StrictVersion("4.5.0") <= StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()

    tb.start()

    tb.show()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        Qt.QApplication.quit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    timer = Qt.QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    qapp.exec_()

if __name__ == '__main__':
    main()
