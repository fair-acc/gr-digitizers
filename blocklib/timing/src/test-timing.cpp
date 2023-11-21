#include <format>
#include <thread>
#include <iostream>
#include <cstdio>

// CLI - interface
#include <CLI/CLI.hpp>

// UI
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include "ImScoped.hpp"

#include <fmt/chrono.h>
#include <fmt/ranges.h>

#include "timing.hpp"
#include "ps4000a.hpp"
#include "fair_header.h"
#include "plot.hpp"

static const std::array<ImColor, 13> bpcidColors{
    // colors taken from: https://git.acc.gsi.de/fcc-applications/common-context-widget-reactor/src/branch/master/common-context-widget-fx/src/main/resources/de/gsi/fcc/applications/common/contextwidget/fx/StylesColorsBase.css
    ImColor{0x00, 0x70, 0xC0}, // 0x0070C0 - materials
    ImColor{0x00, 0xB0, 0x50}, // 0x00B050 - bio
    ImColor{0x5C, 0xB7, 0xC3}, // 0x5CB7C3 - cbm
    ImColor{0x70, 0x30, 0xA0}, // 0x7030A0 - nustar-basic
    ImColor{0xC6, 0x59, 0x11}, // 0xC65911 - machine
    ImColor{0xFF, 0xDA, 0x4A}, // 0xFFDA4A - nustar-r3b
    ImColor{0xC4, 0x8B, 0xD0}, // 0xC48BD0 - nustar-she-c
    ImColor{0xE1, 0x4C, 0xFF}, // 0xE14CFF - nustar-she-p
    ImColor{0xFF, 0xFF, 0x00}, // 0xFFFF00 - nustar-hades
    ImColor{0xFF, 0xA6, 0x32}, // 0xFFA632 - plasma
    ImColor{0x00, 0xFC, 0xD6}, // 0x00FCD6 - appa-pp
    ImColor{0x80, 0x80, 0x80}, // 0x808080 - undefined
    ImColor{0x9e, 0x9e, 0x9e}  // 0x9e9e9e - invalid
};

// generic colormap for timing ids
static std::array<ImColor, 20> colorlist { // gemerated with http://medialab.github.io/iwanthue/
    ImColor{78,172,215,120}, ImColor{200,76,41,120}, ImColor{85,199,102,120}, ImColor{186,84,191,120},
    ImColor{101,173,51,120}, ImColor{117,98,204,120}, ImColor{169,180,56,120}, ImColor{211,76,146,120},
    ImColor{71,136,57,120}, ImColor{209,69,88,120}, ImColor{86,192,158,120}, ImColor{217,132,45,120},
    ImColor{102,125,198,120}, ImColor{201,160,65,120}, ImColor{199,135,198,120}, ImColor{156,176,104,120},
    ImColor{168,85,112,120}, ImColor{55,132,95,120}, ImColor{203,126,93,120}, ImColor{118,109,41,120},
};

