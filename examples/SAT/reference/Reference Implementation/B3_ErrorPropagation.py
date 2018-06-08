#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: B3 Aggregation, Decimation, and Error Propagation Example
# Author: rstein
# Description: computes low-pass filter and decimation including computiation/propagation of error estimates. Algorithm ID selects filter implementation (0: FIR-LP, 1: FIR-BP, 2: FIR-CUSTOM, 3: FIR-CUSTOM_FFT, 4: IIR_LP, 5: IIR_HP, 6: IIR-CUSTOM)
# Generated: Fri Aug 25 11:12:16 2017
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
from PyQt4.QtCore import QObject, pyqtSlot
from block_aggregation import block_aggregation  # grc-generated hier_block
from block_error_band import block_error_band  # grc-generated hier_block
from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import qtgui
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from gnuradio.qtgui import Range, RangeWidget
from optparse import OptionParser
import sip
from gnuradio import qtgui


class B3_ErrorPropagation(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "B3 Aggregation, Decimation, and Error Propagation Example")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("B3 Aggregation, Decimation, and Error Propagation Example")
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

        self.settings = Qt.QSettings("GNU Radio", "B3_ErrorPropagation")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 100000
        self.freq_transition = freq_transition = 100
        self.freq_max = freq_max = 100
        self.decimation = decimation = 10
        self.variable_qtgui_label_1 = variable_qtgui_label_1 = 'Algorithm ID and Decimation should (ideally) be setup during startup'
        self.variable_qtgui_label_0_0 = variable_qtgui_label_0_0 = decimation
        self.out_delay = out_delay = 850
        self.input_noise_est = input_noise_est = 0.6
        self.freq_min = freq_min = 10
        self.fir_LP = fir_LP =  firdes.low_pass(1, samp_rate, freq_max, freq_transition, firdes.WIN_HAMMING, 6.76)
        self.filter_taps_b = filter_taps_b = 0.0009959859297622798, -0.003939740084930013, 0.005887765960493963, -0.003939740084930017, 0.0009959859297622804
        self.filter_taps_a = filter_taps_a = 1.0, -3.9692103976755453, 5.9090978836797685, -3.9105406695954064, 0.9706534726794167
        self.filter_taps = filter_taps = 0.0002388461580267176, 0.0002398108917986974, 0.00024103927717078477, 0.00024253396259155124, 0.00024429752374999225, 0.00024633246357552707, 0.00024864121223799884, 0.000251226156251505, 0.0002540895657148212, 0.0002572336816228926, 0.000260660657659173, 0.0002643725892994553, 0.0002683714556042105, 0.00027265920653007925, 0.0002772376174107194, 0.00028210863820277154, 0.0002872738114092499, 0.00029273482505232096, 0.0002984932216349989, 0.0003045504563488066, 0.000310907926177606, 0.0003175669116899371, 0.00032452866435050964, 0.00033179429010488093, 0.00033936486579477787, 0.0003472413227427751, 0.0003554245922714472, 0.00036391543108038604, 0.000372714624973014, 0.0003818227269221097, 0.00039124031900428236, 0.00040096783777698874, 0.0004110056906938553, 0.0004213540814816952, 0.000432013301178813, 0.00044298337888903916, 0.0004542643146123737, 0.0004658560792449862, 0.0004777585272677243, 0.0004899713094346225, 0.0005024942220188677, 0.0005153266829438508, 0.0005284681683406234, 0.0005419181543402374, 0.0005556760588660836, 0.0005697407759726048, 0.0005841115489602089, 0.0005987873882986605, 0.000613767362665385, 0.0006290500750765204, 0.0006446344777941704, 0.0006605190574191511, 0.0006767025333829224, 0.0006931832176633179, 0.0007099595386534929, 0.0007270299247466028, 0.0007443923386745155, 0.0007620451506227255, 0.000779986206907779, 0.0007982135284692049, 0.0008167249034158885, 0.0008355180616490543, 0.0008545908494852483, 0.0008739405311644077, 0.0008935648947954178, 0.0009134612628258765, 0.000933626783080399, 0.0009540588944219053, 0.0009747546864673495, 0.0009957111906260252, 0.0010169253218919039, 0.0010383941698819399, 0.0010601143585518003, 0.0010820826282724738, 0.0011042957194149494, 0.0011267501395195723, 0.0011494423961266875, 0.0011723688803613186, 0.0011955259833484888, 0.001218909863382578, 0.0012425165623426437, 0.0012663424713537097, 0.00129038339946419, 0.0013146355049684644, 0.0013390943640843034, 0.001363756018690765, 0.0013886161614209414, 0.0014136701356619596, 0.0014389140997081995, 0.0014643431641161442, 0.0014899527886882424, 0.001515738433226943, 0.0015416955575346947, 0.0015678192721679807, 0.001594104920513928, 0.001620547496713698, 0.001647141994908452, 0.0016738835256546736, 0.0017007671995088458, 0.0017277876613661647, 0.0017549399053677917, 0.0017822188092395663, 0.001809618785046041, 0.0018371348269283772, 0.0018647615797817707, 0.001892493455670774, 0.001920325099490583, 0.00194825092330575, 0.001976265572011471, 0.002004363341256976, 0.002032538643106818, 0.002060785423964262, 0.002089098561555147, 0.0021174720022827387, 0.002145900158211589, 0.0021743769757449627, 0.002202897099778056, 0.002231454011052847, 0.0022600421216338873, 0.0022886553779244423, 0.002317287726327777, 0.0023459335789084435, 0.0023745866492390633, 0.002403241116553545, 0.0024318909272551537, 0.002460529562085867, 0.0024891512002795935, 0.0025177502539008856, 0.0025463199708610773, 0.0025748545303940773, 0.0026033474132418633, 0.002631793264299631, 0.002660185331478715, 0.002688517328351736, 0.0027167838998138905, 0.002744978293776512, 0.002773093990981579, 0.00280112586915493, 0.0028290671762079, 0.0028569118585437536, 0.002884653862565756, 0.0029122866690158844, 0.0029398049227893353, 0.0029672018717974424, 0.0029944716952741146, 0.0030216083396226168, 0.0030486059840768576, 0.0030754581093788147, 0.0031021591275930405, 0.0031287027522921562, 0.0031550831627100706, 0.003181294770911336, 0.0032073308248072863, 0.003233186202123761, 0.003258854616433382, 0.0032843309454619884, 0.003309608669951558, 0.0033346819691359997, 0.00335954618640244, 0.0033841945696622133, 0.0034086215309798717, 0.003432822646573186, 0.00345679116435349, 0.0034805217292159796, 0.0035040099173784256, 0.003527248976752162, 0.0035502342507243156, 0.0035729603841900826, 0.003595422487705946, 0.003617614973336458, 0.0036395324859768152, 0.003661170369014144, 0.0036825237330049276, 0.0037035869900137186, 0.0037243557162582874, 0.0037448250222951174, 0.003764990484341979, 0.0037848467472940683, 0.003804389387369156, 0.003823614213615656, 0.0038425165694206953, 0.0038610915653407574, 0.0038793354760855436, 0.003897243645042181, 0.003914812114089727, 0.003932036925107241, 0.003948913421481848, 0.0039654383435845375, 0.003981607500463724, 0.003997416701167822, 0.00401286268606782, 0.004027941729873419, 0.004042650572955608, 0.0040569850243628025, 0.004070942290127277, 0.004084519110620022, 0.004097711760550737, 0.0041105174459517, 0.0041229333728551865, 0.004134956281632185, 0.004146583378314972, 0.0041578118689358234, 0.004168638959527016, 0.004179062787443399, 0.0041890800930559635, 0.004198689013719559, 0.004207886755466461, 0.004216671455651522, 0.004225040785968304, 0.004232993349432945, 0.0042405263520777225, 0.004247638862580061, 0.004254329018294811, 0.004260594490915537, 0.004266435280442238, 0.004271848127245903, 0.004276833031326532, 0.004281388595700264, 0.004285513423383236, 0.004289206583052874, 0.00429246760904789, 0.004295295104384422, 0.004297689069062471, 0.004299648106098175, 0.004301172681152821, 0.004302261862903833, 0.004302915185689926, 0.004303133115172386, 0.004302915185689926, 0.004302261862903833, 0.004301172681152821, 0.004299648106098175, 0.004297689069062471, 0.004295295104384422, 0.00429246760904789, 0.004289206583052874, 0.004285513423383236, 0.004281388595700264, 0.004276833031326532, 0.004271848127245903, 0.004266435280442238, 0.004260594490915537, 0.004254329018294811, 0.004247638862580061, 0.0042405263520777225, 0.004232993349432945, 0.004225040785968304, 0.004216671455651522, 0.004207886755466461, 0.004198689013719559, 0.0041890800930559635, 0.004179062787443399, 0.004168638959527016, 0.0041578118689358234, 0.004146583378314972, 0.004134956281632185, 0.0041229333728551865, 0.0041105174459517, 0.004097711760550737, 0.004084519110620022, 0.004070942290127277, 0.0040569850243628025, 0.004042650572955608, 0.004027941729873419, 0.00401286268606782, 0.003997416701167822, 0.003981607500463724, 0.0039654383435845375, 0.003948913421481848, 0.003932036925107241, 0.003914812114089727, 0.003897243645042181, 0.0038793354760855436, 0.0038610915653407574, 0.0038425165694206953, 0.003823614213615656, 0.003804389387369156, 0.0037848467472940683, 0.003764990484341979, 0.0037448250222951174, 0.0037243557162582874, 0.0037035869900137186, 0.0036825237330049276, 0.003661170369014144, 0.0036395324859768152, 0.003617614973336458, 0.003595422487705946, 0.0035729603841900826, 0.0035502342507243156, 0.003527248976752162, 0.0035040099173784256, 0.0034805217292159796, 0.00345679116435349, 0.003432822646573186, 0.0034086215309798717, 0.0033841945696622133, 0.00335954618640244, 0.0033346819691359997, 0.003309608669951558, 0.0032843309454619884, 0.003258854616433382, 0.003233186202123761, 0.0032073308248072863, 0.003181294770911336, 0.0031550831627100706, 0.0031287027522921562, 0.0031021591275930405, 0.0030754581093788147, 0.0030486059840768576, 0.0030216083396226168, 0.0029944716952741146, 0.0029672018717974424, 0.0029398049227893353, 0.0029122866690158844, 0.002884653862565756, 0.0028569118585437536, 0.0028290671762079, 0.00280112586915493, 0.002773093990981579, 0.002744978293776512, 0.0027167838998138905, 0.002688517328351736, 0.002660185331478715, 0.002631793264299631, 0.0026033474132418633, 0.0025748545303940773, 0.0025463199708610773, 0.0025177502539008856, 0.0024891512002795935, 0.002460529562085867, 0.0024318909272551537, 0.002403241116553545, 0.0023745866492390633, 0.0023459335789084435, 0.002317287726327777, 0.0022886553779244423, 0.0022600421216338873, 0.002231454011052847, 0.002202897099778056, 0.0021743769757449627, 0.002145900158211589, 0.0021174720022827387, 0.002089098561555147, 0.002060785423964262, 0.002032538643106818, 0.002004363341256976, 0.001976265572011471, 0.00194825092330575, 0.001920325099490583, 0.001892493455670774, 0.0018647615797817707, 0.0018371348269283772, 0.001809618785046041, 0.0017822188092395663, 0.0017549399053677917, 0.0017277876613661647, 0.0017007671995088458, 0.0016738835256546736, 0.001647141994908452, 0.001620547496713698, 0.001594104920513928, 0.0015678192721679807, 0.0015416955575346947, 0.001515738433226943, 0.0014899527886882424, 0.0014643431641161442, 0.0014389140997081995, 0.0014136701356619596, 0.0013886161614209414, 0.001363756018690765, 0.0013390943640843034, 0.0013146355049684644, 0.00129038339946419, 0.0012663424713537097, 0.0012425165623426437, 0.001218909863382578, 0.0011955259833484888, 0.0011723688803613186, 0.0011494423961266875, 0.0011267501395195723, 0.0011042957194149494, 0.0010820826282724738, 0.0010601143585518003, 0.0010383941698819399, 0.0010169253218919039, 0.0009957111906260252, 0.0009747546864673495, 0.0009540588944219053, 0.000933626783080399, 0.0009134612628258765, 0.0008935648947954178, 0.0008739405311644077, 0.0008545908494852483, 0.0008355180616490543, 0.0008167249034158885, 0.0007982135284692049, 0.000779986206907779, 0.0007620451506227255, 0.0007443923386745155, 0.0007270299247466028, 0.0007099595386534929, 0.0006931832176633179, 0.0006767025333829224, 0.0006605190574191511, 0.0006446344777941704, 0.0006290500750765204, 0.000613767362665385, 0.0005987873882986605, 0.0005841115489602089, 0.0005697407759726048, 0.0005556760588660836, 0.0005419181543402374, 0.0005284681683406234, 0.0005153266829438508, 0.0005024942220188677, 0.0004899713094346225, 0.0004777585272677243, 0.0004658560792449862, 0.0004542643146123737, 0.00044298337888903916, 0.000432013301178813, 0.0004213540814816952, 0.0004110056906938553, 0.00040096783777698874, 0.00039124031900428236, 0.0003818227269221097, 0.000372714624973014, 0.00036391543108038604, 0.0003554245922714472, 0.0003472413227427751, 0.00033936486579477787, 0.00033179429010488093, 0.00032452866435050964, 0.0003175669116899371, 0.000310907926177606, 0.0003045504563488066, 0.0002984932216349989, 0.00029273482505232096, 0.0002872738114092499, 0.00028210863820277154, 0.0002772376174107194, 0.00027265920653007925, 0.0002683714556042105, 0.0002643725892994553, 0.000260660657659173, 0.0002572336816228926, 0.0002540895657148212, 0.000251226156251505, 0.00024864121223799884, 0.00024633246357552707, 0.00024429752374999225, 0.00024253396259155124, 0.00024103927717078477, 0.0002398108917986974, 0.0002388461580267176
        self.algorithm_ID = algorithm_ID = 4

        ##################################################
        # Blocks
        ##################################################
        self.tab = Qt.QTabWidget()
        self.tab_widget_0 = Qt.QWidget()
        self.tab_layout_0 = Qt.QBoxLayout(Qt.QBoxLayout.TopToBottom, self.tab_widget_0)
        self.tab_grid_layout_0 = Qt.QGridLayout()
        self.tab_layout_0.addLayout(self.tab_grid_layout_0)
        self.tab.addTab(self.tab_widget_0, 'Signal')
        self.tab_widget_1 = Qt.QWidget()
        self.tab_layout_1 = Qt.QBoxLayout(Qt.QBoxLayout.TopToBottom, self.tab_widget_1)
        self.tab_grid_layout_1 = Qt.QGridLayout()
        self.tab_layout_1.addLayout(self.tab_grid_layout_1)
        self.tab.addTab(self.tab_widget_1, 'Config')
        self.top_grid_layout.addWidget(self.tab, 0,0,4,4)
        self._out_delay_range = Range(0, 2*samp_rate, 10, 850, 200)
        self._out_delay_win = RangeWidget(self._out_delay_range, self.set_out_delay, "'Out' Path Delay", "counter", int)
        self.tab_grid_layout_1.addWidget(self._out_delay_win, 1,3,1,1)
        self._freq_transition_range = Range(10, 1000, 10, 100, 200)
        self._freq_transition_win = RangeWidget(self._freq_transition_range, self.set_freq_transition, 'Transition Width [Hz]', "counter_slider", float)
        self.tab_grid_layout_1.addWidget(self._freq_transition_win, 0,2,1,2)
        self._freq_min_range = Range(1, 1000, 1, 10, 200)
        self._freq_min_win = RangeWidget(self._freq_min_range, self.set_freq_min, 'Min Frequency [Hz]', "counter_slider", float)
        self.tab_grid_layout_1.addWidget(self._freq_min_win, 0,0,1,2)
        self._freq_max_range = Range(1, 1000, 1, 100, 200)
        self._freq_max_win = RangeWidget(self._freq_max_range, self.set_freq_max, 'Max Frequency [Hz]', "counter_slider", float)
        self.tab_grid_layout_1.addWidget(self._freq_max_win, 1,0,1,2)
        self._algorithm_ID_options = [0, 1, 2, 3, 4, 5, 6]
        self._algorithm_ID_labels = ["0: FIR-LP", "1: FIR-BP", "2: FIR-CUSTOM", "3: FIR-CUSTOM_FFT", "4: IIR_LP", "5: IIR_HP", "6: IIR-CUSTOM"]
        self._algorithm_ID_tool_bar = Qt.QToolBar(self)
        self._algorithm_ID_tool_bar.addWidget(Qt.QLabel('Algorithm ID'+": "))
        self._algorithm_ID_combo_box = Qt.QComboBox()
        self._algorithm_ID_tool_bar.addWidget(self._algorithm_ID_combo_box)
        for label in self._algorithm_ID_labels: self._algorithm_ID_combo_box.addItem(label)
        self._algorithm_ID_callback = lambda i: Qt.QMetaObject.invokeMethod(self._algorithm_ID_combo_box, "setCurrentIndex", Qt.Q_ARG("int", self._algorithm_ID_options.index(i)))
        self._algorithm_ID_callback(self.algorithm_ID)
        self._algorithm_ID_combo_box.currentIndexChanged.connect(
        	lambda i: self.set_algorithm_ID(self._algorithm_ID_options[i]))
        self.top_grid_layout.addWidget(self._algorithm_ID_tool_bar, 4,0,1,1)
        self._input_noise_est_range = Range(0, 100, 0.1, 0.6, 200)
        self._input_noise_est_win = RangeWidget(self._input_noise_est_range, self.set_input_noise_est, 'Input Noise Estimate', "counter", float)
        self.tab_grid_layout_1.addWidget(self._input_noise_est_win, 1,2,1,1)
        self.block_aggregation = block_aggregation(
            algorithm_ID=algorithm_ID,
            decimation1=decimation,
            delay=out_delay,
            fir_Taps=fir_LP,
            freq_max=freq_max,
            freq_min=freq_min,
            freq_transition=freq_transition,
            iir_feedback_Taps=filter_taps_a,
            iir_forward_Taps=filter_taps_b,
            samp_rate=samp_rate,
        )
        self._variable_qtgui_label_1_tool_bar = Qt.QToolBar(self)

        if None:
          self._variable_qtgui_label_1_formatter = None
        else:
          self._variable_qtgui_label_1_formatter = lambda x: x

        self._variable_qtgui_label_1_tool_bar.addWidget(Qt.QLabel('N.B.'+": "))
        self._variable_qtgui_label_1_label = Qt.QLabel(str(self._variable_qtgui_label_1_formatter(self.variable_qtgui_label_1)))
        self._variable_qtgui_label_1_tool_bar.addWidget(self._variable_qtgui_label_1_label)
        self.top_grid_layout.addWidget(self._variable_qtgui_label_1_tool_bar, 4,2,1,2)

        self._variable_qtgui_label_0_0_tool_bar = Qt.QToolBar(self)

        if None:
          self._variable_qtgui_label_0_0_formatter = None
        else:
          self._variable_qtgui_label_0_0_formatter = lambda x: x

        self._variable_qtgui_label_0_0_tool_bar.addWidget(Qt.QLabel('Decimation Factor per Stage'+": "))
        self._variable_qtgui_label_0_0_label = Qt.QLabel(str(self._variable_qtgui_label_0_0_formatter(self.variable_qtgui_label_0_0)))
        self._variable_qtgui_label_0_0_tool_bar.addWidget(self._variable_qtgui_label_0_0_label)
        self.top_grid_layout.addWidget(self._variable_qtgui_label_0_0_tool_bar, 4,1,1,1)

        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_float*1, samp_rate,True)
        self.blocks_multiply_const_vxx_0_1_0_1 = blocks.multiply_const_vff((1, ))
        self.blocks_multiply_const_vxx_0_1_0_0_0 = blocks.multiply_const_vff((1, ))
        self.blocks_multiply_const_vxx_0_1_0_0 = blocks.multiply_const_vff((1, ))
        self.blocks_multiply_const_vxx_0_1_0 = blocks.multiply_const_vff((1, ))
        self.blocks_keep_one_in_n_0_1 = blocks.keep_one_in_n(gr.sizeof_float*1, decimation)
        self.blocks_keep_one_in_n_0_0_0 = blocks.keep_one_in_n(gr.sizeof_float*1, decimation)
        self.blocks_keep_one_in_n_0_0 = blocks.keep_one_in_n(gr.sizeof_float*1, decimation)
        self.blocks_keep_one_in_n_0 = blocks.keep_one_in_n(gr.sizeof_float*1, decimation)
        self.blocks_add_xx_0 = blocks.add_vff(1)
        self.block_error_band_0_0_0 = block_error_band()
        self.block_error_band_0_0 = block_error_band()
        self.block_error_band_0 = block_error_band()
        self.block_aggregation_0_0 = block_aggregation(
            algorithm_ID=algorithm_ID,
            decimation1=decimation,
            delay=out_delay/decimation/decimation,
            fir_Taps=fir_LP,
            freq_max=freq_max,
            freq_min=freq_min,
            freq_transition=freq_transition,
            iir_feedback_Taps=filter_taps_a,
            iir_forward_Taps=filter_taps_b,
            samp_rate=samp_rate/(decimation*decimation),
        )
        self.block_aggregation_0 = block_aggregation(
            algorithm_ID=algorithm_ID,
            decimation1=decimation,
            delay=out_delay/decimation,
            fir_Taps=fir_LP,
            freq_max=freq_max,
            freq_min=freq_min,
            freq_transition=freq_transition,
            iir_feedback_Taps=filter_taps_a,
            iir_forward_Taps=filter_taps_b,
            samp_rate=samp_rate/decimation,
        )
        self.analog_sig_source_x_0 = analog.sig_source_f(samp_rate, analog.GR_SQR_WAVE, 2, 1, 0)
        self.analog_noise_source_x_0 = analog.noise_source_f(analog.GR_UNIFORM, 1, 0)
        self.analog_const_source_x_0 = analog.sig_source_f(0, analog.GR_CONST_WAVE, 0, 0, input_noise_est)
        self.Aggregation_Sample_Example_0_0 = qtgui.time_sink_f(
        	samp_rate/(decimation*decimation*decimation), #size
        	samp_rate/(decimation*decimation*decimation), #samp_rate
        	"", #name
        	4 #number of inputs
        )
        self.Aggregation_Sample_Example_0_0.set_update_time(0.1)
        self.Aggregation_Sample_Example_0_0.set_y_axis(-0.2, 1.7)

        self.Aggregation_Sample_Example_0_0.set_y_label('Signal @ 100 Hz', "a.u.")

        self.Aggregation_Sample_Example_0_0.enable_tags(-1, False)
        self.Aggregation_Sample_Example_0_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.Aggregation_Sample_Example_0_0.enable_autoscale(False)
        self.Aggregation_Sample_Example_0_0.enable_grid(True)
        self.Aggregation_Sample_Example_0_0.enable_axis_labels(True)
        self.Aggregation_Sample_Example_0_0.enable_control_panel(False)

        if not True:
          self.Aggregation_Sample_Example_0_0.disable_legend()

        labels = ['est: y(t) + sigma(t)', 'est: y(t)', 'est: y(t) - sigma(t)', 'true y(t)', 'true y(t)',
                  '', '', '', '', '']
        widths = [1, 2, 1, 2, 2,
                  1, 1, 1, 1, 1]
        colors = ["red", "red", "red", "green", "green",
                  "magenta", "yellow", "dark red", "dark green", "blue"]
        styles = [2, 1, 2, 1, 1,
                  1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
                   -1, -1, -1, -1, -1]
        alphas = [1.0, 1.0, 1.0, 1.0, 1,
                  1.0, 1.0, 1.0, 1.0, 1.0]

        for i in xrange(4):
            if len(labels[i]) == 0:
                self.Aggregation_Sample_Example_0_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.Aggregation_Sample_Example_0_0.set_line_label(i, labels[i])
            self.Aggregation_Sample_Example_0_0.set_line_width(i, widths[i])
            self.Aggregation_Sample_Example_0_0.set_line_color(i, colors[i])
            self.Aggregation_Sample_Example_0_0.set_line_style(i, styles[i])
            self.Aggregation_Sample_Example_0_0.set_line_marker(i, markers[i])
            self.Aggregation_Sample_Example_0_0.set_line_alpha(i, alphas[i])

        self._Aggregation_Sample_Example_0_0_win = sip.wrapinstance(self.Aggregation_Sample_Example_0_0.pyqwidget(), Qt.QWidget)
        self.tab_grid_layout_0.addWidget(self._Aggregation_Sample_Example_0_0_win, 4,0,2,4)
        self.Aggregation_Sample_Example_0 = qtgui.time_sink_f(
        	samp_rate/(decimation*decimation), #size
        	samp_rate/(decimation*decimation), #samp_rate
        	"", #name
        	4 #number of inputs
        )
        self.Aggregation_Sample_Example_0.set_update_time(0.1)
        self.Aggregation_Sample_Example_0.set_y_axis(-0.2, 1.7)

        self.Aggregation_Sample_Example_0.set_y_label('Signal @ 1 kHz', "a.u.")

        self.Aggregation_Sample_Example_0.enable_tags(-1, False)
        self.Aggregation_Sample_Example_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.Aggregation_Sample_Example_0.enable_autoscale(False)
        self.Aggregation_Sample_Example_0.enable_grid(True)
        self.Aggregation_Sample_Example_0.enable_axis_labels(True)
        self.Aggregation_Sample_Example_0.enable_control_panel(False)

        if not True:
          self.Aggregation_Sample_Example_0.disable_legend()

        labels = ['est: y(t) + sigma(t)', 'est: y(t)', 'est: y(t) - sigma(t)', 'true y(t)', 'true y(t)',
                  '', '', '', '', '']
        widths = [1, 2, 1, 2, 2,
                  1, 1, 1, 1, 1]
        colors = ["red", "red", "red", "green", "green",
                  "magenta", "yellow", "dark red", "dark green", "blue"]
        styles = [2, 1, 2, 1, 1,
                  1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
                   -1, -1, -1, -1, -1]
        alphas = [1.0, 1.0, 1.0, 1.0, 1,
                  1.0, 1.0, 1.0, 1.0, 1.0]

        for i in xrange(4):
            if len(labels[i]) == 0:
                self.Aggregation_Sample_Example_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.Aggregation_Sample_Example_0.set_line_label(i, labels[i])
            self.Aggregation_Sample_Example_0.set_line_width(i, widths[i])
            self.Aggregation_Sample_Example_0.set_line_color(i, colors[i])
            self.Aggregation_Sample_Example_0.set_line_style(i, styles[i])
            self.Aggregation_Sample_Example_0.set_line_marker(i, markers[i])
            self.Aggregation_Sample_Example_0.set_line_alpha(i, alphas[i])

        self._Aggregation_Sample_Example_0_win = sip.wrapinstance(self.Aggregation_Sample_Example_0.pyqwidget(), Qt.QWidget)
        self.tab_grid_layout_0.addWidget(self._Aggregation_Sample_Example_0_win, 2,0,2,4)
        self.Aggregation_Sample_Example = qtgui.time_sink_f(
        	samp_rate/decimation, #size
        	samp_rate/decimation, #samp_rate
        	"", #name
        	5 #number of inputs
        )
        self.Aggregation_Sample_Example.set_update_time(0.1)
        self.Aggregation_Sample_Example.set_y_axis(-0.2, 1.7)

        self.Aggregation_Sample_Example.set_y_label('Signal @ 10 kHz', "a.u.")

        self.Aggregation_Sample_Example.enable_tags(-1, False)
        self.Aggregation_Sample_Example.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.Aggregation_Sample_Example.enable_autoscale(False)
        self.Aggregation_Sample_Example.enable_grid(True)
        self.Aggregation_Sample_Example.enable_axis_labels(True)
        self.Aggregation_Sample_Example.enable_control_panel(False)

        if not True:
          self.Aggregation_Sample_Example.disable_legend()

        labels = ['raw y(t)', 'est: y(t) + sigma(t)', 'est: y(t)', 'est: y(t) - sigma(t)', 'true y(t)',
                  '', '', '', '', '']
        widths = [1, 1, 1, 1, 2,
                  1, 1, 1, 1, 1]
        colors = ["blue", "red", "red", "red", "green",
                  "magenta", "yellow", "dark red", "dark green", "blue"]
        styles = [3, 2, 1, 2, 1,
                  1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
                   -1, -1, -1, -1, -1]
        alphas = [1.0, 1.0, 1.0, 1.0, 1,
                  1.0, 1.0, 1.0, 1.0, 1.0]

        for i in xrange(5):
            if len(labels[i]) == 0:
                self.Aggregation_Sample_Example.set_line_label(i, "Data {0}".format(i))
            else:
                self.Aggregation_Sample_Example.set_line_label(i, labels[i])
            self.Aggregation_Sample_Example.set_line_width(i, widths[i])
            self.Aggregation_Sample_Example.set_line_color(i, colors[i])
            self.Aggregation_Sample_Example.set_line_style(i, styles[i])
            self.Aggregation_Sample_Example.set_line_marker(i, markers[i])
            self.Aggregation_Sample_Example.set_line_alpha(i, alphas[i])

        self._Aggregation_Sample_Example_win = sip.wrapinstance(self.Aggregation_Sample_Example.pyqwidget(), Qt.QWidget)
        self.tab_grid_layout_0.addWidget(self._Aggregation_Sample_Example_win, 0,0,2,4)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_const_source_x_0, 0), (self.block_aggregation, 1))
        self.connect((self.analog_noise_source_x_0, 0), (self.blocks_add_xx_0, 0))
        self.connect((self.analog_sig_source_x_0, 0), (self.blocks_add_xx_0, 1))
        self.connect((self.analog_sig_source_x_0, 0), (self.blocks_keep_one_in_n_0, 0))
        self.connect((self.block_aggregation, 0), (self.block_aggregation_0, 0))
        self.connect((self.block_aggregation, 1), (self.block_aggregation_0, 1))
        self.connect((self.block_aggregation, 0), (self.block_error_band_0, 0))
        self.connect((self.block_aggregation, 1), (self.blocks_multiply_const_vxx_0_1_0, 0))
        self.connect((self.block_aggregation_0, 0), (self.block_aggregation_0_0, 0))
        self.connect((self.block_aggregation_0, 1), (self.block_aggregation_0_0, 1))
        self.connect((self.block_aggregation_0, 0), (self.block_error_band_0_0, 0))
        self.connect((self.block_aggregation_0, 1), (self.blocks_multiply_const_vxx_0_1_0_0, 0))
        self.connect((self.block_aggregation_0_0, 0), (self.block_error_band_0_0_0, 0))
        self.connect((self.block_aggregation_0_0, 1), (self.blocks_multiply_const_vxx_0_1_0_0_0, 0))
        self.connect((self.block_error_band_0, 0), (self.Aggregation_Sample_Example, 1))
        self.connect((self.block_error_band_0, 2), (self.Aggregation_Sample_Example, 3))
        self.connect((self.block_error_band_0, 1), (self.Aggregation_Sample_Example, 2))
        self.connect((self.block_error_band_0_0, 0), (self.Aggregation_Sample_Example_0, 0))
        self.connect((self.block_error_band_0_0, 2), (self.Aggregation_Sample_Example_0, 2))
        self.connect((self.block_error_band_0_0, 1), (self.Aggregation_Sample_Example_0, 1))
        self.connect((self.block_error_band_0_0_0, 0), (self.Aggregation_Sample_Example_0_0, 0))
        self.connect((self.block_error_band_0_0_0, 2), (self.Aggregation_Sample_Example_0_0, 2))
        self.connect((self.block_error_band_0_0_0, 1), (self.Aggregation_Sample_Example_0_0, 1))
        self.connect((self.blocks_add_xx_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.blocks_keep_one_in_n_0, 0), (self.Aggregation_Sample_Example, 4))
        self.connect((self.blocks_keep_one_in_n_0, 0), (self.blocks_keep_one_in_n_0_0, 0))
        self.connect((self.blocks_keep_one_in_n_0_0, 0), (self.Aggregation_Sample_Example_0, 3))
        self.connect((self.blocks_keep_one_in_n_0_0, 0), (self.blocks_keep_one_in_n_0_0_0, 0))
        self.connect((self.blocks_keep_one_in_n_0_0_0, 0), (self.Aggregation_Sample_Example_0_0, 3))
        self.connect((self.blocks_keep_one_in_n_0_1, 0), (self.Aggregation_Sample_Example, 0))
        self.connect((self.blocks_multiply_const_vxx_0_1_0, 0), (self.block_error_band_0, 1))
        self.connect((self.blocks_multiply_const_vxx_0_1_0_0, 0), (self.block_error_band_0_0, 1))
        self.connect((self.blocks_multiply_const_vxx_0_1_0_0_0, 0), (self.block_error_band_0_0_0, 1))
        self.connect((self.blocks_multiply_const_vxx_0_1_0_1, 0), (self.blocks_keep_one_in_n_0_1, 0))
        self.connect((self.blocks_throttle_0, 0), (self.block_aggregation, 0))
        self.connect((self.blocks_throttle_0, 0), (self.blocks_multiply_const_vxx_0_1_0_1, 0))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "B3_ErrorPropagation")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_fir_LP( firdes.low_pass(1, self.samp_rate, self.freq_max, self.freq_transition, firdes.WIN_HAMMING, 6.76))
        self.block_aggregation.set_samp_rate(self.samp_rate)
        self.blocks_throttle_0.set_sample_rate(self.samp_rate)
        self.block_aggregation_0_0.set_samp_rate(self.samp_rate/(self.decimation*self.decimation))
        self.block_aggregation_0.set_samp_rate(self.samp_rate/self.decimation)
        self.analog_sig_source_x_0.set_sampling_freq(self.samp_rate)
        self.Aggregation_Sample_Example_0_0.set_samp_rate(self.samp_rate/(self.decimation*self.decimation*self.decimation))
        self.Aggregation_Sample_Example_0.set_samp_rate(self.samp_rate/(self.decimation*self.decimation))
        self.Aggregation_Sample_Example.set_samp_rate(self.samp_rate/self.decimation)

    def get_freq_transition(self):
        return self.freq_transition

    def set_freq_transition(self, freq_transition):
        self.freq_transition = freq_transition
        self.set_fir_LP( firdes.low_pass(1, self.samp_rate, self.freq_max, self.freq_transition, firdes.WIN_HAMMING, 6.76))
        self.block_aggregation.set_freq_transition(self.freq_transition)
        self.block_aggregation_0_0.set_freq_transition(self.freq_transition)
        self.block_aggregation_0.set_freq_transition(self.freq_transition)

    def get_freq_max(self):
        return self.freq_max

    def set_freq_max(self, freq_max):
        self.freq_max = freq_max
        self.set_fir_LP( firdes.low_pass(1, self.samp_rate, self.freq_max, self.freq_transition, firdes.WIN_HAMMING, 6.76))
        self.block_aggregation.set_freq_max(self.freq_max)
        self.block_aggregation_0_0.set_freq_max(self.freq_max)
        self.block_aggregation_0.set_freq_max(self.freq_max)

    def get_decimation(self):
        return self.decimation

    def set_decimation(self, decimation):
        self.decimation = decimation
        self.block_aggregation.set_decimation1(self.decimation)
        self.set_variable_qtgui_label_0_0(self._variable_qtgui_label_0_0_formatter(self.decimation))
        self.blocks_keep_one_in_n_0_1.set_n(self.decimation)
        self.blocks_keep_one_in_n_0_0_0.set_n(self.decimation)
        self.blocks_keep_one_in_n_0_0.set_n(self.decimation)
        self.blocks_keep_one_in_n_0.set_n(self.decimation)
        self.block_aggregation_0_0.set_decimation1(self.decimation)
        self.block_aggregation_0_0.set_delay(self.out_delay/self.decimation/self.decimation)
        self.block_aggregation_0_0.set_samp_rate(self.samp_rate/(self.decimation*self.decimation))
        self.block_aggregation_0.set_decimation1(self.decimation)
        self.block_aggregation_0.set_delay(self.out_delay/self.decimation)
        self.block_aggregation_0.set_samp_rate(self.samp_rate/self.decimation)
        self.Aggregation_Sample_Example_0_0.set_samp_rate(self.samp_rate/(self.decimation*self.decimation*self.decimation))
        self.Aggregation_Sample_Example_0.set_samp_rate(self.samp_rate/(self.decimation*self.decimation))
        self.Aggregation_Sample_Example.set_samp_rate(self.samp_rate/self.decimation)

    def get_variable_qtgui_label_1(self):
        return self.variable_qtgui_label_1

    def set_variable_qtgui_label_1(self, variable_qtgui_label_1):
        self.variable_qtgui_label_1 = variable_qtgui_label_1
        Qt.QMetaObject.invokeMethod(self._variable_qtgui_label_1_label, "setText", Qt.Q_ARG("QString", str(self.variable_qtgui_label_1)))

    def get_variable_qtgui_label_0_0(self):
        return self.variable_qtgui_label_0_0

    def set_variable_qtgui_label_0_0(self, variable_qtgui_label_0_0):
        self.variable_qtgui_label_0_0 = variable_qtgui_label_0_0
        Qt.QMetaObject.invokeMethod(self._variable_qtgui_label_0_0_label, "setText", Qt.Q_ARG("QString", str(self.variable_qtgui_label_0_0)))

    def get_out_delay(self):
        return self.out_delay

    def set_out_delay(self, out_delay):
        self.out_delay = out_delay
        self.block_aggregation.set_delay(self.out_delay)
        self.block_aggregation_0_0.set_delay(self.out_delay/self.decimation/self.decimation)
        self.block_aggregation_0.set_delay(self.out_delay/self.decimation)

    def get_input_noise_est(self):
        return self.input_noise_est

    def set_input_noise_est(self, input_noise_est):
        self.input_noise_est = input_noise_est
        self.analog_const_source_x_0.set_offset(self.input_noise_est)

    def get_freq_min(self):
        return self.freq_min

    def set_freq_min(self, freq_min):
        self.freq_min = freq_min
        self.block_aggregation.set_freq_min(self.freq_min)
        self.block_aggregation_0_0.set_freq_min(self.freq_min)
        self.block_aggregation_0.set_freq_min(self.freq_min)

    def get_fir_LP(self):
        return self.fir_LP

    def set_fir_LP(self, fir_LP):
        self.fir_LP = fir_LP
        self.block_aggregation.set_fir_Taps(self.fir_LP)
        self.block_aggregation_0_0.set_fir_Taps(self.fir_LP)
        self.block_aggregation_0.set_fir_Taps(self.fir_LP)

    def get_filter_taps_b(self):
        return self.filter_taps_b

    def set_filter_taps_b(self, filter_taps_b):
        self.filter_taps_b = filter_taps_b
        self.block_aggregation.set_iir_forward_Taps(self.filter_taps_b)
        self.block_aggregation_0_0.set_iir_forward_Taps(self.filter_taps_b)
        self.block_aggregation_0.set_iir_forward_Taps(self.filter_taps_b)

    def get_filter_taps_a(self):
        return self.filter_taps_a

    def set_filter_taps_a(self, filter_taps_a):
        self.filter_taps_a = filter_taps_a
        self.block_aggregation.set_iir_feedback_Taps(self.filter_taps_a)
        self.block_aggregation_0_0.set_iir_feedback_Taps(self.filter_taps_a)
        self.block_aggregation_0.set_iir_feedback_Taps(self.filter_taps_a)

    def get_filter_taps(self):
        return self.filter_taps

    def set_filter_taps(self, filter_taps):
        self.filter_taps = filter_taps

    def get_algorithm_ID(self):
        return self.algorithm_ID

    def set_algorithm_ID(self, algorithm_ID):
        self.algorithm_ID = algorithm_ID
        self._algorithm_ID_callback(self.algorithm_ID)
        self.block_aggregation.set_algorithm_ID(self.algorithm_ID)
        self.block_aggregation_0_0.set_algorithm_ID(self.algorithm_ID)
        self.block_aggregation_0.set_algorithm_ID(self.algorithm_ID)


def main(top_block_cls=B3_ErrorPropagation, options=None):

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
