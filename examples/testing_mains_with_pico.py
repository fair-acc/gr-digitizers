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
import sip
from gnuradio import blocks
from gnuradio import gr
from gnuradio.filter import firdes
from gnuradio.fft import window
import sys
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
import pulsed_power_daq



from gnuradio import qtgui

class testing_mains_with_pico(gr.top_block, Qt.QWidget):

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

        self.settings = Qt.QSettings("GNU Radio", "testing_mains_with_pico")

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
        self.qtgui_number_sink_0 = qtgui.number_sink(
            gr.sizeof_float,
            0,
            qtgui.NUM_GRAPH_HORIZ,
            1,
            None # parent
        )
        self.qtgui_number_sink_0.set_update_time(0.10)
        self.qtgui_number_sink_0.set_title("")

        labels = ['', '', '', '', '',
            '', '', '', '', '']
        units = ['', '', '', '', '',
            '', '', '', '', '']
        colors = [("black", "black"), ("black", "black"), ("black", "black"), ("black", "black"), ("black", "black"),
            ("black", "black"), ("black", "black"), ("black", "black"), ("black", "black"), ("black", "black")]
        factor = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]

        for i in range(1):
            self.qtgui_number_sink_0.set_min(i, 49)
            self.qtgui_number_sink_0.set_max(i, 51)
            self.qtgui_number_sink_0.set_color(i, colors[i][0], colors[i][1])
            if len(labels[i]) == 0:
                self.qtgui_number_sink_0.set_label(i, "Data {0}".format(i))
            else:
                self.qtgui_number_sink_0.set_label(i, labels[i])
            self.qtgui_number_sink_0.set_unit(i, units[i])
            self.qtgui_number_sink_0.set_factor(i, factor[i])

        self.qtgui_number_sink_0.enable_autoscale(False)
        self._qtgui_number_sink_0_win = sip.wrapinstance(self.qtgui_number_sink_0.qwidget(), Qt.QWidget)
        self.top_layout.addWidget(self._qtgui_number_sink_0_win)
        self.pulsed_power_daq_picoscope_4000a_source_0 = pulsed_power_daq.picoscope_4000a_source('EW413/152', True)
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

        if 'Streaming' == 'Streaming':
            self.pulsed_power_daq_picoscope_4000a_source_0.set_nr_buffers(64)
            self.pulsed_power_daq_picoscope_4000a_source_0.set_driver_buffer_size(102400)
            self.pulsed_power_daq_picoscope_4000a_source_0.set_streaming(0.0005)
            self.pulsed_power_daq_picoscope_4000a_source_0.set_buffer_size(204800)
        else:
            self.pulsed_power_daq_picoscope_4000a_source_0.set_rapid_block(5)

        self.pulsed_power_daq_mains_frequency_calc_0 = pulsed_power_daq.mains_frequency_calc(2000000,-100,100)
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_float*1)


        ##################################################
        # Connections
        ##################################################
        self.connect((self.pulsed_power_daq_mains_frequency_calc_0, 0), (self.qtgui_number_sink_0, 0))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 1), (self.blocks_null_sink_0, 0))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 14), (self.blocks_null_sink_0, 8))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 8), (self.blocks_null_sink_0, 10))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 9), (self.blocks_null_sink_0, 3))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 13), (self.blocks_null_sink_0, 7))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 2), (self.blocks_null_sink_0, 1))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 11), (self.blocks_null_sink_0, 5))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 3), (self.blocks_null_sink_0, 2))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 6), (self.blocks_null_sink_0, 12))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 7), (self.blocks_null_sink_0, 11))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 12), (self.blocks_null_sink_0, 6))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 10), (self.blocks_null_sink_0, 4))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 15), (self.blocks_null_sink_0, 9))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 4), (self.blocks_null_sink_0, 14))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 5), (self.blocks_null_sink_0, 13))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 0), (self.pulsed_power_daq_mains_frequency_calc_0, 0))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "testing_mains_with_pico")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.pulsed_power_daq_picoscope_4000a_source_0.set_samp_rate(self.samp_rate)




def main(top_block_cls=testing_mains_with_pico, options=None):

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