// https://www-acc.gsi.de/wiki/Timing/TimingSystemGroupsAndMachines#Groups_for_Operation
static const std::map<uint16_t, std::pair<std::string, std::string>> timingGroupTable{
        {200, {"YRT1_TO_YRT1LQ1 ", "from ion source 1 to merging quadrupole"}},
        {201, {"YRT1LQ1_TO_YRT1LC1 ", "from merging quadrupole at ion sources to chopper"}},
        {202, {"YRT1LC1_TO_YRT1MH2 ", "injector from chopper to merging dipole"}},
        {203, {"YRT1MH2_TO_CRYRING ", "from merging dipole to ring"}},
        {210, {"CRYRING_RING ", "CRYRING ring"}},
        {211, {"CRYRING_COOLER ", "CRYRING electron cooler"}},
        {212, {"CRYRING_TO_YRE ", "Experiment line from CRYRING to YRE"}},
        {300, {"SIS18_RING ", "SIS18 ring"}},
        {301, {"SIS18_COOLER ", "SIS18 electron cooler"}},
        {310, {"SIS100_RING ", "SIS100 ring"}},
        {340, {"ESR_RING ", "ESR ring"}},
        {341, {"ESR_COOLER ", "ESR electron cooler"}},
        {345, {"ESR_TO_HTR ", "ESR to HITRAP"}},
        {350, {"CR_RING ", "CR ring"}},
        {360, {"HESR_RING ", "HESR ring"}},
        {400, {"TO_BE_DISCUSSED ", "PLINAC proton source"}},
        {448, {"PZU_QR ", "UNILAC Timing - Source Right"}},
        {449, {"PZU_QL ", "UNILAC Timing - Source Left"}},
        {450, {"PZU_QN ", "UNILAC Timing - Source High Charge State Injector (HLI)"}},
        {451, {"PZU_UN ", "UNILAC Timing - High Charge State Injector (HLI)"}},
        {452, {"PZU_UH ", "UNILAC Timing - High Current Injector (HSI)"}},
        {453, {"PZU_AT ", "UNILAC Timing - Alvarez Cavities"}},
        {454, {"PZU_TK ", "UNILAC Timing - Transfer Line"}},
        {464, {"GTK7BC1L_TO_PLTKMH2__GS ", "GTK7BC1L to PLTKMH2 - shared group controlled by SIS18 accelerator"}},
        {465, {"GTK7BC1L_TO_PLTKMH2__GU ", "GTK7BC1L to PLTKMH2 - shared group controlled by UNILAC accelerator"}},
        {466, {"PLTKMH2_TO_SIS18__GS ", "PLTKMH2 to SIS18 - shared group controlled by SIS18 accelerator"}},
        {467, {"PLTKMH2_TO_SIS18__GU ", "PLTKMH2 to SIS18 - shared group controlled by UNILAC accelerator"}},
        {500, {"SIS18_TO_GTS1MU1 ", "SIS extraction to GTS1MU1"}},
        {501, {"GTS1MU1_TO_GTE3MU1 ", "GTS1MU1 to GTE3MU1"}},
        {502, {"GTE3MU1_TO_GTS6MU1 ", "GTE3MU1 to GTS6MU1"}},
        {503, {"GTS6MU1_TO_GTS6MU1 ", "GTS6MU1_TO_GTS6MU1"}},
        {504, {"GTS6MU1_TO_ESR ", "GTS6MU1 to ESR"}},
        {505, {"GTS1MU1_TO_GTS3MU1 ", "GTS1MU1 to GTS3MU1"}},
        {506, {"GTS3MU1_TO_HHD ", "GTS3MU1 to HHD Dump"}},
        {507, {"GTS3MU1_TO_GHFSMU1 ", "GTS3MU1 to GHFSMU1"}},
        {508, {"GHFSMU1_TO_HFS ", "Experiment line to HFS"}},
        {509, {"GHFSMU1_TO_GTS6MU1 ", "GHFSMU1 to GTS6MU1"}},
        {510, {"GTS6MU1_TO_GTS7MU1 ", "GTS6MU1 to GTS7MU1"}},
        {511, {"GTE3MU1_TO_GHHTMU1 ", "GTE3MU1 to GHHTMU1"}},
        {512, {"GHHTMU1_TO_HHT ", "Experiment line to the high temperature experiment HHT"}},
        {513, {"GHHTMU1_TO_GTH3MU1 ", "GHHTMU1 to GTH3MU1"}},
        {514, {"GTH3MU1_TO_GTP1MU1 ", "GTH3MU1 to pion branch GTV2MU3"}},
        {515, {"GTP1MU1_TO_HADES ", "Experiment line to the electron-positron-detector HADES (High Acceptance Di-Electron Spectrometer)"}},
        {516, {"GTH3MU1_TO_GTS7MU1 ", "GTH3MU1 to GTS7MU1"}},
        {517, {"GTS7MU1_TO_GTH4MU1 ", "GTS7MU1 to GTH4MU1"}},
        {518, {"GTH4MU1_TO_HTM ", "Experiment line to the particle therapy experiment HTM"}},
        {519, {"GTH4MU1_TO_GTH4MU2 ", "GTH4MU1 to GTH4MU2"}},
        {520, {"GTP1MU1_TO_GTH4MU2 ", "GTP1MU1 to GTH4MU2 pion transfer line"}},
        {521, {"GTH4MU2_TO_GTV1MU1 ", "GTH4MU2 to GTV1MU1"}},
        {522, {"GTV1MU1_TO_GHTDMU1 ", "GTV1MU1 to GHTDMU1"}},
        {523, {"GTV1MU1_TO_GTV2MU2 ", "GTV1MU1 to GTV2MU2"}},
        {524, {"ESR_TO_GTV2MU2 ", "ESR extraction to TV2MU2"}},
        {525, {"GHTDMU1_TO_HTC ", "Experiment line to nuclear reaction experiments at HTC (exotic nuclei and nuclear dynamics)"}},
        {526, {"GTV2MU2_TO_GTV2MU3 ", "GTV2MU2 to GTV2MU3"}},
        {527, {"GTV2MU3_TO_HTA ", "Experiment line to HTA"}},
        {528, {"GTV2MU3_TO_GHTBMU1 ", "GTV2MU3 to GHTBMU1"}},
        {529, {"GHTBMU1_TO_HTP ", "Experiment line to HTP Dump"}},
        {530, {"GHTDMU1_TO_HTD ", "Experiment line to HTD"}},
        {531, {"GHTBMU1_TO_YRT1MH2 ", "GHTBMU1 to YRT1MH2"}},
        {592, {"UR_TO_GUH1MU2 ", "HSI LEBT Right Source"}},
        {593, {"UL_TO_GUH1MU2 ", "HSI LEBT Left Source"}},
        {595, {"UN_TO_GUN3BC1L ", "HLI LEBT"}},
        {608, {"GUH1MU2_TO_GUH2BC1L ", "GUH1MU2 to GUH2BC1L"}},
        {609, {"GUH2BC1L_TO_GUS3MK1 ", "HSI"}},
        {610, {"GUS3MK1_TO_GUS4MK6 ", "GUS3MK1 to GUS4MK6"}},
        {611, {"GUS3MK1_TO_GUS3MK2 ", "GUS3MK1 to GUS3MK2"}},
        {612, {"GUS3MK2_TO_GUS4MK6 ", "GUS3MK2 to GUS4MK6"}},
        {613, {"GUS3MK2_TO_US3 ", "Diagnostic line US3"}},
        {624, {"GUN3BC1L_TO_GUN6MU2 ", "HLI"}},
        {625, {"GUN6MU2_TO_GUS4MK6 ", "GUN6MU2 to GUS4MK6"}},
        {626, {"GUN6MU2_TO_UCW ", "Development line to CW"}},
        {640, {"GUS4MK6_TO_GUT1MK0 ", "Poststripper"}},
        {641, {"GUT1MK0_TO_GUT1MK1 ", "GUT1MK0 to GUT1MK1"}},
        {642, {"GUT1MK1_TO_GUT2MK2 ", "GUT1MK1 to GUT2MK2"}},
        {656, {"GUT1MK1_TO_GTK3MV1 ", "GUT1MK1 to GTK3MV1"}},
        {657, {"GTK3MV1_TO_GTK3MV4 ", "GTK3MV1 to GTK3MV4"}},
        {658, {"GTK3MV1_TO_GTK3MV3 ", "GTK3MV1 to GTK3MV3"}},
        {659, {"GTK3MV3_TO_GTK3MV4 ", "GTK3MV3 to GTK3MV4"}},
        {660, {"GTK3MV3_TO_TKD ", "Diagnostic line TKD"}},
        {661, {"GTK3MV4_TO_GTK7BC1L ", "GTK3MV4 to GTK7BC1L"}},
        {672, {"GUT1MK0_TO_GUM2MU5 ", "GUT1MK0 to GUM2MU5"}},
        {673, {"GUM2MU5_TO_UM1 ", "Experiment line to UM1"}},
        {674, {"GUM2MU5_TO_GUM3MU6 ", "GUM2MU5 to GUM3MU6"}},
        {675, {"GUM3MU6_TO_UM2 ", "Experiment line to UM2"}},
        {676, {"GUM3MU6_TO_UM3 ", "Experiment line to UM3"}},
        {677, {"GUT2MK2_TO_GUZCMU1 ", "GUT2MK2 to GUZCMU1"}},
        {678, {"GUZCMU1_TO_UZ6 ", "Experiment line to UZ6"}},
        {679, {"GUZCMU1_TO_UZ7 ", "Experiment line to UZ7"}},
        {680, {"GUT2MK2_TO_UY7 ", "Experiment line to UY7"}},
        {688, {"GUT2MK2_TO_GUXAMU3 ", "GUT2MK2 to GUXAMU3"}},
        {689, {"GUXAMU3_TO_GUXAMU4 ", "GUXAMU3 to GUXAMU4"}},
        {690, {"GUXAMU3_TO_GUXCMU3 ", "GUXAMU3 to GUXCMU3"}},
        {691, {"GUXAMU4_TO_GUXBMU5 ", "GUXAMU4 to GUXBMU5"}},
        {692, {"GUXAMU4_TO_UX3 ", "Experiment line to UX3"}},
        {693, {"GUXBMU5_TO_UX2 ", "Experiment line to UX2"}},
        {694, {"GUXCMU3_TO_GUXFMU1 ", "GUXCMU3 to GUXFMU1"}},
        {695, {"GUXCMU3_TO_UX6 ", "Experiment line to UX6"}},
        {696, {"GUXFMU1_TO_GUXHMU2 ", "GUXFMU1 to GUXHMU2"}},
        {697, {"GUXFMU1_TO_UX7 ", "Experiment line to UX7"}},
        {698, {"GUXHMU2_TO_UX0 ", "Experiment line to UX0"}},
        {699, {"GUXHMU2_TO_UX8 ", "Experiment line to UX8"}},
        {928, {"SIS18_B2B_EXTRACT ", "B2B internal: extraction from SIS18"}},
        {929, {"SIS18_B2B_ESR ", "B2B internal: transfer SIS18 to ESR"}},
        {930, {"SIS18_B2B_SIS100 ", "B2B internal: transfer SIS18 to SIS100"}},
        {933, {"ESR_B2B_EXTRACT ", "B2B internal: extraction from ESR"}},
        {934, {"ESR_B2B_CRYRING ", "B2B internal: transfer ESR to CRYRING"}},
        {938, {"CRYRING_B2B_EXTRACT ", "B2B internal: extraction from CRYRING"}},
        {944, {"SIS100_B2B_EXTRACT ", "B2B internal: extraction from SIS100"}},
        {0xFFF, {"LocalGroup", "never sent by data master"}}
};
// https://www-acc.gsi.de/wiki/Timing/TimingSystemEventNumbers
static const std::map<uint16_t, std::pair<std::string, std::string>> eventNrTable{
        {0, {"EVT_PZ_ChanEnd", "SIS/ESR PZ only: mark end of channel"}},
        {1, {"EVT_START_RF", "Power to RF cavities"}},
        {2, {"EVT_START_IQ", "Begin of beam production"}},
        {3, {"EVT_IQ_HEATING", "Begin of ion source arc, ECR RF"}},
        {4, {"EVT_PREP_BEAM_ON", "Switch on chopper, read act. values"}},
        {5, {"EVT_IQ_GAS_ON", "Start of heading gas (ion source)"}},
        {6, {"EVT_BEAM_ON", "Valid beam"}},
        {8, {"EVT_BEAM_OFF", "End of beam production"}},
        {10, {"EVT_STOP_IQ", "Switch IQ off"}},
        {12, {"EVT_STOP_RF", "Switch RF off"}},
        {16, {"EVT_PREP_NEXT_ACC", "Prepare next acc., write set values"}},
        {17, {"EVT_AUX_PRP_NXT_ACC", "Set values in magnet prep. cycles"}},
        {18, {"EVT_RF_PREP_NXT_ACC", "Begin of RF extra cycle"}},
        {19, {"EVT_PREP_UNI_DIAG", "Prepare diagnostic devices, Unilac"}},
        {20, {"EVT_PREP_AUX", "Trigger BIF beam diagnostics"}},
        {21, {"EVT_UNLOCK_ALVAREZ", "Unlock A4 for next pulse"}},
        {22, {"EVT_PREP_EXP", "Pretrigger for Experiments"}},
        {24, {"EVT_RF_AUX_TRIG", "Trigger ADC in RF extra cycles"}},
        {25, {"EVT_MAGN_DOWN", "Set magnets to zero current"}},
        {26, {"EVT_SD_AUX_START", "Beam diagnostics aux start trigger"}},
        {27, {"EVT_SD_AUX_STOP", "Beam diagnostics aux stop trigger"}},
        {28, {"EVT_PRETRIG_BEAM", "Magnets on flattop, PG trigger"}},
        {29, {"EVT_UNI_END_CYCLE", "End of a UNILAC cycle"}},
        {30, {"EVT_READY_TO_SIS", "10 ms before beam transfer"}},
        {31, {"EVT_SRC_DST_ID", "Source/Destination of beam"}},
        {32, {"EVT_START_CYCLE", "First Event in a cycle"}},
        {33, {"EVT_START_BFELD", "Start of B-Field"}},
        {34, {"EVT_PEAK_UP", "Peaking trip up"}},
        {35, {"EVT_INJECT", "B-field reached injection level"}},
        {36, {"EVT_UNI_TRANS", "Demand UNILAC beam"}},
        {37, {"EVT_UNI_ACKN", "Acknowledge from Unilac"}},
        {38, {"EVT_UNI_READY", "Unilac ready, transfer in preparation"}},
        {39, {"EVT_MB_LOAD", "Start Bumper magnet ramp up"}},
        {40, {"EVT_MB_TRIGGER", "Start active Bumper Ramp (down)"}},
        {41, {"EVT_INJECT_END", "Start of injection from unilac"}},
        {42, {"EVT_TIMEOUT_1", "Trigger timeout channel 1"}},
        {43, {"EVT_RAMP_START", "Start of acc/deacc ramp in magnets"}},
        {44, {"EVT_PREP_INJ", "Prepare devices for next Injection"}},
        {45, {"EVT_FLATTOP", "Magnets reached Flattop"}},
        {46, {"EVT_EXTR_START_SLOW", "Start of extraction"}},
        {47, {"EVT_MK_LOAD_1", "Load Kicker for HF-triggered puls"}},
        {48, {"EVT_MK_LOAD_2", "Load Kicker for Timinggenerator-triggered puls"}},
        {49, {"EVT_KICK_START_1", "Start Kicker for HF-triggered extraction"}},
        {50, {"EVT_TIMEOUT_2", "Trigger timeout channel 2"}},
        {51, {"EVT_EXTR_END", "End of extraction"}},
        {52, {"EVT_FLATTOP_END", "End of Flattop (Magnets) reached"}},
        {53, {"EVT_PREP_EXTR", "Prepare devices for next Extraction"}},
        {54, {"EVT_PEAK_DOWN", "Peaking strip down"}},
        {55, {"EVT_END_CYCLE", "End of a cycle"}},
        {56, {"EVT_SYNCH", "Trigger all function generators"}},
        {57, {"EVT_EXTR_BUMP", "Start of closed orbit distortion"}},
        {58, {"EVT_SIS_ACK_TO_ESR", "SIS acknowledge to ESR"}},
        {59, {"EVT_SIS_READY_1", "SIS ready for extraction to ESR"}},
        {60, {"EVT_SIS_READY_2", "SIS ready for extraction to ESR"}},
        {61, {"EVT_TRANS_START_1", "Begin of transmission to ESR"}},
        {62, {"EVT_TRANS_START_2", "Begin of transmission to ESR"}},
        {63, {"EVT_PHASE_SYNCH_GATE_1", "Begin of first phase synchronisation between ESR- and SIS-HF"}},
        {64, {"EVT_TIMEOUT_3", "Trigger timeout channel 3"}},
        {65, {"EVT_PHASE_SYNCH_GATE_2", "Begin of second phase synchronisation between ESR- and SIS-HF"}},
        {66, {"EVT_TIMEOUT_4", "Trigger timeout channel 4"}},
        {67, {"EVT_TIMEOUT_5", "Trigger timeout channel 5"}},
        {68, {"EVT_TIMEOUT_6", "Trigger timeout channel 6"}},
        {69, {"EVT_KICK_START_2", "Start Kicker for TG-triggered extraction"}},
        {70, {"EVT_UNI_PREP", "Demand setting of TK (200 ms before beam)"}},
        {71, {"EVT_INJ_BUMP", "Closed orbit destortion for reinjection"}},
        {72, {"EVT_RE_INJ_START", "SIS is ready for reinjection"}},
        {73, {"EVT_RE_INJ_END", "End of reinjection"}},
        {74, {"EVT_PREP_RE_INJ", "Prepare devices for Reinjection"}},
        {75, {"EVT_PREP_KICK_1", "Prepare kicker: load capacitor 1"}},
        {76, {"EVT_PREP_KICK_2", "Prepare kicker: load capacitor 2"}},
        {77, {"EVT_MK_LOAD_RE_INJ", "Load Kicker for Reinjection"}},
        {78, {"EVT_EXTR_STOP_SLOW", "End of slow extraction"}},
        {79, {"EVT_ASYNC_TRANS", "Transfer to ESR without HF synchron."}},
        {80, {"EVT_EMA_START", "Start EMA meassurement gate"}},
        {81, {"EVT_HF_BM_START", "Vorbereitung Strahlgym."}},
        {82, {"EVT_HF_BM_AMPH2", "Start Ampl., Ph. Kav. 2 Strahlgym."}},
        {83, {"EVT_HF_BM_FREQ2", "Start Freq. Kav. 2 Strahlgym."}},
        {84, {"EVT_HF_BM_AMPH12", "Start Ampl., Ph. Kav. 1+2 Strahlgym."}},
        {85, {"EVT_HF_BM_FREQ1", "Start Freq. Kav. 1 Strahlgym."}},
        {86, {"EVT_HF_BM_AMPH1", "Start Ampl., Ph. Kav. 1 Strahlgym."}},
        {87, {"EVT_TG_CLEAR", "Clear Timinggenerator"}},
        {88, {"EVT_MQ_START1", "Load Q-Kicker (1st shot)"}},
        {89, {"EVT_MQ_START2", "Load Q-Kicker (2nd shot)"}},
        {90, {"EVT_MQ_MESS", "Trigger for Q-Wert measurement"}},
        {91, {"EVT_INT_FLAT", "Start of internal Flattop"}},
        {92, {"EVT_DT_MESS", "Trigger for DTML measurement"}},
        {93, {"EVT_DX_MESS", "Trigger for DX measurement"}},
        {94, {"EVT_TG_SWITCH", "Umschalten der TG synchronisation"}},
        {95, {"EVT_START_LECROY", "Start measurement with LeCroy"}},
        {96, {"EVT_GAP_POS_MESS", "Messevent zwischen den Zyklen"}},
        {97, {"EVT_GAP_TRA_MESS", "Messevent zwischen den Zyklen"}},
        {98, {"EVT_GAP_SCR_MESS", "Messevent zwischen den Zyklen"}},
        {99, {"EVT_GAP_DTS_MESS", "Messevent für schnelle Trafos"}},
        {100, {"EVT_SIS_TRANS1_ESR", "First transmission complete to ESR"}},
        {101, {"EVT_SIS_TRANS2_ESR", "Second transmission complete to ESR"}},
        {102, {"EVT_PREP_DIAGNOSE", "..."}},
        {103, {"EVT_PREP_DG", "Vorbereitung Gitterhardware"}},
        {104, {"EVT_DG_TRIGGER", "Trigger Messung Gitterhardware"}},
        {105, {"EVT_KICK_READY", "Ext. Synchronisat. für Kicker"}},
        {106, {"EVT_KICK_GATE", "Start externe Synchr. für Kicker"}},
        {107, {"EVT_PREP_DTI", "Entklemmung TK trafos SIS timing"}},
        {108, {"EVT_INJ_READY", "Einzelne Unilac-Injektion erfolgt"}},
        {109, {"EVT_MHB", "Multi-Harmonischer-Betrieb von GS00BE_S (SIS18)"}},
        {128, {"EVT_ESR_READY_1", "ESR ready for beam transfer"}},
        {129, {"EVT_ESR_READY_2", "ESR ready for beam transfer"}},
        {130, {"EVT_ESR_REQ_TO_SIS", "ESR beam request to SIS"}},
        {131, {"EVT_EIN1", "ESR ????? 1"}},
        {132, {"EVT_EIN2", "ESR ????? 2"}},
        {133, {"EVT_MAN1", "ESR manipulation 1"}},
        {134, {"EVT_MAN2", "ESR manipulation 2"}},
        {135, {"EVT_PHASE_SYNCH_1", "Phase is 1st time synchronized between ESR- and SIS-HF"}},
        {136, {"EVT_PHASE_SYNCH_2", "Phase is 2nd time synchronized between ESR- and SIS-HF"}},
        {137, {"EVT_NO_BEAM", "There is no beam in ESR (for diagnostics)"}},
        {138, {"EVT_DT_STOP", "Stop of DTML measurement"}},
        {139, {"EVT_DT_READ", "Read data for DTML measurement"}},
        {140, {"EVT_DX_STOP", "Stop of DX measurement"}},
        {141, {"EVT_DX_READ", "Read data for DX measurement"}},
        {142, {"EVT_LEXT", "ESR start with ?????????????"}},
        {143, {"EVT_PSTACK", "ESR start with ?????????????"}},
        {144, {"EVT_STACK", "ESR start with ?????????????"}},
        {145, {"EVT_ESR_TRANS_SIS", "Transmission complete to SIS"}},
        {146, {"EVT_ECE_HV_VAR", "Stepwise variation of ECE voltage"}},
        {147, {"EVT_ECE_HV_ON", "Write set value for pulsed ECE HV"}},
        {148, {"EVT_ECE_HV_MESS", "Read actual value of pulsed ECE HV"}},
        {149, {"EVT_BUNCH_ROTATE", "Start with bunch rotation"}},
        {150, {"EVT_ESR_REQ_REINJ", "ESR request SIS reinjection"}},
        {151, {"EVT_RESET", "Start of 11th Ramp in magnets"}},
        {152, {"EVT_AUS1", "Start of 11th Ramp in magnets"}},
        {153, {"EVT_AUS2", "Start of 11th Ramp in magnets"}},
        {154, {"EVT_RAMP_11", "Start of 11th Ramp in magnets"}},
        {155, {"EVT_RAMP_12", "Start of 12th Ramp in magnets"}},
        {156, {"EVT_RAMP_13", "Start of 13th Ramp in magnets"}},
        {157, {"EVT_RAMP_14", "Start of 14th Ramp in magnets"}},
        {158, {"EVT_RAMP_15", "Start of 15th Ramp in magnets"}},
        {159, {"EVT_RAMP_16", "Start of 16th Ramp in magnets"}},
        {160, {"EVT_EBEAM_ON", "Electron beam on"}},
        {161, {"EVT_EBEAM_OFF", "Electron beam off"}},
        {162, {"EVT_UDET_IN", "Move detector (charge changed) in"}},
        {163, {"EVT_UDET_OUT", "Move detector out"}},
        {180, {"EVT_TIMING_LOCAL", "Take local timing"}},
        {181, {"EVT_TIMING_EXTERN", "Switch to extern timing"}},
        {199, {"EVT_INTERNAL_FILL", "PZ: Fill long event delays (>10s)"}},
        {200, {"EVT_DATA_START", "First data transfer event"}},
        {201, {"EVT_DATA_0", "Data transfer event"}},
        {202, {"EVT_DATA_1", "Data transfer event"}},
        {203, {"EVT_DATA_2", "Data transfer event"}},
        {204, {"EVT_DATA_3", "Data transfer event"}},
        {205, {"EVT_DATA_4", "Data transfer event"}},
        {206, {"EVT_DATA_5", "Data transfer event"}},
        {207, {"EVT_DATA_6", "Data transfer event"}},
        {208, {"EVT_DATA_7", "Data transfer event"}},
        {209, {"EVT_TIME_1", "Time stamp, most sign. bits"}},
        {210, {"EVT_TIME_2", "Time stamp, medium sign. bits"}},
        {211, {"EVT_TIME_3", "Time stamp, least sign. bits"}},
        {224, {"EVT_UTC_1", "Time stamp UTC bits 32..39"}},
        {225, {"EVT_UTC_2", "Time stamp UTC bits 24..31"}},
        {226, {"EVT_UTC_3", "Time stamp UTC bits 16..23"}},
        {227, {"EVT_UTC_4", "Time stamp UTC bits 8..15"}},
        {228, {"EVT_UTC_5", "Time stamp UTC bits 0.. 7"}},
        {245, {"EVT_END_CMD_EXEC", "End of command evaluation within gap"}},
        {246, {"EVT_BEGIN_CMD_EXEC", "Start of command evaluation within gap"}},
        {247, {"EVT_GAP_INFO", "PZ information about next cycle"}},
        {248, {"EVT_COLL_DET", "PZ detected collision of 2 asynch. events"}},
        {249, {"EVT_TIMING_DIAG", "For diagnostics in timing system"}},
        {250, {"EVT_SUP_CYCLE_START", "Supercycle starts"}},
        {251, {"EVT_GET_EC_TIME", "Synchronous reading of time of all ECs"}},
        {252, {"EVT_SET_EC_TIME", "Synchronous setting of time in all ECs"}},
        {253, {"EVT_RESERVED", "In older systems : change vrtacc event"}},
        {254, {"EVT_EMERGENCY", "Emergency event"}},
        {255, {"EVT_COMMAND", "Command event"}},
        {256, {"CMD_BP_START", "Start of Beam Process"}},
        {257, {"CMD_SEQ_START", "Sequence start. Implicitly, a CMD_SEQ_START is also a CMD_BP_START"}},
        {258, {"CMD_GAP_START", "Start of gap window. Within this window, background tasks can be performed safely."}},
        {259, {"CMD_GAP_END", "End of gap window. Within this window, background tasks can be performed safely."}},
        {270, {"CMD_SOURCE_BEAM_ON", "Source Beam On"}},
        {271, {"CMD_SOURCE_BEAM_OFF", "Source Beam Off"}},
        {272, {"CMD_SOURCE_HV_PREP", "HV Preparation for the Source"}},
        {273, {"CMD_SOURCE_START", "Start beam production in source"}},
        {274, {"CMD_SOURCE_STOP", "Stop beam production in source and switch source to standby"}},
        {280, {"CMD_BI_TRIGGER", "Beam Diagnostics: Trigger"}},
        {281, {"CMD_BI_MEAS1", "Beam Diagnostics: Measure Event 1"}},
        {282, {"CMD_BI_MEAS2", "Beam Diagnostics: Measure Event 2"}},
        {283, {"CMD_BEAM_INJECTION", "Injection into a ring machine; played shortly prior to each injection within a sequence"}},
        {284, {"CMD_BEAM_EXTRACTION", "Extraction from a ring machine; played shortly prior to each extraction within a sequence"}},
        {285, {"CMD_START_ENERGY_RAMP", "Start acceleration or deceleration; only for beam-in processes; intended for diagnostic purposes"}},
        {286, {"CMD_CUSTOM_DIAG_1", "Intended for diagnostic purposes"}},
        {287, {"CMD_CUSTOM_DIAG_2", "Intended for diagnostic purposes"}},
        {288, {"CMD_CUSTOM_DIAG_3", "Intended for diagnostic purposes"}},
        {290, {"CMD_RF_PREP", "RF: Prepare device for pulse generation"}},
        {291, {"CMD_RF_PREP_PAUSE", "RF: Prepare device for pulse generation (paused operation and beam-out)"}},
        {292, {"CMD_RF_STOP_PAUSE", "RF: Stop generation of pulse (paused operation and beam-out)"}},
        {312, {"CMD_SYNCH", "Trigger all function generators"}},
        {313, {"CMD_RF_SWITCH_01", "Change RF system operation mode according to predefined settings"}},
        {314, {"CMD_RF_SWITCH_02", "Change RF system operation mode according to predefined settings"}},
        {315, {"CMD_RF_SWITCH_03", "Change RF system operation mode according to predefined settings"}},
        {316, {"CMD_RF_SWITCH_04", "Change RF system operation mode according to predefined settings"}},
        {317, {"CMD_RF_SWITCH_05", "Change RF system operation mode according to predefined settings"}},
        {318, {"CMD_RF_SWITCH_06", "Change RF system operation mode according to predefined settings"}},
        {319, {"CMD_RF_SWITCH_07", "Change RF system operation mode according to predefined settings"}},
        {320, {"CMD_RF_SWITCH_08", "Change RF system operation mode according to predefined settings"}},
        {321, {"CMD_RF_SWITCH_09", "Change RF system operation mode according to predefined settings"}},
        {322, {"CMD_RF_SWITCH_10", "Change RF system operation mode according to predefined settings"}},
        {323, {"CMD_RF_SWITCH_11", "Change RF system operation mode according to predefined settings"}},
        {324, {"CMD_RF_SWITCH_12", "Change RF system operation mode according to predefined settings"}},
        {325, {"CMD_RF_SWITCH_13", "Change RF system operation mode according to predefined settings"}},
        {326, {"CMD_RF_SWITCH_14", "Change RF system operation mode according to predefined settings"}},
        {327, {"CMD_RF_SWITCH_15", "Change RF system operation mode according to predefined settings"}},
        {328, {"CMD_RF_SWITCH_16", "Change RF system operation mode according to predefined settings"}},
        {329, {"CMD_RF_SWITCH_17", "Change RF system operation mode according to predefined settings"}},
        {330, {"CMD_RF_SWITCH_18", "Change RF system operation mode according to predefined settings"}},
        {331, {"CMD_RF_SWITCH_19", "Change RF system operation mode according to predefined settings"}},
        {332, {"CMD_RF_SWITCH_20", "Change RF system operation mode according to predefined settings"}},
        {333, {"CMD_RF_SWITCH_21", "Change RF system operation mode according to predefined settings"}},
        {334, {"CMD_RF_SWITCH_22", "Change RF system operation mode according to predefined settings"}},
        {335, {"CMD_RF_SWITCH_23", "Change RF system operation mode according to predefined settings"}},
        {336, {"CMD_RF_SWITCH_24", "Change RF system operation mode according to predefined settings"}},
        {337, {"CMD_RF_SWITCH_25", "Change RF system operation mode according to predefined settings"}},
        {338, {"CMD_RF_SWITCH_26", "Change RF system operation mode according to predefined settings"}},
        {339, {"CMD_RF_SWITCH_27", "Change RF system operation mode according to predefined settings"}},
        {340, {"CMD_RF_SWITCH_28", "Change RF system operation mode according to predefined settings"}},
        {341, {"CMD_RF_SWITCH_29", "Change RF system operation mode according to predefined settings"}},
        {342, {"CMD_RF_SWITCH_30", "Change RF system operation mode according to predefined settings"}},
        {343, {"CMD_RF_SWITCH_31", "Change RF system operation mode according to predefined settings"}},
        {344, {"CMD_RF_SWITCH_32", "Change RF system operation mode according to predefined settings"}},
        {345, {"CMD_RF_PHASE_RESET", "Trigger the DDS phase reset for RF devices"}},
        {350, {"CMD_UNI_TCREQ", "Request UNILAC PZ to reserve the transfer channel"}},
        {351, {"CMD_UNI_TCREL", "Request UNILAC PZ to release the transfer channel"}},
        {352, {"CMD_UNI_BREQ", "Request UNILAC PZ to deliver beam"}},
        {353, {"CMD_UNI_BPREP", "Request UNILAC PZ to prepare beam delivery"}},
        {354, {"CMD_UNI_BREQ_NOWAIT", "Request UNILAC PZ to deliver beam but without 'wait block' in Data Master schedule"}},
        {512, {"CMD_FG_PREP", "Prepare function generator"}},
        {513, {"CMD_FG_START", "Start function generator"}},
        {518, {"CMD_BEAM_ON", "General: Begin of beam passage"}},
        {520, {"CMD_BEAM_OFF", "General: End of beam passage"}},
        {521, {"CMD_SEPTUM_CHARGE", "Start septum ramp up"}},
        {522, {"CMD_EBEAM_ON", "Electron beam on"}},
        {523, {"CMD_EBEAM_OFF", "Electron beam off"}},
        {524, {"CMD_STOCH_COOLING_ON", "Stochastic cooling on"}},
        {525, {"CMD_STOCH_COOLING_OFF", "Stochastic cooling off"}},
        {526, {"CMD_TARGET_ON", "Enable target"}},
        {527, {"CMD_TARGET_OFF", "Disable target"}},
        {528, {"CMD_EXP_DAQ_START", "Start experiment data acquisition"}},
        {529, {"CMD_EXP_DAQ_STOP", "Stop experiment data acquisition"}},
        {530, {"CMD_EXP_DRIVE_IN", "Move experiment actuators (detectors) to measurement position"}},
        {531, {"CMD_EXP_DRIVE_OUT", "Move experiment actuators (detectors) to outside position (clear beam aperture)"}},
        {532, {"CMD_DRIVE_MOVE", "Move timing-controlled actuators to new position"}},
        {539, {"CMD_CHOPPER_CHARGE", "Start energizing chopper"}},
        {1024, {"CMD_BUMPER_CHARGE", "Start bumper magnet ramp up"}},
        {1025, {"CMD_BUMPER_START", "Start bumper ramp down"}},
        {1026, {"CMD_INJ_KICKER_START", "Start injection kicker"}},
        {1027, {"CMD_EXTR_KICKER_START", "Start extraction kicker"}},
        {2048, {"CMD_B2B_PMEXT", "B2B internal: request phase measurement (extraction)"}},
        {2049, {"CMD_B2B_PMINJ", "B2B internal: request phase measurement (injection)"}},
        {2050, {"CMD_B2B_PREXT", "B2B internal: send result of phase measurement (extraction)"}},
        {2051, {"CMD_B2B_PRINJ", "B2B internal: send result of phase measurement (injection)"}},
        {2052, {"CMD_B2B_TRIGGEREXT", "B2B: trigger kicker electronics (extraction)"}},
        {2053, {"CMD_B2B_TRIGGERINJ", "B2B: trigger kicker electronics (injection)"}},
        {2054, {"CMD_B2B_DIAGKICKEXT", "B2B: send result of kick diagnostic (extraction)"}},
        {2055, {"CMD_B2B_DIAGKICKINJ", "B2B: send result of kick diagnostic (injection)"}},
        {2056, {"CMD_B2B_DIAGEXT", "B2B internal: send result of optional diagnostic (extraction)"}},
        {2057, {"CMD_B2B_DIAGINJ", "B2B internal: send result of optional diagnostic (injection)"}},
        {2079, {"CMD_B2B_START", "Start B2B procedure"}},
        {4000, {"CMD_GMT_INTERNAL1", "internal use by the GMT"}}
};

