#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Trigger Sender
# Generated: Tue May  8 15:03:59 2018
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
from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import qtgui
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import digitizers
import pmt
import sip
import sys
from gnuradio import qtgui


class trigger_sender(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Trigger Sender")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Trigger Sender")
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

        self.settings = Qt.QSettings("GNU Radio", "trigger_sender")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())


        ##################################################
        # Variables
        ##################################################
        self.wr_tag = wr_tag = gr.tag_utils.python_to_tag((10000, pmt.intern("wr_event"), pmt.make_tuple(pmt.string_to_symbol('foo'), pmt.from_uint64(2000000000), pmt.from_uint64(1900000000), pmt.from_bool(False), pmt.from_bool(True)), pmt.intern("src")))
        self.trigger_tag = trigger_tag = gr.tag_utils.python_to_tag((500, pmt.intern("trigger"), pmt.make_tuple(), pmt.intern("src")))
        self.send_udp = send_udp = True
        self.samples = samples = 1000
        self.samp_rate = samp_rate = 100000
        self.pre_samples2 = pre_samples2 = 100
        self.pre_samples = pre_samples = 100

        ##################################################
        # Blocks
        ##################################################
        _send_udp_check_box = Qt.QCheckBox('Send UDP?')
        self._send_udp_choices = {True: True, False: False}
        self._send_udp_choices_inv = dict((v,k) for k,v in self._send_udp_choices.iteritems())
        self._send_udp_callback = lambda i: Qt.QMetaObject.invokeMethod(_send_udp_check_box, "setChecked", Qt.Q_ARG("bool", self._send_udp_choices_inv[i]))
        self._send_udp_callback(self.send_udp)
        _send_udp_check_box.stateChanged.connect(lambda i: self.set_send_udp(self._send_udp_choices[bool(i)]))
        self.top_grid_layout.addWidget(_send_udp_check_box)
        self.qtgui_time_sink_x_0_0 = qtgui.time_sink_f(
        	samples, #size
        	samp_rate, #samp_rate
        	'Triggered Time-Domain', #name
        	2 #number of inputs
        )
        self.qtgui_time_sink_x_0_0.set_update_time(0.10)
        self.qtgui_time_sink_x_0_0.set_y_axis(-2, 2)

        self.qtgui_time_sink_x_0_0.set_y_label('Amplitude', "")

        self.qtgui_time_sink_x_0_0.enable_tags(-1, True)
        self.qtgui_time_sink_x_0_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 1, 'acq_info')
        self.qtgui_time_sink_x_0_0.enable_autoscale(False)
        self.qtgui_time_sink_x_0_0.enable_grid(False)
        self.qtgui_time_sink_x_0_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0_0.enable_control_panel(False)
        self.qtgui_time_sink_x_0_0.enable_stem_plot(False)

        if not True:
          self.qtgui_time_sink_x_0_0.disable_legend()

        labels = ['primary signal (UDP source)', 'secondary signal (UDP sink)', '', '', '',
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

        for i in xrange(2):
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
        self.top_grid_layout.addWidget(self._qtgui_time_sink_x_0_0_win, 0, 1, 1, 1)
        for r in range(0, 1):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(1, 2):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.qtgui_time_sink_x_0 = qtgui.time_sink_f(
        	samp_rate//2, #size
        	samp_rate, #samp_rate
        	'Continuous Time-Domain', #name
        	2 #number of inputs
        )
        self.qtgui_time_sink_x_0.set_update_time(0.10)
        self.qtgui_time_sink_x_0.set_y_axis(-2, 2)

        self.qtgui_time_sink_x_0.set_y_label('Amplitude', "")

        self.qtgui_time_sink_x_0.enable_tags(-1, True)
        self.qtgui_time_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 1, '')
        self.qtgui_time_sink_x_0.enable_autoscale(False)
        self.qtgui_time_sink_x_0.enable_grid(False)
        self.qtgui_time_sink_x_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0.enable_control_panel(False)
        self.qtgui_time_sink_x_0.enable_stem_plot(False)

        if not True:
          self.qtgui_time_sink_x_0.disable_legend()

        labels = ['raw y(t)', 'edge detect y(t)', '', '', '',
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

        for i in xrange(2):
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
        self.digitizers_edge_trigger_receiver_f_0 = digitizers.edge_trigger_receiver_f(2025)
        self.digitizers_edge_trigger_ff_0 = digitizers.edge_trigger_ff(samp_rate, 0.3, 0.5, 0.0, True, "localhost:2025, 127.0.0.1:2026", True)
        self.digitizers_demux_ff_0_0 = digitizers.demux_ff(samp_rate, 50000, samples-pre_samples, pre_samples)
        self.digitizers_demux_ff_0 = digitizers.demux_ff(samp_rate, 50000, samples-pre_samples2, pre_samples2)
        self.blocks_vector_source_x_0 = blocks.vector_source_f(([0] *samp_rate) , True, 1, [trigger_tag, wr_tag])
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_float*1, samp_rate,True)
        self.blocks_tag_debug_0 = blocks.tag_debug(gr.sizeof_float*1, '', ""); self.blocks_tag_debug_0.set_display(False)
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_vff((0.3, ))
        self.blocks_delay_0 = blocks.delay(gr.sizeof_float*1, 10)
        self.blocks_add_xx_1 = blocks.add_vff(1)
        self.blocks_add_xx_0 = blocks.add_vff(1)
        self.analog_sig_source_x_0 = analog.sig_source_f(samp_rate, analog.GR_SQR_WAVE, 2, 1, 0)
        self.analog_fastnoise_source_x_0 = analog.fastnoise_source_f(analog.GR_GAUSSIAN, 0.04, 0, 8192)



        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_fastnoise_source_x_0, 0), (self.blocks_add_xx_0, 0))
        self.connect((self.analog_sig_source_x_0, 0), (self.blocks_add_xx_0, 1))
        self.connect((self.blocks_add_xx_0, 0), (self.blocks_multiply_const_vxx_0, 0))
        self.connect((self.blocks_add_xx_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.blocks_add_xx_0, 0), (self.digitizers_edge_trigger_ff_0, 0))
        self.connect((self.blocks_add_xx_1, 0), (self.digitizers_demux_ff_0, 0))
        self.connect((self.blocks_delay_0, 0), (self.blocks_add_xx_1, 0))
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.blocks_delay_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.qtgui_time_sink_x_0, 0))
        self.connect((self.blocks_vector_source_x_0, 0), (self.blocks_add_xx_0, 2))
        self.connect((self.digitizers_demux_ff_0, 0), (self.qtgui_time_sink_x_0_0, 1))
        self.connect((self.digitizers_demux_ff_0_0, 0), (self.qtgui_time_sink_x_0_0, 0))
        self.connect((self.digitizers_edge_trigger_ff_0, 0), (self.blocks_tag_debug_0, 0))
        self.connect((self.digitizers_edge_trigger_ff_0, 0), (self.digitizers_demux_ff_0_0, 0))
        self.connect((self.digitizers_edge_trigger_ff_0, 0), (self.qtgui_time_sink_x_0, 1))
        self.connect((self.digitizers_edge_trigger_receiver_f_0, 0), (self.blocks_add_xx_1, 1))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "trigger_sender")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_wr_tag(self):
        return self.wr_tag

    def set_wr_tag(self, wr_tag):
        self.wr_tag = wr_tag
        self.blocks_vector_source_x_0.set_data(([0] *self.samp_rate) , [self.trigger_tag, self.wr_tag])

    def get_trigger_tag(self):
        return self.trigger_tag

    def set_trigger_tag(self, trigger_tag):
        self.trigger_tag = trigger_tag
        self.blocks_vector_source_x_0.set_data(([0] *self.samp_rate) , [self.trigger_tag, self.wr_tag])

    def get_send_udp(self):
        return self.send_udp

    def set_send_udp(self, send_udp):
        self.send_udp = send_udp
        self._send_udp_callback(self.send_udp)

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
        self.blocks_vector_source_x_0.set_data(([0] *self.samp_rate) , [self.trigger_tag, self.wr_tag])
        self.blocks_throttle_0.set_sample_rate(self.samp_rate)
        self.analog_sig_source_x_0.set_sampling_freq(self.samp_rate)

    def get_pre_samples2(self):
        return self.pre_samples2

    def set_pre_samples2(self, pre_samples2):
        self.pre_samples2 = pre_samples2

    def get_pre_samples(self):
        return self.pre_samples

    def set_pre_samples(self, pre_samples):
        self.pre_samples = pre_samples


def main(top_block_cls=trigger_sender, options=None):

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
