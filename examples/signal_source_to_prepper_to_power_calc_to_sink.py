#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Not titled yet
# Author: gsi
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
from gnuradio import analog
from gnuradio import blocks
from gnuradio import gr
from gnuradio.fft import window
import sys
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
from gnuradio import zeromq
import pulsed_power_daq



from gnuradio import qtgui

class signal_source_to_prepper_to_power_calc_to_sink(gr.top_block, Qt.QWidget):

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

        self.settings = Qt.QSettings("GNU Radio", "signal_source_to_prepper_to_power_calc_to_sink")

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
        self.zeromq_pub_sink_0 = zeromq.pub_sink(gr.sizeof_float, 1, 'tcp://10.0.0.2:5001', 100, False, -1, '')
        self.qtgui_time_sink_x_0_0 = qtgui.time_sink_f(
            1024, #size
            samp_rate, #samp_rate
            "", #name
            9, #number of inputs
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
        self.qtgui_time_sink_x_0_0.enable_control_panel(True)
        self.qtgui_time_sink_x_0_0.enable_stem_plot(False)


        labels = ['Phi', 'band pass current', 'band pass voltage', 'raw current', 'raw voltage',
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


        for i in range(9):
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
        self.pulsed_power_daq_power_calc_ff_prepper_0 = pulsed_power_daq.power_calc_ff_prepper()
        self.pulsed_power_daq_power_calc_ff_0 = pulsed_power_daq.power_calc_ff(0.0001)
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_float*1)
        self.analog_sig_source_x_0_0 = analog.sig_source_f(samp_rate, analog.GR_COS_WAVE, 50, 2, 0, 0.5)
        self.analog_sig_source_x_0 = analog.sig_source_f(samp_rate, analog.GR_COS_WAVE, 50, 5, 0, 0)


        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_sig_source_x_0, 0), (self.pulsed_power_daq_power_calc_ff_prepper_0, 0))
        self.connect((self.analog_sig_source_x_0_0, 0), (self.pulsed_power_daq_power_calc_ff_prepper_0, 1))
        self.connect((self.pulsed_power_daq_power_calc_ff_0, 2), (self.qtgui_time_sink_x_0_0, 7))
        self.connect((self.pulsed_power_daq_power_calc_ff_0, 0), (self.qtgui_time_sink_x_0_0, 5))
        self.connect((self.pulsed_power_daq_power_calc_ff_0, 3), (self.qtgui_time_sink_x_0_0, 8))
        self.connect((self.pulsed_power_daq_power_calc_ff_0, 1), (self.qtgui_time_sink_x_0_0, 6))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 3), (self.blocks_null_sink_0, 0))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 4), (self.blocks_null_sink_0, 1))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 2), (self.pulsed_power_daq_power_calc_ff_0, 0))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 0), (self.pulsed_power_daq_power_calc_ff_0, 2))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 1), (self.pulsed_power_daq_power_calc_ff_0, 1))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 3), (self.qtgui_time_sink_x_0_0, 3))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 4), (self.qtgui_time_sink_x_0_0, 4))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 2), (self.qtgui_time_sink_x_0_0, 2))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 0), (self.qtgui_time_sink_x_0_0, 0))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 1), (self.qtgui_time_sink_x_0_0, 1))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 4), (self.zeromq_pub_sink_0, 0))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "signal_source_to_prepper_to_power_calc_to_sink")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.analog_sig_source_x_0.set_sampling_freq(self.samp_rate)
        self.analog_sig_source_x_0_0.set_sampling_freq(self.samp_rate)
        self.qtgui_time_sink_x_0_0.set_samp_rate(self.samp_rate)




def main(top_block_cls=signal_source_to_prepper_to_power_calc_to_sink, options=None):

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