static auto tai_ns_to_utc(auto input) {
    return std::chrono::utc_clock::to_sys(std::chrono::tai_clock::to_utc(std::chrono::tai_clock::time_point{} + std::chrono::nanoseconds(input)));
}

template <typename... T>
void tableColumnString(fmt::format_string<T...> &&fmt, T&&... args) {
    if (ImGui::TableNextColumn()) {
        ImGui::Text("%s", fmt::vformat(fmt, fmt::make_format_args(args...)).c_str());
    }
}
void tableColumnBool(bool state, ImColor trueColor, ImColor falseColor) {
    if (ImGui::TableNextColumn()) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, state ? trueColor : falseColor);
        ImGui::Text("%s", fmt::format("{}", state ? "y" : "n").c_str());
    }
}
template <typename T>
void tableColumnSlider(const std::string &id, T &field, const uint64_t &max, float width) {
    static constexpr uint64_t min_uint64 = 0;
    if (ImGui::TableNextColumn()) {
        ImGui::SetNextItemWidth(width);
        uint64_t tmp = field;
        ImGui::DragScalar(id.c_str(), ImGuiDataType_U64, &tmp, 1.0f, &min_uint64, &max, "%d", ImGuiSliderFlags_None);
        field = tmp & max;
    }
}
void tableColumnCheckbox(const std::string &id, bool &field) {
    if (ImGui::TableNextColumn()) {
        bool flag_beamin = field;
        ImGui::Checkbox(id.c_str(), &flag_beamin);
        field = flag_beamin;
    }
}

