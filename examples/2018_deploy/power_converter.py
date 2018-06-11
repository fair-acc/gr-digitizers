#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Default Power Converter DAQ
# Author: rstein
# Description: default configuration for AEG power converter acquisition
# Generated: Mon Jun 11 06:44:26 2018
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
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import digitizers
import sys
from gnuradio import qtgui


class power_converter(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Default Power Converter DAQ")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Default Power Converter DAQ")
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

        self.settings = Qt.QSettings("GNU Radio", "power_converter")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())


        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 20000000

        ##################################################
        # Blocks
        ##################################################
        self.digitizers_time_realignment_ff_channel_U_SET = digitizers.time_realignment_ff(samp_rate, 0.0)
        self.digitizers_time_realignment_ff_channel_I_SET = digitizers.time_realignment_ff(samp_rate, 0.0)
        self.digitizers_time_realignment_ff_channel_I = digitizers.time_realignment_ff(samp_rate, 0.0)
        self.digitizers_time_realignment_ff__channel_U_TST = digitizers.time_realignment_ff(samp_rate, 0.0)
        self.digitizers_time_realignment_ff__channel_U = digitizers.time_realignment_ff(samp_rate, 0.0)
        self.digitizers_time_realignment_ff__channel_SPARE = digitizers.time_realignment_ff(samp_rate, 0.0)
        self.digitizers_time_realignment_ff__channel_DELTA_I = digitizers.time_realignment_ff(samp_rate, 0.0)
        self.digitizers_picoscope_4000a_0 = digitizers.picoscope_4000a('EW413/194', True)
        self.digitizers_picoscope_4000a_0.set_trigger_once(False)
        self.digitizers_picoscope_4000a_0.set_samp_rate(samp_rate)
        self.digitizers_picoscope_4000a_0.set_downsampling(0, 20)
        self.digitizers_picoscope_4000a_0.set_aichan('A', True, 1, True, 0)
        self.digitizers_picoscope_4000a_0.set_aichan('B', True, 1, True, 0.0)
        self.digitizers_picoscope_4000a_0.set_aichan('C', True, 1, True, 0)
        self.digitizers_picoscope_4000a_0.set_aichan('D', True, 1, True, 0.0)
        self.digitizers_picoscope_4000a_0.set_aichan('E', True, 1, True, 0.0)
        self.digitizers_picoscope_4000a_0.set_aichan('F', True, 1, True, 0.0)
        self.digitizers_picoscope_4000a_0.set_aichan('G', True, 1, True, 0.0)
        self.digitizers_picoscope_4000a_0.set_aichan('H', True, 1, True, 0.0)

        if 'H' != 'None':
            self.digitizers_picoscope_4000a_0.set_aichan_trigger('H', 0, 0.3)

        if 'Streaming' == 'Streaming':
            self.digitizers_picoscope_4000a_0.set_buffer_size(10000)
            self.digitizers_picoscope_4000a_0.set_nr_buffers(100)
            self.digitizers_picoscope_4000a_0.set_driver_buffer_size(100000)
            self.digitizers_picoscope_4000a_0.set_streaming(0.001)
        else:
            self.digitizers_picoscope_4000a_0.set_samples(10000, 0)
            self.digitizers_picoscope_4000a_0.set_rapid_block(5)

        self.digitizers_cascade_sink_channel_U_TST = digitizers.cascade_sink(4, 0, (), 1000, 10000, 1000, (), (), samp_rate, 2.0, ':U_TST', 'V')
        self.digitizers_cascade_sink_channel_TIMING = digitizers.cascade_sink(4, 0, (), 1000, 10000, 1000, (), (), samp_rate, 2.0, ':TIMING', 'V')
        self.digitizers_cascade_sink_channel_SPARE = digitizers.cascade_sink(4, 0, (), 1000, 10000, 1000, (), (), samp_rate, 2.0, ':SPARE', 'V')
        self.digitizers_cascade_sink_channel_I_SET = digitizers.cascade_sink(4, 0, (), 1000, 5000, 1000, (), (), samp_rate, 2.0, ':I_SET', 'A')
        self.digitizers_cascade_sink__channel_U_SET = digitizers.cascade_sink(4, 0, (), 1000, 10000, 1000, (), (), samp_rate, 2.0, ':U_SET', 'V')
        self.digitizers_cascade_sink__channel_U = digitizers.cascade_sink(4, 0, (), 1000, 10000, 1000, (), (), samp_rate, 2.0, ':U', 'V')
        self.digitizers_cascade_sink__channel_I = digitizers.cascade_sink(4, 0, (), 1000, 10000, 1000, (), (), samp_rate, 2.0, ':I', 'A')
        self.digitizers_cascade_sink__channel_DELTA_I = digitizers.cascade_sink(4, 0, (), 1000, 10000, 1000, (), (), samp_rate, 2.0, ':DELTA_I', 'A')
        self.digitizers_block_scaling_offset_channel_U_TST = digitizers.block_scaling_offset(1.0, 0.0)
        self.digitizers_block_scaling_offset_channel_U_SET = digitizers.block_scaling_offset(1.0, 0.0)
        self.digitizers_block_scaling_offset_channel_U = digitizers.block_scaling_offset(1.0, 0.0)
        self.digitizers_block_scaling_offset_channel_SPARE = digitizers.block_scaling_offset(1.0, 0.0)
        self.digitizers_block_scaling_offset_channel_I_SET = digitizers.block_scaling_offset(1.0, 0.0)
        self.digitizers_block_scaling_offset_channel_I = digitizers.block_scaling_offset(1.0, 0.0)
        self.digitizers_block_scaling_offset_channel_DELTA_I = digitizers.block_scaling_offset(1.0, 0.0)
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_float*1)



        ##################################################
        # Connections
        ##################################################
        self.connect((self.digitizers_block_scaling_offset_channel_DELTA_I, 1), (self.digitizers_time_realignment_ff__channel_DELTA_I, 1))
        self.connect((self.digitizers_block_scaling_offset_channel_DELTA_I, 0), (self.digitizers_time_realignment_ff__channel_DELTA_I, 0))
        self.connect((self.digitizers_block_scaling_offset_channel_I, 1), (self.digitizers_time_realignment_ff_channel_I, 1))
        self.connect((self.digitizers_block_scaling_offset_channel_I, 0), (self.digitizers_time_realignment_ff_channel_I, 0))
        self.connect((self.digitizers_block_scaling_offset_channel_I_SET, 1), (self.digitizers_time_realignment_ff_channel_I_SET, 1))
        self.connect((self.digitizers_block_scaling_offset_channel_I_SET, 0), (self.digitizers_time_realignment_ff_channel_I_SET, 0))
        self.connect((self.digitizers_block_scaling_offset_channel_SPARE, 1), (self.digitizers_time_realignment_ff__channel_SPARE, 1))
        self.connect((self.digitizers_block_scaling_offset_channel_SPARE, 0), (self.digitizers_time_realignment_ff__channel_SPARE, 0))
        self.connect((self.digitizers_block_scaling_offset_channel_U, 1), (self.digitizers_time_realignment_ff__channel_U, 1))
        self.connect((self.digitizers_block_scaling_offset_channel_U, 0), (self.digitizers_time_realignment_ff__channel_U, 0))
        self.connect((self.digitizers_block_scaling_offset_channel_U_SET, 1), (self.digitizers_time_realignment_ff_channel_U_SET, 1))
        self.connect((self.digitizers_block_scaling_offset_channel_U_SET, 0), (self.digitizers_time_realignment_ff_channel_U_SET, 0))
        self.connect((self.digitizers_block_scaling_offset_channel_U_TST, 1), (self.digitizers_time_realignment_ff__channel_U_TST, 1))
        self.connect((self.digitizers_block_scaling_offset_channel_U_TST, 0), (self.digitizers_time_realignment_ff__channel_U_TST, 0))
        self.connect((self.digitizers_picoscope_4000a_0, 14), (self.blocks_null_sink_0, 0))
        self.connect((self.digitizers_picoscope_4000a_0, 15), (self.blocks_null_sink_0, 1))
        self.connect((self.digitizers_picoscope_4000a_0, 8), (self.digitizers_block_scaling_offset_channel_DELTA_I, 0))
        self.connect((self.digitizers_picoscope_4000a_0, 9), (self.digitizers_block_scaling_offset_channel_DELTA_I, 1))
        self.connect((self.digitizers_picoscope_4000a_0, 4), (self.digitizers_block_scaling_offset_channel_I, 0))
        self.connect((self.digitizers_picoscope_4000a_0, 5), (self.digitizers_block_scaling_offset_channel_I, 1))
        self.connect((self.digitizers_picoscope_4000a_0, 0), (self.digitizers_block_scaling_offset_channel_I_SET, 0))
        self.connect((self.digitizers_picoscope_4000a_0, 1), (self.digitizers_block_scaling_offset_channel_I_SET, 1))
        self.connect((self.digitizers_picoscope_4000a_0, 12), (self.digitizers_block_scaling_offset_channel_SPARE, 0))
        self.connect((self.digitizers_picoscope_4000a_0, 13), (self.digitizers_block_scaling_offset_channel_SPARE, 1))
        self.connect((self.digitizers_picoscope_4000a_0, 6), (self.digitizers_block_scaling_offset_channel_U, 0))
        self.connect((self.digitizers_picoscope_4000a_0, 7), (self.digitizers_block_scaling_offset_channel_U, 1))
        self.connect((self.digitizers_picoscope_4000a_0, 2), (self.digitizers_block_scaling_offset_channel_U_SET, 0))
        self.connect((self.digitizers_picoscope_4000a_0, 3), (self.digitizers_block_scaling_offset_channel_U_SET, 1))
        self.connect((self.digitizers_picoscope_4000a_0, 10), (self.digitizers_block_scaling_offset_channel_U_TST, 0))
        self.connect((self.digitizers_picoscope_4000a_0, 11), (self.digitizers_block_scaling_offset_channel_U_TST, 1))
        self.connect((self.digitizers_picoscope_4000a_0, 14), (self.digitizers_cascade_sink_channel_TIMING, 0))
        self.connect((self.digitizers_picoscope_4000a_0, 15), (self.digitizers_cascade_sink_channel_TIMING, 1))
        self.connect((self.digitizers_time_realignment_ff__channel_DELTA_I, 1), (self.digitizers_cascade_sink__channel_DELTA_I, 1))
        self.connect((self.digitizers_time_realignment_ff__channel_DELTA_I, 0), (self.digitizers_cascade_sink__channel_DELTA_I, 0))
        self.connect((self.digitizers_time_realignment_ff__channel_SPARE, 1), (self.digitizers_cascade_sink_channel_SPARE, 1))
        self.connect((self.digitizers_time_realignment_ff__channel_SPARE, 0), (self.digitizers_cascade_sink_channel_SPARE, 0))
        self.connect((self.digitizers_time_realignment_ff__channel_U, 1), (self.digitizers_cascade_sink__channel_U, 1))
        self.connect((self.digitizers_time_realignment_ff__channel_U, 0), (self.digitizers_cascade_sink__channel_U, 0))
        self.connect((self.digitizers_time_realignment_ff__channel_U_TST, 1), (self.digitizers_cascade_sink_channel_U_TST, 1))
        self.connect((self.digitizers_time_realignment_ff__channel_U_TST, 0), (self.digitizers_cascade_sink_channel_U_TST, 0))
        self.connect((self.digitizers_time_realignment_ff_channel_I, 1), (self.digitizers_cascade_sink__channel_I, 1))
        self.connect((self.digitizers_time_realignment_ff_channel_I, 0), (self.digitizers_cascade_sink__channel_I, 0))
        self.connect((self.digitizers_time_realignment_ff_channel_I_SET, 1), (self.digitizers_cascade_sink_channel_I_SET, 1))
        self.connect((self.digitizers_time_realignment_ff_channel_I_SET, 0), (self.digitizers_cascade_sink_channel_I_SET, 0))
        self.connect((self.digitizers_time_realignment_ff_channel_U_SET, 1), (self.digitizers_cascade_sink__channel_U_SET, 1))
        self.connect((self.digitizers_time_realignment_ff_channel_U_SET, 0), (self.digitizers_cascade_sink__channel_U_SET, 0))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "power_converter")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate


def main(top_block_cls=power_converter, options=None):

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
