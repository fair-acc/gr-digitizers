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

from gnuradio import blocks
from gnuradio import filter
from gnuradio.filter import firdes
from gnuradio import gr
from gnuradio.fft import window
import sys
import signal
from PyQt5 import Qt
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
from gnuradio import zeromq
import pulsed_power_daq



from gnuradio import qtgui

class default(gr.top_block, Qt.QWidget):

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

        self.settings = Qt.QSettings("GNU Radio", "default")

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
        self.samp_rate = samp_rate = 2000000

        ##################################################
        # Blocks
        ##################################################
        self.zeromq_pub_sink_0_6 = zeromq.pub_sink(gr.sizeof_float, 4, 'tcp://10.0.0.2:5003', 100, False, -1, '')
        self.zeromq_pub_sink_0_2 = zeromq.pub_sink(gr.sizeof_float, 1, 'tcp://10.0.0.2:5005', 100, False, -1, '')
        self.zeromq_pub_sink_0_1 = zeromq.pub_sink(gr.sizeof_float, 2, 'tcp://10.0.0.2:5002', 100, False, -1, '')
        self.zeromq_pub_sink_0_0 = zeromq.pub_sink(gr.sizeof_float, 2, 'tcp://10.0.0.2:5001', 100, False, -1, '')
        self.zeromq_pub_sink_0 = zeromq.pub_sink(gr.sizeof_float, 1, 'tcp://10.0.0.2:5004', 100, False, -1, '')
        self.pulsed_power_daq_power_calc_ff_prepper_0 = pulsed_power_daq.power_calc_ff_prepper()
        self.pulsed_power_daq_power_calc_ff_0 = pulsed_power_daq.power_calc_ff(0.0001)
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

        if 'Streaming' == 'Streaming':
            self.pulsed_power_daq_picoscope_4000a_source_0.set_nr_buffers(64)
            self.pulsed_power_daq_picoscope_4000a_source_0.set_driver_buffer_size(102400)
            self.pulsed_power_daq_picoscope_4000a_source_0.set_streaming(0.0005)
            self.pulsed_power_daq_picoscope_4000a_source_0.set_buffer_size(204800)
        else:
            self.pulsed_power_daq_picoscope_4000a_source_0.set_rapid_block(5)

        self.pulsed_power_daq_mains_frequency_calc_0 = pulsed_power_daq.mains_frequency_calc(samp_rate,-100,100)
        self.low_pass_filter_0 = filter.fir_filter_fff(
            1,
            firdes.low_pass(
                1,
                samp_rate,
                20,
                100,
                window.WIN_HAMMING,
                6.76))
        self.blocks_streams_to_vector_2 = blocks.streams_to_vector(gr.sizeof_float*1, 2)
        self.blocks_streams_to_vector_1 = blocks.streams_to_vector(gr.sizeof_float*1, 2)
        self.blocks_streams_to_vector_0 = blocks.streams_to_vector(gr.sizeof_float*1, 4)
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_float*1)
        self.blocks_multiply_xx_0 = blocks.multiply_vff(1)
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_ff(100)


        ##################################################
        # Connections
        ##################################################
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.pulsed_power_daq_mains_frequency_calc_0, 0))
        self.connect((self.blocks_multiply_xx_0, 0), (self.low_pass_filter_0, 0))
        self.connect((self.blocks_streams_to_vector_0, 0), (self.zeromq_pub_sink_0_6, 0))
        self.connect((self.blocks_streams_to_vector_1, 0), (self.zeromq_pub_sink_0_0, 0))
        self.connect((self.blocks_streams_to_vector_2, 0), (self.zeromq_pub_sink_0_1, 0))
        self.connect((self.low_pass_filter_0, 0), (self.zeromq_pub_sink_0_2, 0))
        self.connect((self.pulsed_power_daq_mains_frequency_calc_0, 0), (self.zeromq_pub_sink_0, 0))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 0), (self.blocks_multiply_const_vxx_0, 0))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 14), (self.blocks_null_sink_0, 13))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 7), (self.blocks_null_sink_0, 6))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 13), (self.blocks_null_sink_0, 12))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 6), (self.blocks_null_sink_0, 5))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 12), (self.blocks_null_sink_0, 11))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 3), (self.blocks_null_sink_0, 2))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 10), (self.blocks_null_sink_0, 9))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 9), (self.blocks_null_sink_0, 8))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 2), (self.blocks_null_sink_0, 1))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 11), (self.blocks_null_sink_0, 10))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 15), (self.blocks_null_sink_0, 14))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 8), (self.blocks_null_sink_0, 7))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 1), (self.blocks_null_sink_0, 0))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 5), (self.blocks_null_sink_0, 4))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 4), (self.blocks_null_sink_0, 3))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 2), (self.pulsed_power_daq_power_calc_ff_prepper_0, 1))
        self.connect((self.pulsed_power_daq_picoscope_4000a_source_0, 0), (self.pulsed_power_daq_power_calc_ff_prepper_0, 0))
        self.connect((self.pulsed_power_daq_power_calc_ff_0, 2), (self.blocks_streams_to_vector_0, 2))
        self.connect((self.pulsed_power_daq_power_calc_ff_0, 1), (self.blocks_streams_to_vector_0, 1))
        self.connect((self.pulsed_power_daq_power_calc_ff_0, 3), (self.blocks_streams_to_vector_0, 3))
        self.connect((self.pulsed_power_daq_power_calc_ff_0, 0), (self.blocks_streams_to_vector_0, 0))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 3), (self.blocks_multiply_xx_0, 0))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 4), (self.blocks_multiply_xx_0, 1))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 4), (self.blocks_streams_to_vector_1, 1))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 3), (self.blocks_streams_to_vector_1, 0))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 1), (self.blocks_streams_to_vector_2, 0))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 2), (self.blocks_streams_to_vector_2, 1))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 1), (self.pulsed_power_daq_power_calc_ff_0, 1))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 2), (self.pulsed_power_daq_power_calc_ff_0, 0))
        self.connect((self.pulsed_power_daq_power_calc_ff_prepper_0, 0), (self.pulsed_power_daq_power_calc_ff_0, 2))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "default")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.low_pass_filter_0.set_taps(firdes.low_pass(1, self.samp_rate, 20, 100, window.WIN_HAMMING, 6.76))
        self.pulsed_power_daq_picoscope_4000a_source_0.set_samp_rate(self.samp_rate)




def main(top_block_cls=default, options=None):

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