void showTimingEventTable(gr::BufferReader auto &event_reader) {

    if (ImGui::Button("clear")) {
        std::ignore = event_reader.consume(event_reader.available());
    }
    if (ImGui::CollapsingHeader("Received Timing Events", ImGuiTreeNodeFlags_DefaultOpen)) {
        static int freeze_cols = 1;
        static int freeze_rows = 1;

        static std::map<uint16_t, ImColor> colors;

        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 20);
        static ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        if (auto _ = ImScoped::Table("received_events", 19, flags, outer_size, 0.f)) {
            ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
            ImGui::TableSetupColumn("timestamp", ImGuiTableColumnFlags_NoHide); // Make the first column not hideable to match our use of TableSetupScrollFreeze()
            ImGui::TableSetupColumn("executed at", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("bpcid");
            ImGui::TableSetupColumn("sid");
            ImGui::TableSetupColumn("bpid");
            ImGui::TableSetupColumn("gid");
            ImGui::TableSetupColumn("eventno");
            ImGui::TableSetupColumn("beam-in");
            ImGui::TableSetupColumn("bpc-start");
            ImGui::TableSetupColumn("req no beam");
            ImGui::TableSetupColumn("virt acc");
            ImGui::TableSetupColumn("bpcts");
            ImGui::TableSetupColumn("fid", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("param", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("reserved1", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("reserved2", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("reserved", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("flags");

            ImGui::TableHeadersRow();
            auto data = event_reader.get();

            for (auto &evt : std::ranges::reverse_view{data}) {
                ImGui::TableNextRow();
                tableColumnString("{}", tai_ns_to_utc(evt.time));
                tableColumnString("{}", tai_ns_to_utc(evt.executed));
                tableColumnString("{}", evt.bpcid);
                tableColumnString("{}", evt.sid);
                if (ImGui::TableNextColumn()) {
                    if (colors.contains(evt.bpid)) {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, colors[evt.bpid]);
                    } else {
                        if (colorlist.size() > colors.size()) {
                            colors.insert({evt.bpid, ImColor{colorlist[colors.size()]}});
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, colors[evt.bpid]);
                        }
                    }
                    ImGui::Text("%s", fmt::format("{}", evt.bpid).c_str());
                }
                tableColumnString("{1}({0})", evt.gid, timingGroupTable.contains(evt.gid) ? timingGroupTable.at(evt.gid).first : "UNKNOWN_TIMING_GROUP""{}");
                tableColumnString("{1}({0})", evt.eventno, eventNrTable.contains(evt.eventno) ? eventNrTable.at(evt.eventno).first : "UNKNOWN_EVENT");
                tableColumnBool(evt.flag_beamin, ImGui::GetColorU32({0,1.0,0,0.4f}), ImGui::GetColorU32({1.0,0,0,0.4f}));
                tableColumnBool(evt.flag_bpc_start, ImGui::GetColorU32({0,1.0,0,0.4f}), ImGui::GetColorU32({1.0,0,0,0.4f}));
                tableColumnBool(evt.reqNoBeam, ImGui::GetColorU32({0,1.0,0,0.4f}), ImGui::GetColorU32({1.0,0,0,0.4f}));
                tableColumnString("{}", evt.virtAcc);
                tableColumnString("{}", evt.bpcts);
                tableColumnString("{}", evt.fid);
                tableColumnString("{:#08x}", evt.id());
                tableColumnString("{:#08x}", evt.param());
                tableColumnBool(evt.flag_reserved1, ImGui::GetColorU32({0,1.0,0,0.4f}), ImGui::GetColorU32({1.0,0,0,0.4f}));
                tableColumnBool(evt.flag_reserved2, ImGui::GetColorU32({0,1.0,0,0.4f}), ImGui::GetColorU32({1.0,0,0,0.4f}));
                tableColumnBool(evt.reserved, ImGui::GetColorU32({0,1.0,0,0.4f}), ImGui::GetColorU32({1.0,0,0,0.4f}));
                if (ImGui::TableNextColumn()) { // flags
                    // print flags
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, evt.flags ? ImGui::GetColorU32({1.0,0,0,0.4f}) : ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_TableRowBg]));
                    auto delay = (static_cast<double>(evt.executed - evt.time)) * 1e-6;
                    if (evt.flags & 1) {
                        ImGui::Text("%s", fmt::format(" !late (by {} ms)", delay).c_str());
                    } else if (evt.flags & 2) {
                        ImGui::Text("%s", fmt::format(" !early (by {} ms)", delay).c_str());
                    } else if (evt.flags & 4) {
                        ImGui::Text("%s", fmt::format(" !conflict (delayed by {} ms)", delay).c_str());
                    } else if (evt.flags & 8) {
                        ImGui::Text("%s", fmt::format(" !delayed (by {} ms)", delay).c_str());
                    }
                }
            }
            if (data.size() > event_reader.buffer().size() / 2) {
                std::ignore = event_reader.consume(data.size() - event_reader.buffer().size() / 2);
            }
        }
    }
}

