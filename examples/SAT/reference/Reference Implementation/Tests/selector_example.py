#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Selector Example
# Generated: Fri Aug  4 07:56:47 2017
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
from block_custom_filter import block_custom_filter  # grc-generated hier_block
from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio import qtgui
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from gnuradio.qtgui import Range, RangeWidget
from optparse import OptionParser
import sip
from gnuradio import qtgui


class selector_example(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Selector Example")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Selector Example")
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

        self.settings = Qt.QSettings("GNU Radio", "selector_example")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 100000
        self.fir_LP = fir_LP =  firdes.low_pass(1, samp_rate, 1000.0, 100.0, firdes.WIN_HAMMING, 6.76)
        self.algorithm_ID = algorithm_ID = 4
        self.IIR_feedforward_1 = IIR_feedforward_1 = 1.927812121365849e-08, 1.9278121213658484e-07, 8.675154546146312e-07, 2.313374545639017e-06, 4.0484054548682805e-06, 4.858086545841939e-06, 4.048405454868283e-06, 2.3133745456390203e-06, 8.675154546146333e-07, 1.9278121213658535e-07, 1.9278121213658454e-08
        self.IIR_feedforward_0 = IIR_feedforward_0 = 1.7738613342568748e-09, 8.86930667128439e-09, 1.773861334256881e-08, 1.7738613342568837e-08, 8.869306671284435e-09, 1.773861334256889e-09
        self.IIR_feedback_1 = IIR_feedback_1 = 1.0, -7.586357084563112, 26.131288821609992, -53.77540761007004, 73.16941578692908, -68.74321606884482, 45.14084094501371, -20.44899972126457, 6.113672770121455, -1.0889392670519782, 0.08772116891632613
        self.IIR_feedback_0 = IIR_feedback_0 = 1.0, -4.883632384956582, 9.54127151717133, -9.321780090620553, 4.554280419089137, -0.890139403919765

        ##################################################
        # Blocks
        ##################################################
        self._algorithm_ID_range = Range(0, 5, 1, 4, 200)
        self._algorithm_ID_win = RangeWidget(self._algorithm_ID_range, self.set_algorithm_ID, 'Algorithm ID', "counter", int)
        self.top_grid_layout.addWidget(self._algorithm_ID_win, 0,1,1,1)
        self.qtgui_time_sink_x_0 = qtgui.time_sink_f(
        	10000, #size
        	samp_rate, #samp_rate
        	"", #name
        	3 #number of inputs
        )
        self.qtgui_time_sink_x_0.set_update_time(0.10)
        self.qtgui_time_sink_x_0.set_y_axis(-2, 1)

        self.qtgui_time_sink_x_0.set_y_label('Amplitude', "")

        self.qtgui_time_sink_x_0.enable_tags(-1, True)
        self.qtgui_time_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.qtgui_time_sink_x_0.enable_autoscale(True)
        self.qtgui_time_sink_x_0.enable_grid(False)
        self.qtgui_time_sink_x_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0.enable_control_panel(False)

        if not True:
          self.qtgui_time_sink_x_0.disable_legend()

        labels = ['custom', 'raw', 'IIR', '', '',
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
        self.top_grid_layout.addWidget(self._qtgui_time_sink_x_0_win, 1,0,1,2)
        self.iir_filter_xxx_0_0 = filter.iir_filter_ffd((IIR_feedforward_1), (IIR_feedback_1), False)
        self.iir_filter_xxx_0 = filter.iir_filter_ffd((IIR_feedforward_1), (IIR_feedback_1), False)
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_float*1, samp_rate,True)
        self.block_custom_filter_0 = block_custom_filter(
            algorithm_ID=algorithm_ID,
            decimation=1,
            fir_Taps=fir_LP,
            freq_max=1000,
            freq_min=1,
            freq_transition=100,
            iir_feedback_Taps=IIR_feedback_0,
            iir_forward_Taps=IIR_feedforward_0,
            samp_rate=samp_rate,
        )
        self.analog_sig_source_x_0 = analog.sig_source_f(samp_rate, analog.GR_SQR_WAVE, 80.1, 1, 0)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_sig_source_x_0, 0), (self.block_custom_filter_0, 0))
        self.connect((self.analog_sig_source_x_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.analog_sig_source_x_0, 0), (self.iir_filter_xxx_0, 0))
        self.connect((self.block_custom_filter_0, 0), (self.qtgui_time_sink_x_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.qtgui_time_sink_x_0, 1))
        self.connect((self.iir_filter_xxx_0, 0), (self.iir_filter_xxx_0_0, 0))
        self.connect((self.iir_filter_xxx_0_0, 0), (self.qtgui_time_sink_x_0, 2))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "selector_example")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_fir_LP( firdes.low_pass(1, self.samp_rate, 1000.0, 100.0, firdes.WIN_HAMMING, 6.76))
        self.qtgui_time_sink_x_0.set_samp_rate(self.samp_rate)
        self.blocks_throttle_0.set_sample_rate(self.samp_rate)
        self.block_custom_filter_0.set_samp_rate(self.samp_rate)
        self.analog_sig_source_x_0.set_sampling_freq(self.samp_rate)

    def get_fir_LP(self):
        return self.fir_LP

    def set_fir_LP(self, fir_LP):
        self.fir_LP = fir_LP
        self.block_custom_filter_0.set_fir_Taps(self.fir_LP)

    def get_algorithm_ID(self):
        return self.algorithm_ID

    def set_algorithm_ID(self, algorithm_ID):
        self.algorithm_ID = algorithm_ID
        self.block_custom_filter_0.set_algorithm_ID(self.algorithm_ID)

    def get_IIR_feedforward_1(self):
        return self.IIR_feedforward_1

    def set_IIR_feedforward_1(self, IIR_feedforward_1):
        self.IIR_feedforward_1 = IIR_feedforward_1
        self.iir_filter_xxx_0_0.set_taps((self.IIR_feedforward_1), (self.IIR_feedback_1))
        self.iir_filter_xxx_0.set_taps((self.IIR_feedforward_1), (self.IIR_feedback_1))

    def get_IIR_feedforward_0(self):
        return self.IIR_feedforward_0

    def set_IIR_feedforward_0(self, IIR_feedforward_0):
        self.IIR_feedforward_0 = IIR_feedforward_0
        self.block_custom_filter_0.set_iir_forward_Taps(self.IIR_feedforward_0)

    def get_IIR_feedback_1(self):
        return self.IIR_feedback_1

    def set_IIR_feedback_1(self, IIR_feedback_1):
        self.IIR_feedback_1 = IIR_feedback_1
        self.iir_filter_xxx_0_0.set_taps((self.IIR_feedforward_1), (self.IIR_feedback_1))
        self.iir_filter_xxx_0.set_taps((self.IIR_feedforward_1), (self.IIR_feedback_1))

    def get_IIR_feedback_0(self):
        return self.IIR_feedback_0

    def set_IIR_feedback_0(self, IIR_feedback_0):
        self.IIR_feedback_0 = IIR_feedback_0
        self.block_custom_filter_0.set_iir_feedback_Taps(self.IIR_feedback_0)


def main(top_block_cls=selector_example, options=None):

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