void showTimingSchedule(Timing &timing) {
    static constexpr uint64_t min_uint64 = 0;
    static constexpr uint64_t max_uint64 = std::numeric_limits<uint64_t>::max();
    static constexpr uint64_t max_uint42 = (1ul << 42) - 1;
    static constexpr uint64_t max_uint22 = (1ul << 22) - 1;
    static constexpr uint64_t max_uint14 = (1ul << 14) - 1;
    static constexpr uint64_t max_uint12 = (1ul << 12) - 1;
    static constexpr uint64_t max_uint4 = (1ul << 4) - 1;

    static std::size_t current = 0;
    static uint64_t time_offset = 0;
    static std::vector<Timing::event> events{};
    static enum class InjectState { STOPPED, RUNNING, ONCE, SINGLE } injectState = InjectState::STOPPED;
    if (ImGui::CollapsingHeader("Schedule to inject", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SetNextItemWidth(80.f);
        static uint64_t default_offset = 100000000ul; // 100ms
        ImGui::DragScalar("default event offset", ImGuiDataType_U64, &default_offset, 1.0f, &min_uint64, &max_uint64, "%d", ImGuiSliderFlags_None);
        ImGui::SameLine();
        if (ImGui::Button("+")) {
            events.emplace_back(default_offset + (events.empty() ? 0ul : events.back().time));
        }
        ImGui::SameLine();
        if (ImGui::Button("clear##schedule")) {
            events.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("load")) {
            events.clear();
            std::string string = ImGui::GetClipboardText();
            using std::operator""sv;
            try {
                for (auto line: std::views::split(string, "\n"sv)) {
                    std::string_view line_sv(line);
                    if (line_sv.empty()) continue;
                    std::array<uint64_t, 3> numbers{};
                    for (auto [i, number]: std::views::split(line_sv, " "sv) | std::views::enumerate) {
                        std::string_view number_sv(number);
                        numbers[i] = std::stoul(std::string(number_sv), nullptr, 0);
                    }
                    events.emplace_back(numbers[2], numbers[0], numbers[1]);
                }
            } catch (std::exception &e) {
                events.clear();
                fmt::print("Error parsing clipboard data: {}", string);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("store")) {
            std::string string;
            for (auto &ev: events) {
                string.append(fmt::format("{:#x} {:#x} {}\n", ev.id(), ev.param(), ev.time));
            }
            ImGui::SetClipboardText(string.c_str());
        }
        // set state
        ImGui::SameLine();
        if (ImGui::Button("start")) {
            current = 0;
            time_offset = timing.receiver->CurrentTime().getTAI();
            injectState = InjectState::RUNNING;
        }
        ImGui::SameLine();
        if (ImGui::Button("once")) {
            current = 0;
            time_offset = timing.receiver->CurrentTime().getTAI();
            injectState = InjectState::ONCE;
        }
        ImGui::SameLine();
        if (ImGui::Button("stop")) {
            injectState = InjectState::STOPPED;
        }
        // schedule table
        static int freeze_cols = 1;
        static int freeze_rows = 1;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 15);
        static ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        ImGui::BeginDisabled(injectState != InjectState::STOPPED);
        if (auto _ = ImScoped::Table("event schedule", 20, flags, outer_size, 0.f)) {
            ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
            ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_NoHide); // Make the first column not hideable to match our use of TableSetupScrollFreeze()
            ImGui::TableSetupColumn("bpcid");
            ImGui::TableSetupColumn("sid");
            ImGui::TableSetupColumn("bpid");
            ImGui::TableSetupColumn("gid");
            ImGui::TableSetupColumn("eventno");
            ImGui::TableSetupColumn("beam-in");
            ImGui::TableSetupColumn("bpc-start");
            ImGui::TableSetupColumn("req no beam");
            ImGui::TableSetupColumn("virt acc");
            ImGui::TableSetupColumn("bpcts");
            ImGui::TableSetupColumn("fid");
            ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("param", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("reserved1", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("reserved2", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("reserved", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("##inject", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("##remove", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Trigger Generation", ImGuiTableColumnFlags_NoHide);

            ImGui::TableHeadersRow();

            events.erase(std::remove_if(events.begin(), events.end(), [&timing, default_offset = default_offset](auto &ev) {
                bool to_remove = false;
                ImGui::PushID(&ev);
                ImGui::TableNextRow();
                tableColumnSlider("##time", ev.time, max_uint64, 80.f);
                tableColumnSlider("##bpcid",ev.bpcid, max_uint22, 40.f);
                tableColumnSlider("##sid", ev.sid, max_uint12, 40.f);
                tableColumnSlider("##pbid", ev.bpid, max_uint14, 40.f);
                tableColumnSlider("##gid", ev.gid, max_uint12,40.f);
                tableColumnSlider("##eventno", ev.eventno, max_uint12, 40.f);
                tableColumnCheckbox("##beamin", ev.flag_beamin);
                tableColumnCheckbox("##bpcstart",ev.flag_bpc_start);
                tableColumnCheckbox("##reqNoBeam",ev.reqNoBeam);
                tableColumnSlider("##virtAcc", ev.virtAcc, max_uint4, 40.f);
                tableColumnSlider("##bpcts",ev.bpcts, max_uint42, 80.f);
                tableColumnSlider("##fid",ev.fid, max_uint4, 40.f);
                tableColumnString("{:#08x}", ev.id());
                tableColumnString("{:#08x}", ev.param());
                tableColumnCheckbox("##reserved1",ev.flag_reserved1);
                tableColumnCheckbox("##reserved2",ev.flag_reserved2);
                tableColumnCheckbox("##reserved",ev.reserved);
                // interactive settings
                ImGui::TableNextColumn();
                if (ImGui::Button("remove")) {
                    to_remove = true;
                }
                ImGui::TableNextColumn();
                if (ImGui::Button("inject")) {
                    timing.injectEvent(ev, timing.receiver->CurrentTime().getTAI() + default_offset);
                }
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("TODO: add timing trigger info here");
                ImGui::PopID();
                return to_remove;
            }), events.end());
        }
        ImGui::EndDisabled();
    }
    // if running, schedule events up to 100ms ahead
    if (injectState == InjectState::RUNNING || injectState == InjectState::ONCE || injectState == InjectState::SINGLE) {
        while (events[current].time + time_offset < timing.receiver->CurrentTime().getTAI() + 500000000ul) {
            auto ev = events[current];
            timing.injectEvent(ev, time_offset);
            if (injectState == InjectState::SINGLE) {
                injectState = InjectState::STOPPED;
                break;
            }
            if (current + 1 >= events.size()) {
                if (injectState == InjectState::ONCE) {
                    injectState = InjectState::STOPPED;
                    break;
                } else {
                    time_offset += events[current].time;
                    current = 0;
                }
            } else {
                ++current;
            }
        }
    }
}

void showTRConfig(Timing &timing) {
    ImGui::SetNextItemOpen(false, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Timing Receiver IO configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (!timing.initialized) {
            ImGui::TextUnformatted("No timing receiver found");
            return;
        }
        auto trTime = timing.receiver->CurrentTime().getTAI();
        ImGui::TextUnformatted(fmt::format("{} -- ({} ns)\nTemperature: {}°C,\nGateware: {},\n(\"version\", \"{}\")",
                                           trTime, tai_ns_to_utc(trTime),
                                           timing.receiver->CurrentTemperature(),
                                           fmt::join(timing.receiver->getGatewareInfo(), ",\n"),
                                           timing.receiver->getGatewareVersion()).c_str());
        // print table of io info
        static int freeze_cols = 1;
        static int freeze_rows = 1;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size = ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, TEXT_BASE_HEIGHT * 24);
        static ImGuiTableFlags flags =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        if (auto _ = ImScoped::Table("ioConfiguration", 8, flags, outer_size, 0.f)) {
            ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Direction");
            ImGui::TableSetupColumn("Output Enable");
            ImGui::TableSetupColumn("Input Termination");
            ImGui::TableSetupColumn("Special Out");
            ImGui::TableSetupColumn("Special In");
            ImGui::TableSetupColumn("Resolution");
            ImGui::TableSetupColumn("Level");
            ImGui::TableHeadersRow();

            auto outputs = timing.receiver->getOutputs();

            for (auto &out :outputs) {
                ImGui::PushID(&out);
                ImGui::TableNextRow();
                auto port_proxy = saftlib::Output_Proxy::create(out.second);
                auto conditions = port_proxy->getAllConditions();
                if (ImGui::TableSetColumnIndex(0)) {
                    ImGui::Text("%s", fmt::format("{}", out.first).c_str());
                }
                ImGui::PopID();
            }
        }
        // Table of ECA Conditions
        ImGui::SameLine();
        static int freeze_cols_conds = 1;
        static int freeze_rows_conds = 1;
        ImVec2 outer_size_conds = ImVec2(0.0f, TEXT_BASE_HEIGHT * 24);
        static ImGuiTableFlags flags_conds =
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        if (auto _ = ImScoped::Table("ioConfiguration", 8, flags_conds, outer_size_conds, 0.f)) {
            ImGui::TableSetupScrollFreeze(freeze_cols_conds, freeze_rows_conds);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Direction");
            ImGui::TableSetupColumn("Output Enable");
            ImGui::TableSetupColumn("Input Termination");
            ImGui::TableSetupColumn("Special Out");
            ImGui::TableSetupColumn("Special In");
            ImGui::TableSetupColumn("Resolution");
            ImGui::TableSetupColumn("Level");
            ImGui::TableHeadersRow();

            auto outputs = timing.receiver->getOutputs();

            for (auto &out :outputs) {
                ImGui::PushID(&out);
                ImGui::TableNextRow();
                auto port_proxy = saftlib::Output_Proxy::create(out.second);
                auto conditions = port_proxy->getAllConditions();
                if (ImGui::TableSetColumnIndex(0)) {
                    ImGui::Text("%s", fmt::format("{}", out.first).c_str());
                }
                ImGui::PopID();
            }
        }

        // Table: EVENT | MASK | OFFSET | FLAGS (late1/early2/conflict4/delayed8) | on/off
    }
}

void showEBConsole(WBConsole &console) {
    ImGui::SetNextItemOpen(false, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("EtherBone Console", ImGuiTreeNodeFlags_CollapsingHeader)) {
    }
}

void showTimePlot(gr::BufferReader auto &picoscope_reader, Timing &timing, gr::BufferReader auto &event_reader) {
    auto data = picoscope_reader.get();
    double plot_depth = 20; // [ms] = 5 s
    double time = timing.receiver->CurrentTime().getTAI() * 1e-9;
    if (ImGui::CollapsingHeader("Plot", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImPlot::BeginPlot("timing markers", ImVec2(-1,0), ImPlotFlags_CanvasOnly)) {
            ImPlot::SetupAxes("x","y");
            ImPlot::SetupAxisLimits(ImAxis_X1, time, time - plot_depth, ImGuiCond_Always);
            auto d= event_reader.get();

            ImPlot::PushStyleVar(ImPlotStyleVar_DigitalBitHeight, 16.0f);
            ImPlot::PushStyleColor(ImPlotCol_Line, {1.0,0,0,0.6f});
            ImPlot::PushStyleColor(ImPlotCol_Fill, {0,1.0,0,0.6f});
            ImPlot::PlotStatusBarG("beamin", [](int i, void* data) {auto ev = ((Timing::event*) data)[i];return ImPlotPoint{ev.time * 1e-9, ev.flag_beamin * 1.0};}, ((void *) d.data()), d.size(), ImPlotStatusBarFlags_Bool);
            ImPlot::PopStyleColor(2);

            ImPlot::PlotStatusBarG("pbcid", [](int i, void* data) {auto ev = ((Timing::event*) data)[i];return ImPlotPoint{ev.time * 1e-9, ev.bpcid * 1.0};}, ((void *) d.data()), d.size(), ImPlotStatusBarFlags_Discrete);

            ImPlot::PushStyleColor(ImPlotCol_Line, {0.7f,0.2f,0,0.6f});
            ImPlot::PushStyleColor(ImPlotCol_Fill, {0.2f,0.1f,0,0.6f});
            ImPlot::PlotStatusBarG("pbcid", [](int i, void* data) {auto ev = ((Timing::event*) data)[i];return ImPlotPoint{ev.time * 1e-9, ev.sid * 1.0};}, ((void *) d.data()), d.size(), ImPlotStatusBarFlags_Alternate);
            ImPlot::PopStyleColor(2);

            ImPlot::PushStyleColor(ImPlotCol_Line, {0,0.7f,0.2f,0.6f});
            ImPlot::PushStyleColor(ImPlotCol_Fill, {0,0.2f,0.7f,0.6f});
            ImPlot::PlotStatusBarG("pbid", [](int i, void* data) {auto ev = ((Timing::event*) data)[i];return ImPlotPoint{ev.time * 1e-9, ev.bpid * 1.0};}, ((void *) d.data()), d.size(), ImPlotStatusBarFlags_Alternate);
            ImPlot::PopStyleColor(2);
            ImPlot::PopStyleVar();

            // plot non-beam process events

            // TODO: reenable picoscope data
            //ImPlot::PlotLine("IO1", data.data(), data.size());
            ImPlot::EndPlot();
        }
    }
    // remove as many items from the buffer s.t. it only half full so that new samples can be added
    if (data.size() > picoscope_reader.buffer().size() / 2) {
        std::ignore = picoscope_reader.consume(data.size() - picoscope_reader.buffer().size() / 2);
    }
}

int interactive(Ps4000a &digitizer, Timing &timing, WBConsole &console) {
    // Initialize UI
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }
    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::StyleColorsLight();

    auto defaultFont = app_header::loadHeaderFont(13.f);
    auto headerFont = app_header::loadHeaderFont(32.f);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    gr::BufferReader auto event_reader = timing.snooped.new_reader();
    gr::BufferReader auto digitizer_reader = digitizer.buffer().new_reader();
    //gr::HistoryBuffer<Timing::event, 1000> snoop_history{};

    // Main loop
    bool done = false;
    while (!done) {
        digitizer.process();
        //console.process();
        timing.process();

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                done = true;
            } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
                done = true;
            }
        }
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        static ImGuiWindowFlags imGuiWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);
        if (auto _ = ImScoped::Window("Example: Fullscreen window", nullptr, imGuiWindowFlags)) {
            // TODO: include FAIR header
            app_header::draw_header_bar("Digitizer Timing Debug", headerFont);
            showTimingEventTable(event_reader);
            showTimingSchedule(timing);
            showTRConfig(timing);
            showTimePlot(digitizer_reader, timing, event_reader);
            showEBConsole(console);
        }

        ImGui::ShowDemoWindow();
        ImPlot::ShowDemoWindow();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void draw_plot() {
}

int main(int argc, char** argv) {
    Ps4000a picoscope; // a picoscope to examine generated timing signals
    Timing timing; // an interface to the timing card allowing condition & io configuration and event injection & snooping
    WBConsole console; // a whishbone console to send raw commands to the timing card and switch it beteween master and slave mode

    CLI::App app{"timing receiver saftbus example"};
    bool scope = false;
    app.add_flag("--scope", scope, "enter interactive scope mode showing the timing input connected to a picoscope and the generated and received timing msgs");
    app.add_flag("--pps", timing.ppsAlign, "add time to next pps pulse");
    app.add_flag("--abs", timing.absoluteTime, "time is an absolute time instead of an offset");
    app.add_flag("--utc", timing.UTC, "absolute time is in utc, default is tai");
    app.add_flag("--leap", timing.UTCleap, "utc calculation leap second flag");

    bool snoop = false;
    app.add_flag("-s", snoop, "snoop");
    app.add_option("-f", timing.snoopID, "id filter");
    app.add_option("-m", timing.snoopMask, "snoop mask");
    app.add_option("-o", timing.snoopOffset, "snoop offset");

    CLI11_PARSE(app, argc, argv);

    if (scope) {
        fmt::print("entering interactive timing scope mode\n");
        return interactive(picoscope, timing, console);
    }

    return 0;
}
