#pragma once

#include <PicoStatus.h>
#include <string>

#include <fmt/format.h>

// Helper functions to get readable messages from PICO_STATUS values. Assumes
// that PICO_STATUS is identical for all psXXXX implementations, which is 
// currently the case.

namespace gr::picoscope::detail {

inline constexpr std::string_view status_to_string(PICO_STATUS status)
{
    switch (status) {
    case PICO_OK:
        return "PICO_OK";
    case PICO_MAX_UNITS_OPENED:
        return "PICO_MAX_UNITS_OPENED";
    case PICO_MEMORY_FAIL:
        return "PICO_MEMORY_FAIL";
    case PICO_NOT_FOUND:
        return "PICO_NOT_FOUND";
    case PICO_FW_FAIL:
        return "PICO_FW_FAIL";
    case PICO_OPEN_OPERATION_IN_PROGRESS:
        return "PICO_OPEN_OPERATION_IN_PROGRESS";
    case PICO_OPERATION_FAILED:
        return "PICO_OPERATION_FAILED";
    case PICO_NOT_RESPONDING:
        return "PICO_NOT_RESPONDING";
    case PICO_CONFIG_FAIL:
        return "PICO_CONFIG_FAIL";
    case PICO_KERNEL_DRIVER_TOO_OLD:
        return "PICO_KERNEL_DRIVER_TOO_OLD";
    case PICO_EEPROM_CORRUPT:
        return "PICO_EEPROM_CORRUPT";
    case PICO_OS_NOT_SUPPORTED:
        return "PICO_OS_NOT_SUPPORTED";
    case PICO_INVALID_HANDLE:
        return "PICO_INVALID_HANDLE";
    case PICO_INVALID_PARAMETER:
        return "PICO_INVALID_PARAMETER";
    case PICO_INVALID_TIMEBASE:
        return "PICO_INVALID_TIMEBASE";
    case PICO_INVALID_VOLTAGE_RANGE:
        return "PICO_INVALID_VOLTAGE_RANGE";
    case PICO_INVALID_CHANNEL:
        return "PICO_INVALID_CHANNEL";
    case PICO_INVALID_TRIGGER_CHANNEL:
        return "PICO_INVALID_TRIGGER_CHANNEL";
    case PICO_INVALID_CONDITION_CHANNEL:
        return "PICO_INVALID_CONDITION_CHANNEL";
    case PICO_NO_SIGNAL_GENERATOR:
        return "PICO_NO_SIGNAL_GENERATOR";
    case PICO_STREAMING_FAILED:
        return "PICO_STREAMING_FAILED";
    case PICO_BLOCK_MODE_FAILED:
        return "PICO_BLOCK_MODE_FAILED";
    case PICO_NULL_PARAMETER:
        return "PICO_NULL_PARAMETER";
    case PICO_ETS_MODE_SET:
        return "PICO_ETS_MODE_SET";
    case PICO_DATA_NOT_AVAILABLE:
        return "PICO_DATA_NOT_AVAILABLE";
    case PICO_STRING_BUFFER_TO_SMALL:
        return "PICO_STRING_BUFFER_TO_SMALL";
    case PICO_ETS_NOT_SUPPORTED:
        return "PICO_ETS_NOT_SUPPORTED";
    case PICO_AUTO_TRIGGER_TIME_TO_SHORT:
        return "PICO_AUTO_TRIGGER_TIME_TO_SHORT";
    case PICO_BUFFER_STALL:
        return "PICO_BUFFER_STALL";
    case PICO_TOO_MANY_SAMPLES:
        return "PICO_TOO_MANY_SAMPLES";
    case PICO_TOO_MANY_SEGMENTS:
        return "PICO_TOO_MANY_SEGMENTS";
    case PICO_PULSE_WIDTH_QUALIFIER:
        return "PICO_PULSE_WIDTH_QUALIFIER";
    case PICO_DELAY:
        return "PICO_DELAY";
    case PICO_SOURCE_DETAILS:
        return "PICO_SOURCE_DETAILS";
    case PICO_CONDITIONS:
        return "PICO_CONDITIONS";
    case PICO_USER_CALLBACK:
        return "PICO_USER_CALLBACK";
    case PICO_DEVICE_SAMPLING:
        return "PICO_DEVICE_SAMPLING";
    case PICO_NO_SAMPLES_AVAILABLE:
        return "PICO_NO_SAMPLES_AVAILABLE";
    case PICO_SEGMENT_OUT_OF_RANGE:
        return "PICO_SEGMENT_OUT_OF_RANGE";
    case PICO_BUSY:
        return "PICO_BUSY";
    case PICO_STARTINDEX_INVALID:
        return "PICO_STARTINDEX_INVALID";
    case PICO_INVALID_INFO:
        return "PICO_INVALID_INFO";
    case PICO_INFO_UNAVAILABLE:
        return "PICO_INFO_UNAVAILABLE";
    case PICO_INVALID_SAMPLE_INTERVAL:
        return "PICO_INVALID_SAMPLE_INTERVAL";
    case PICO_TRIGGER_ERROR:
        return "PICO_TRIGGER_ERROR";
    case PICO_MEMORY:
        return "PICO_MEMORY";
    case PICO_SIG_GEN_PARAM:
        return "PICO_SIG_GEN_PARAM";
    case PICO_SHOTS_SWEEPS_WARNING:
        return "PICO_SHOTS_SWEEPS_WARNING";
    case PICO_SIGGEN_TRIGGER_SOURCE:
        return "PICO_SIGGEN_TRIGGER_SOURCE";
    case PICO_AUX_OUTPUT_CONFLICT:
        return "PICO_AUX_OUTPUT_CONFLICT";
    case PICO_AUX_OUTPUT_ETS_CONFLICT:
        return "PICO_AUX_OUTPUT_ETS_CONFLICT";
    case PICO_WARNING_EXT_THRESHOLD_CONFLICT:
        return "PICO_WARNING_EXT_THRESHOLD_CONFLICT";
    case PICO_WARNING_AUX_OUTPUT_CONFLICT:
        return "PICO_WARNING_AUX_OUTPUT_CONFLICT";
    case PICO_SIGGEN_OUTPUT_OVER_VOLTAGE:
        return "PICO_SIGGEN_OUTPUT_OVER_VOLTAGE";
    case PICO_DELAY_NULL:
        return "PICO_DELAY_NULL";
    case PICO_INVALID_BUFFER:
        return "PICO_INVALID_BUFFER";
    case PICO_SIGGEN_OFFSET_VOLTAGE:
        return "PICO_SIGGEN_OFFSET_VOLTAGE";
    case PICO_SIGGEN_PK_TO_PK:
        return "PICO_SIGGEN_PK_TO_PK";
    case PICO_CANCELLED:
        return "PICO_CANCELLED";
    case PICO_SEGMENT_NOT_USED:
        return "PICO_SEGMENT_NOT_USED";
    case PICO_INVALID_CALL:
        return "PICO_INVALID_CALL";
    case PICO_GET_VALUES_INTERRUPTED:
        return "PICO_GET_VALUES_INTERRUPTED";
    case PICO_NOT_USED:
        return "PICO_NOT_USED";
    case PICO_INVALID_SAMPLERATIO:
        return "PICO_INVALID_SAMPLERATIO";
    case PICO_INVALID_STATE:
        return "PICO_INVALID_STATE";
    case PICO_NOT_ENOUGH_SEGMENTS:
        return "PICO_NOT_ENOUGH_SEGMENTS";
    case PICO_DRIVER_FUNCTION:
        return "PICO_DRIVER_FUNCTION";
    case PICO_RESERVED:
        return "PICO_RESERVED";
    case PICO_INVALID_COUPLING:
        return "PICO_INVALID_COUPLING";
    case PICO_BUFFERS_NOT_SET:
        return "PICO_BUFFERS_NOT_SET";
    case PICO_RATIO_MODE_NOT_SUPPORTED:
        return "PICO_RATIO_MODE_NOT_SUPPORTED";
    case PICO_RAPID_NOT_SUPPORT_AGGREGATION:
        return "PICO_RAPID_NOT_SUPPORT_AGGREGATION";
    case PICO_INVALID_TRIGGER_PROPERTY:
        return "PICO_INVALID_TRIGGER_PROPERTY";
    case PICO_INTERFACE_NOT_CONNECTED:
        return "PICO_INTERFACE_NOT_CONNECTED";
    case PICO_RESISTANCE_AND_PROBE_NOT_ALLOWED:
        return "PICO_RESISTANCE_AND_PROBE_NOT_ALLOWED";
    case PICO_POWER_FAILED:
        return "PICO_POWER_FAILED";
    case PICO_SIGGEN_WAVEFORM_SETUP_FAILED:
        return "PICO_SIGGEN_WAVEFORM_SETUP_FAILED";
    case PICO_FPGA_FAIL:
        return "PICO_FPGA_FAIL";
    case PICO_POWER_MANAGER:
        return "PICO_POWER_MANAGER";
    case PICO_INVALID_ANALOGUE_OFFSET:
        return "PICO_INVALID_ANALOGUE_OFFSET";
    case PICO_PLL_LOCK_FAILED:
        return "PICO_PLL_LOCK_FAILED";
    case PICO_ANALOG_BOARD:
        return "PICO_ANALOG_BOARD";
    case PICO_CONFIG_FAIL_AWG:
        return "PICO_CONFIG_FAIL_AWG";
    case PICO_INITIALISE_FPGA:
        return "PICO_INITIALISE_FPGA";
    case PICO_EXTERNAL_FREQUENCY_INVALID:
        return "PICO_EXTERNAL_FREQUENCY_INVALID";
    case PICO_CLOCK_CHANGE_ERROR:
        return "PICO_CLOCK_CHANGE_ERROR";
    case PICO_TRIGGER_AND_EXTERNAL_CLOCK_CLASH:
        return "PICO_TRIGGER_AND_EXTERNAL_CLOCK_CLASH";
    case PICO_PWQ_AND_EXTERNAL_CLOCK_CLASH:
        return "PICO_PWQ_AND_EXTERNAL_CLOCK_CLASH";
    case PICO_UNABLE_TO_OPEN_SCALING_FILE:
        return "PICO_UNABLE_TO_OPEN_SCALING_FILE";
    case PICO_MEMORY_CLOCK_FREQUENCY:
        return "PICO_MEMORY_CLOCK_FREQUENCY";
    case PICO_I2C_NOT_RESPONDING:
        return "PICO_I2C_NOT_RESPONDING";
    case PICO_NO_CAPTURES_AVAILABLE:
        return "PICO_NO_CAPTURES_AVAILABLE";
    case PICO_TOO_MANY_TRIGGER_CHANNELS_IN_USE:
        return "PICO_TOO_MANY_TRIGGER_CHANNELS_IN_USE";
    case PICO_INVALID_TRIGGER_DIRECTION:
        return "PICO_INVALID_TRIGGER_DIRECTION";
    case PICO_INVALID_TRIGGER_STATES:
        return "PICO_INVALID_TRIGGER_STATES";
    case PICO_NOT_USED_IN_THIS_CAPTURE_MODE:
        return "PICO_NOT_USED_IN_THIS_CAPTURE_MODE";
    case PICO_GET_DATA_ACTIVE:
        return "PICO_GET_DATA_ACTIVE";
    case PICO_IP_NETWORKED:
        return "PICO_IP_NETWORKED";
    case PICO_INVALID_IP_ADDRESS:
        return "PICO_INVALID_IP_ADDRESS";
    case PICO_IPSOCKET_FAILED:
        return "PICO_IPSOCKET_FAILED";
    case PICO_IPSOCKET_TIMEDOUT:
        return "PICO_IPSOCKET_TIMEDOUT";
    case PICO_SETTINGS_FAILED:
        return "PICO_SETTINGS_FAILED";
    case PICO_NETWORK_FAILED:
        return "PICO_NETWORK_FAILED";
    case PICO_WS2_32_DLL_NOT_LOADED:
        return "PICO_WS2_32_DLL_NOT_LOADED";
    case PICO_INVALID_IP_PORT:
        return "PICO_INVALID_IP_PORT";
    case PICO_COUPLING_NOT_SUPPORTED:
        return "PICO_COUPLING_NOT_SUPPORTED";
    case PICO_BANDWIDTH_NOT_SUPPORTED:
        return "PICO_BANDWIDTH_NOT_SUPPORTED";
    case PICO_INVALID_BANDWIDTH:
        return "PICO_INVALID_BANDWIDTH";
    case PICO_AWG_NOT_SUPPORTED:
        return "PICO_AWG_NOT_SUPPORTED";
    case PICO_ETS_NOT_RUNNING:
        return "PICO_ETS_NOT_RUNNING";
    case PICO_SIG_GEN_WHITENOISE_NOT_SUPPORTED:
        return "PICO_SIG_GEN_WHITENOISE_NOT_SUPPORTED";
    case PICO_SIG_GEN_WAVETYPE_NOT_SUPPORTED:
        return "PICO_SIG_GEN_WAVETYPE_NOT_SUPPORTED";
    case PICO_INVALID_DIGITAL_PORT:
        return "PICO_INVALID_DIGITAL_PORT";
    case PICO_INVALID_DIGITAL_CHANNEL:
        return "PICO_INVALID_DIGITAL_CHANNEL";
    case PICO_INVALID_DIGITAL_TRIGGER_DIRECTION:
        return "PICO_INVALID_DIGITAL_TRIGGER_DIRECTION";
    case PICO_SIG_GEN_PRBS_NOT_SUPPORTED:
        return "PICO_SIG_GEN_PRBS_NOT_SUPPORTED";
    case PICO_ETS_NOT_AVAILABLE_WITH_LOGIC_CHANNELS:
        return "PICO_ETS_NOT_AVAILABLE_WITH_LOGIC_CHANNELS";
    case PICO_WARNING_REPEAT_VALUE:
        return "PICO_WARNING_REPEAT_VALUE";
    case PICO_POWER_SUPPLY_CONNECTED:
        return "PICO_POWER_SUPPLY_CONNECTED";
    case PICO_POWER_SUPPLY_NOT_CONNECTED:
        return "PICO_POWER_SUPPLY_NOT_CONNECTED";
    case PICO_POWER_SUPPLY_REQUEST_INVALID:
        return "PICO_POWER_SUPPLY_REQUEST_INVALID";
    case PICO_POWER_SUPPLY_UNDERVOLTAGE:
        return "PICO_POWER_SUPPLY_UNDERVOLTAGE";
    case PICO_CAPTURING_DATA:
        return "PICO_CAPTURING_DATA";
    case PICO_USB3_0_DEVICE_NON_USB3_0_PORT:
        return "PICO_USB3_0_DEVICE_NON_USB3_0_PORT";
    case PICO_NOT_SUPPORTED_BY_THIS_DEVICE:
        return "PICO_NOT_SUPPORTED_BY_THIS_DEVICE";
    case PICO_INVALID_DEVICE_RESOLUTION:
        return "PICO_INVALID_DEVICE_RESOLUTION";
    case PICO_INVALID_NUMBER_CHANNELS_FOR_RESOLUTION:
        return "PICO_INVALID_NUMBER_CHANNELS_FOR_RESOLUTION";
    case PICO_CHANNEL_DISABLED_DUE_TO_USB_POWERED:
        return "PICO_CHANNEL_DISABLED_DUE_TO_USB_POWERED";
    case PICO_SIGGEN_DC_VOLTAGE_NOT_CONFIGURABLE:
        return "PICO_SIGGEN_DC_VOLTAGE_NOT_CONFIGURABLE";
    case PICO_NO_TRIGGER_ENABLED_FOR_TRIGGER_IN_PRE_TRIG:
        return "PICO_NO_TRIGGER_ENABLED_FOR_TRIGGER_IN_PRE_TRIG";
    case PICO_TRIGGER_WITHIN_PRE_TRIG_NOT_ARMED:
        return "PICO_TRIGGER_WITHIN_PRE_TRIG_NOT_ARMED";
    case PICO_TRIGGER_WITHIN_PRE_NOT_ALLOWED_WITH_DELAY:
        return "PICO_TRIGGER_WITHIN_PRE_NOT_ALLOWED_WITH_DELAY";
    case PICO_TRIGGER_INDEX_UNAVAILABLE:
        return "PICO_TRIGGER_INDEX_UNAVAILABLE";
    case PICO_AWG_CLOCK_FREQUENCY:
        return "PICO_AWG_CLOCK_FREQUENCY";
    case PICO_TOO_MANY_CHANNELS_IN_USE:
        return "PICO_TOO_MANY_CHANNELS_IN_USE";
    case PICO_NULL_CONDITIONS:
        return "PICO_NULL_CONDITIONS";
    case PICO_DUPLICATE_CONDITION_SOURCE:
        return "PICO_DUPLICATE_CONDITION_SOURCE";
    case PICO_INVALID_CONDITION_INFO:
        return "PICO_INVALID_CONDITION_INFO";
    case PICO_SETTINGS_READ_FAILED:
        return "PICO_SETTINGS_READ_FAILED";
    case PICO_SETTINGS_WRITE_FAILED:
        return "PICO_SETTINGS_WRITE_FAILED";
    case PICO_ARGUMENT_OUT_OF_RANGE:
        return "PICO_ARGUMENT_OUT_OF_RANGE";
    case PICO_HARDWARE_VERSION_NOT_SUPPORTED:
        return "PICO_HARDWARE_VERSION_NOT_SUPPORTED";
    case PICO_DIGITAL_HARDWARE_VERSION_NOT_SUPPORTED:
        return "PICO_DIGITAL_HARDWARE_VERSION_NOT_SUPPORTED";
    case PICO_ANALOGUE_HARDWARE_VERSION_NOT_SUPPORTED:
        return "PICO_ANALOGUE_HARDWARE_VERSION_NOT_SUPPORTED";
    case PICO_UNABLE_TO_CONVERT_TO_RESISTANCE:
        return "PICO_UNABLE_TO_CONVERT_TO_RESISTANCE";
    case PICO_DUPLICATED_CHANNEL:
        return "PICO_DUPLICATED_CHANNEL";
    case PICO_INVALID_RESISTANCE_CONVERSION:
        return "PICO_INVALID_RESISTANCE_CONVERSION";
    case PICO_INVALID_VALUE_IN_MAX_BUFFER:
        return "PICO_INVALID_VALUE_IN_MAX_BUFFER";
    case PICO_INVALID_VALUE_IN_MIN_BUFFER:
        return "PICO_INVALID_VALUE_IN_MIN_BUFFER";
    case PICO_SIGGEN_FREQUENCY_OUT_OF_RANGE:
        return "PICO_SIGGEN_FREQUENCY_OUT_OF_RANGE";
    case PICO_EEPROM2_CORRUPT:
        return "PICO_EEPROM2_CORRUPT";
    case PICO_EEPROM2_FAIL:
        return "PICO_EEPROM2_FAIL";
    case PICO_SERIAL_BUFFER_TOO_SMALL:
        return "PICO_SERIAL_BUFFER_TOO_SMALL";
    case PICO_SIGGEN_TRIGGER_AND_EXTERNAL_CLOCK_CLASH:
        return "PICO_SIGGEN_TRIGGER_AND_EXTERNAL_CLOCK_CLASH";
    case PICO_WARNING_SIGGEN_AUXIO_TRIGGER_DISABLED:
        return "PICO_WARNING_SIGGEN_AUXIO_TRIGGER_DISABLED";
    case PICO_SIGGEN_GATING_AUXIO_NOT_AVAILABLE:
        return "PICO_SIGGEN_GATING_AUXIO_NOT_AVAILABLE";
    case PICO_SIGGEN_GATING_AUXIO_ENABLED:
        return "PICO_SIGGEN_GATING_AUXIO_ENABLED";
    case PICO_RESOURCE_ERROR:
        return "PICO_RESOURCE_ERROR";
    case PICO_TEMPERATURE_TYPE_INVALID:
        return "PICO_TEMPERATURE_TYPE_INVALID";
    case PICO_TEMPERATURE_TYPE_NOT_SUPPORTED:
        return "PICO_TEMPERATURE_TYPE_NOT_SUPPORTED";
    case PICO_TIMEOUT:
        return "PICO_TIMEOUT";
    case PICO_DEVICE_NOT_FUNCTIONING:
        return "PICO_DEVICE_NOT_FUNCTIONING";
    case PICO_INTERNAL_ERROR:
        return "PICO_INTERNAL_ERROR";
    case PICO_MULTIPLE_DEVICES_FOUND:
        return "PICO_MULTIPLE_DEVICES_FOUND";
    case PICO_WARNING_NUMBER_OF_SEGMENTS_REDUCED:
        return "PICO_WARNING_NUMBER_OF_SEGMENTS_REDUCED";
    case PICO_CAL_PINS_STATES:
        return "PICO_CAL_PINS_STATES";
    case PICO_CAL_PINS_FREQUENCY:
        return "PICO_CAL_PINS_FREQUENCY";
    case PICO_CAL_PINS_AMPLITUDE:
        return "PICO_CAL_PINS_AMPLITUDE";
    case PICO_CAL_PINS_WAVETYPE:
        return "PICO_CAL_PINS_WAVETYPE";
    case PICO_CAL_PINS_OFFSET:
        return "PICO_CAL_PINS_OFFSET";
    case PICO_PROBE_FAULT:
        return "PICO_PROBE_FAULT";
    case PICO_PROBE_IDENTITY_UNKNOWN:
        return "PICO_PROBE_IDENTITY_UNKNOWN";
    case PICO_PROBE_POWER_DC_POWER_SUPPLY_REQUIRED:
        return "PICO_PROBE_POWER_DC_POWER_SUPPLY_REQUIRED";
    case PICO_PROBE_NOT_POWERED_WITH_DC_POWER_SUPPLY:
        return "PICO_PROBE_NOT_POWERED_WITH_DC_POWER_SUPPLY";
    case PICO_PROBE_CONFIG_FAILURE:
        return "PICO_PROBE_CONFIG_FAILURE";
    case PICO_PROBE_INTERACTION_CALLBACK:
        return "PICO_PROBE_INTERACTION_CALLBACK";
    case PICO_UNKNOWN_INTELLIGENT_PROBE:
        return "PICO_UNKNOWN_INTELLIGENT_PROBE";
    case PICO_INTELLIGENT_PROBE_CORRUPT:
        return "PICO_INTELLIGENT_PROBE_CORRUPT";
    case PICO_PROBE_COLLECTION_NOT_STARTED:
        return "PICO_PROBE_COLLECTION_NOT_STARTED";
    case PICO_PROBE_POWER_CONSUMPTION_EXCEEDED:
        return "PICO_PROBE_POWER_CONSUMPTION_EXCEEDED";
    case PICO_WARNING_PROBE_CHANNEL_OUT_OF_SYNC:
        return "PICO_WARNING_PROBE_CHANNEL_OUT_OF_SYNC";
    case PICO_DEVICE_TIME_STAMP_RESET:
        return "PICO_DEVICE_TIME_STAMP_RESET";
    case PICO_WATCHDOGTIMER:
        return "PICO_WATCHDOGTIMER";
    case PICO_IPP_NOT_FOUND:
        return "PICO_IPP_NOT_FOUND";
    case PICO_IPP_NO_FUNCTION:
        return "PICO_IPP_NO_FUNCTION";
    case PICO_IPP_ERROR:
        return "PICO_IPP_ERROR";
    case PICO_SHADOW_CAL_NOT_AVAILABLE:
        return "PICO_SHADOW_CAL_NOT_AVAILABLE";
    case PICO_SHADOW_CAL_DISABLED:
        return "PICO_SHADOW_CAL_DISABLED";
    case PICO_SHADOW_CAL_ERROR:
        return "PICO_SHADOW_CAL_ERROR";
    case PICO_SHADOW_CAL_CORRUPT:
        return "PICO_SHADOW_CAL_CORRUPT";
    case PICO_DEVICE_MEMORY_OVERFLOW:
        return "PICO_DEVICE_MEMORY_OVERFLOW";
    default:
        return "unknown status";
    }
}

inline constexpr std::string_view status_to_string_verbose(PICO_STATUS status)
{
    switch (status) {
    case PICO_OK:
        return "The PicoScope is functioning correctly.";
    case PICO_MAX_UNITS_OPENED:
        return "An attempt has been made to open more than <API>_MAX_UNITS.";
    case PICO_MEMORY_FAIL:
        return "Not enough memory could be allocated on the host machine.";
    case PICO_NOT_FOUND:
        return "No Pico Technology device could be found.";
    case PICO_FW_FAIL:
        return "Unable to download firmware.";
    case PICO_OPEN_OPERATION_IN_PROGRESS:
        return "The driver is busy opening a device.";
    case PICO_OPERATION_FAILED:
        return "An unspecified failure occurred.";
    case PICO_NOT_RESPONDING:
        return "The PicoScope is not responding to commands from the PC.";
    case PICO_CONFIG_FAIL:
        return "The configuration information in the PicoScope is corrupt or missing.";
    case PICO_KERNEL_DRIVER_TOO_OLD:
        return "The picopp.sys file is too old to be used with the device driver.";
    case PICO_EEPROM_CORRUPT:
        return "The EEPROM has become corrupt, so the device will use a default setting.";
    case PICO_OS_NOT_SUPPORTED:
        return "The operating system on the PC is not supported by this driver.";
    case PICO_INVALID_HANDLE:
        return "There is no device with the handle value passed.";
    case PICO_INVALID_PARAMETER:
        return "A parameter value is not valid.";
    case PICO_INVALID_TIMEBASE:
        return "The timebase is not supported or is invalid.";
    case PICO_INVALID_VOLTAGE_RANGE:
        return "The voltage range is not supported or is invalid.";
    case PICO_INVALID_CHANNEL:
        return "The channel number is not valid on this device or no channels have been "
               "set.";
    case PICO_INVALID_TRIGGER_CHANNEL:
        return "The channel set for a trigger is not available on this device.";
    case PICO_INVALID_CONDITION_CHANNEL:
        return "The channel set for a condition is not available on this device.";
    case PICO_NO_SIGNAL_GENERATOR:
        return "The device does not have a signal generator.";
    case PICO_STREAMING_FAILED:
        return "Streaming has failed to start or has stopped without user request.";
    case PICO_BLOCK_MODE_FAILED:
        return "Block failed to start - a parameter may have been set wrongly.";
    case PICO_NULL_PARAMETER:
        return "A parameter that was required is NULL.";
    case PICO_ETS_MODE_SET:
        return "The current functionality is not available while using ETS capture mode.";
    case PICO_DATA_NOT_AVAILABLE:
        return "No data is available from a run block call.";
    case PICO_STRING_BUFFER_TO_SMALL:
        return "The buffer passed for the information was too small.";
    case PICO_ETS_NOT_SUPPORTED:
        return "ETS is not supported on this device.";
    case PICO_AUTO_TRIGGER_TIME_TO_SHORT:
        return "The auto trigger time is less than the time it will take to collect the "
               "pre-trigger data.";
    case PICO_BUFFER_STALL:
        return "The collection of data has stalled as unread data would be overwritten.";
    case PICO_TOO_MANY_SAMPLES:
        return "Number of samples requested is more than available in the current memory "
               "segment.";
    case PICO_TOO_MANY_SEGMENTS:
        return "Not possible to create number of segments requested.";
    case PICO_PULSE_WIDTH_QUALIFIER:
        return "A null pointer has been passed in the trigger function or one of the "
               "parameters is out of range.";
    case PICO_DELAY:
        return "One or more of the hold-off parameters are out of range.";
    case PICO_SOURCE_DETAILS:
        return "One or more of the source details are incorrect.";
    case PICO_CONDITIONS:
        return "One or more of the conditions are incorrect.";
    case PICO_USER_CALLBACK:
        return "The driver's thread is currently in the <API>Ready callback function and "
               "therefore the action cannot be carried out.";
    case PICO_DEVICE_SAMPLING:
        return "An attempt is being made to get stored data while streaming. Either stop "
               "streaming by calling <API>Stop, or use <API>GetStreamingLatestValues.";
    case PICO_NO_SAMPLES_AVAILABLE:
        return "Data is unavailable because a run has not been completed.";
    case PICO_SEGMENT_OUT_OF_RANGE:
        return "The memory segment index is out of range.";
    case PICO_BUSY:
        return "The device is busy so data cannot be returned yet.";
    case PICO_STARTINDEX_INVALID:
        return "The start time to get stored data is out of range.";
    case PICO_INVALID_INFO:
        return "The information number requested is not a valid number.";
    case PICO_INFO_UNAVAILABLE:
        return "The handle is invalid so no information is available about the device. "
               "Only PICO_DRIVER_VERSION is available.";
    case PICO_INVALID_SAMPLE_INTERVAL:
        return "The sample interval selected for streaming is out of range.";
    case PICO_TRIGGER_ERROR:
        return "ETS is set but no trigger has been set. A trigger setting is required "
               "for ETS.";
    case PICO_MEMORY:
        return "Driver cannot allocate memory.";
    case PICO_SIG_GEN_PARAM:
        return "Incorrect parameter passed to the signal generator.";
    case PICO_SHOTS_SWEEPS_WARNING:
        return "Conflict between the shots and sweeps parameters sent to the signal "
               "generator.";
    case PICO_SIGGEN_TRIGGER_SOURCE:
        return "A software trigger has been sent but the trigger source is not a "
               "software trigger.";
    case PICO_AUX_OUTPUT_CONFLICT:
        return "An <API>SetTrigger call has found a conflict between the trigger source "
               "and the AUX output enable.";
    case PICO_AUX_OUTPUT_ETS_CONFLICT:
        return "ETS mode is being used and AUX is set as an input.";
    case PICO_WARNING_EXT_THRESHOLD_CONFLICT:
        return "Attempt to set different EXT input thresholds set for signal generator "
               "and oscilloscope trigger.";
    case PICO_WARNING_AUX_OUTPUT_CONFLICT:
        return "An <API>SetTrigger... function has set AUX as an output and the signal "
               "generator is using it as a trigger.";
    case PICO_SIGGEN_OUTPUT_OVER_VOLTAGE:
        return "The combined peak to peak voltage and the analog offset voltage exceed "
               "the maximum voltage the signal generator can produce.";
    case PICO_DELAY_NULL:
        return "NULL pointer passed as delay parameter.";
    case PICO_INVALID_BUFFER:
        return "The buffers for overview data have not been set while streaming.";
    case PICO_SIGGEN_OFFSET_VOLTAGE:
        return "The analog offset voltage is out of range.";
    case PICO_SIGGEN_PK_TO_PK:
        return "The analog peak-to-peak voltage is out of range.";
    case PICO_CANCELLED:
        return "A block collection has been cancelled.";
    case PICO_SEGMENT_NOT_USED:
        return "The segment index is not currently being used.";
    case PICO_INVALID_CALL:
        return "The wrong GetValues function has been called for the collection mode in "
               "use.";
    case PICO_GET_VALUES_INTERRUPTED:
        return "";
    case PICO_NOT_USED:
        return "The function is not available.";
    case PICO_INVALID_SAMPLERATIO:
        return "The aggregation ratio requested is out of range.";
    case PICO_INVALID_STATE:
        return "Device is in an invalid state.";
    case PICO_NOT_ENOUGH_SEGMENTS:
        return "The number of segments allocated is fewer than the number of captures "
               "requested.";
    case PICO_DRIVER_FUNCTION:
        return "A driver function has already been called and not yet finished. Only one "
               "call to the driver can be made at any one time.";
    case PICO_RESERVED:
        return "Not used";
    case PICO_INVALID_COUPLING:
        return "An invalid coupling type was specified in <API>SetChannel.";
    case PICO_BUFFERS_NOT_SET:
        return "An attempt was made to get data before a data buffer was defined.";
    case PICO_RATIO_MODE_NOT_SUPPORTED:
        return "The selected downsampling mode (used for data reduction) is not allowed.";
    case PICO_RAPID_NOT_SUPPORT_AGGREGATION:
        return "Aggregation was requested in rapid block mode.";
    case PICO_INVALID_TRIGGER_PROPERTY:
        return "An invalid parameter was passed to <API>SetTriggerChannelProperties.";
    case PICO_INTERFACE_NOT_CONNECTED:
        return "The driver was unable to contact the oscilloscope.";
    case PICO_RESISTANCE_AND_PROBE_NOT_ALLOWED:
        return "Resistance-measuring mode is not allowed in conjunction with the "
               "specified probe.";
    case PICO_POWER_FAILED:
        return "The device was unexpectedly powered down.";
    case PICO_SIGGEN_WAVEFORM_SETUP_FAILED:
        return "A problem occurred in <API>SetSigGenBuiltIn or <API>SetSigGenArbitrary.";
    case PICO_FPGA_FAIL:
        return "FPGA not successfully set up.";
    case PICO_POWER_MANAGER:
        return "";
    case PICO_INVALID_ANALOGUE_OFFSET:
        return "An impossible analog offset value was specified in <API>SetChannel.";
    case PICO_PLL_LOCK_FAILED:
        return "There is an error within the device hardware.";
    case PICO_ANALOG_BOARD:
        return "There is an error within the device hardware.";
    case PICO_CONFIG_FAIL_AWG:
        return "Unable to configure the signal generator.";
    case PICO_INITIALISE_FPGA:
        return "The FPGA cannot be initialized, so unit cannot be opened.";
    case PICO_EXTERNAL_FREQUENCY_INVALID:
        return "The frequency for the external clock is not within 15% of the nominal "
               "value.";
    case PICO_CLOCK_CHANGE_ERROR:
        return "The FPGA could not lock the clock signal.";
    case PICO_TRIGGER_AND_EXTERNAL_CLOCK_CLASH:
        return "You are trying to configure the AUX input as both a trigger and a "
               "reference clock.";
    case PICO_PWQ_AND_EXTERNAL_CLOCK_CLASH:
        return "You are trying to congfigure the AUX input as both a pulse width "
               "qualifier and a reference clock.";
    case PICO_UNABLE_TO_OPEN_SCALING_FILE:
        return "The requested scaling file cannot be opened.";
    case PICO_MEMORY_CLOCK_FREQUENCY:
        return "The frequency of the memory is reporting incorrectly.";
    case PICO_I2C_NOT_RESPONDING:
        return "The I2C that is being actioned is not responding to requests.";
    case PICO_NO_CAPTURES_AVAILABLE:
        return "There are no captures available and therefore no data can be returned.";
    case PICO_TOO_MANY_TRIGGER_CHANNELS_IN_USE:
        return "The number of trigger channels is greater than 4, except for a PS4824 "
               "where 8 channels are allowed for rising/falling/rising_or_falling "
               "trigger directions.";
    case PICO_INVALID_TRIGGER_DIRECTION:
        return "When more than 4 trigger channels are set on a PS4824 and the direction "
               "is out of range.";
    case PICO_INVALID_TRIGGER_STATES:
        return "When more than 4 trigger channels are set and their trigger condition "
               "states are not <API>_CONDITION_TRUE.";
    case PICO_NOT_USED_IN_THIS_CAPTURE_MODE:
        return "The capture mode the device is currently running in does not support the "
               "current request.";
    case PICO_GET_DATA_ACTIVE:
        return "";
    case PICO_IP_NETWORKED:
        return "The device is currently connected via the IP Network socket and thus the "
               "call made is not supported.";
    case PICO_INVALID_IP_ADDRESS:
        return "An incorrect IP address has been passed to the driver.";
    case PICO_IPSOCKET_FAILED:
        return "The IP socket has failed.";
    case PICO_IPSOCKET_TIMEDOUT:
        return "The IP socket has timed out.";
    case PICO_SETTINGS_FAILED:
        return "Failed to apply the requested settings.";
    case PICO_NETWORK_FAILED:
        return "The network connection has failed.";
    case PICO_WS2_32_DLL_NOT_LOADED:
        return "Unable to load the WS2 DLL.";
    case PICO_INVALID_IP_PORT:
        return "The specified IP port is invalid.";
    case PICO_COUPLING_NOT_SUPPORTED:
        return "The type of coupling requested is not supported on the opened device.";
    case PICO_BANDWIDTH_NOT_SUPPORTED:
        return "Bandwidth limiting is not supported on the opened device.";
    case PICO_INVALID_BANDWIDTH:
        return "The value requested for the bandwidth limit is out of range.";
    case PICO_AWG_NOT_SUPPORTED:
        return "The arbitrary waveform generator is not supported by the opened device.";
    case PICO_ETS_NOT_RUNNING:
        return "Data has been requested with ETS mode set but run block has not been "
               "called, or stop has been called.";
    case PICO_SIG_GEN_WHITENOISE_NOT_SUPPORTED:
        return "White noise output is not supported on the opened device.";
    case PICO_SIG_GEN_WAVETYPE_NOT_SUPPORTED:
        return "The wave type requested is not supported by the opened device.";
    case PICO_INVALID_DIGITAL_PORT:
        return "The requested digital port number is out of range (MSOs only).";
    case PICO_INVALID_DIGITAL_CHANNEL:
        return "The digital channel is not in the range <API>_DIGITAL_CHANNEL0 "
               "to<API>_DIGITAL_CHANNEL15, the digital channels that are supported.";
    case PICO_INVALID_DIGITAL_TRIGGER_DIRECTION:
        return "The digital trigger direction is not a valid trigger direction and "
               "should be equal in value to one of the <API>_DIGITAL_DIRECTION "
               "enumerations.";
    case PICO_SIG_GEN_PRBS_NOT_SUPPORTED:
        return "Signal generator does not generate pseudo-random binary sequence.";
    case PICO_ETS_NOT_AVAILABLE_WITH_LOGIC_CHANNELS:
        return "When a digital port is enabled, ETS sample mode is not available for "
               "use.";
    case PICO_WARNING_REPEAT_VALUE:
        return "";
    case PICO_POWER_SUPPLY_CONNECTED:
        return "4-channel scopes only: The DC power supply is connected.";
    case PICO_POWER_SUPPLY_NOT_CONNECTED:
        return "4-channel scopes only: The DC power supply is not connected.";
    case PICO_POWER_SUPPLY_REQUEST_INVALID:
        return "Incorrect power mode passed for current power source.";
    case PICO_POWER_SUPPLY_UNDERVOLTAGE:
        return "The supply voltage from the USB source is too low.";
    case PICO_CAPTURING_DATA:
        return "The oscilloscope is in the process of capturing data.";
    case PICO_USB3_0_DEVICE_NON_USB3_0_PORT:
        return "A USB 3.0 device is connected to a non-USB 3.0 port.";
    case PICO_NOT_SUPPORTED_BY_THIS_DEVICE:
        return "A function has been called that is not supported by the current device.";
    case PICO_INVALID_DEVICE_RESOLUTION:
        return "The device resolution is invalid (out of range).";
    case PICO_INVALID_NUMBER_CHANNELS_FOR_RESOLUTION:
        return "The number of channels that can be enabled is limited in 15 and 16-bit "
               "modes. (Flexible Resolution Oscilloscopes only)";
    case PICO_CHANNEL_DISABLED_DUE_TO_USB_POWERED:
        return "USB power not sufficient for all requested channels.";
    case PICO_SIGGEN_DC_VOLTAGE_NOT_CONFIGURABLE:
        return "The signal generator does not have a configurable DC offset.";
    case PICO_NO_TRIGGER_ENABLED_FOR_TRIGGER_IN_PRE_TRIG:
        return "An attempt has been made to define pre-trigger delay without first "
               "enabling a trigger.";
    case PICO_TRIGGER_WITHIN_PRE_TRIG_NOT_ARMED:
        return "An attempt has been made to define pre-trigger delay without first "
               "arming a trigger.";
    case PICO_TRIGGER_WITHIN_PRE_NOT_ALLOWED_WITH_DELAY:
        return "Pre-trigger delay and post-trigger delay cannot be used at the same "
               "time.";
    case PICO_TRIGGER_INDEX_UNAVAILABLE:
        return "The array index points to a nonexistent trigger.";
    case PICO_AWG_CLOCK_FREQUENCY:
        return "";
    case PICO_TOO_MANY_CHANNELS_IN_USE:
        return "There are more 4 analog channels with a trigger condition set.";
    case PICO_NULL_CONDITIONS:
        return "The condition parameter is a null pointer.";
    case PICO_DUPLICATE_CONDITION_SOURCE:
        return "There is more than one condition pertaining to the same channel.";
    case PICO_INVALID_CONDITION_INFO:
        return "The parameter relating to condition information is out of range.";
    case PICO_SETTINGS_READ_FAILED:
        return "Reading the metadata has failed.";
    case PICO_SETTINGS_WRITE_FAILED:
        return "Writing the metadata has failed.";
    case PICO_ARGUMENT_OUT_OF_RANGE:
        return "A parameter has a value out of the expected range.";
    case PICO_HARDWARE_VERSION_NOT_SUPPORTED:
        return "The driver does not support the hardware variant connected.";
    case PICO_DIGITAL_HARDWARE_VERSION_NOT_SUPPORTED:
        return "The driver does not support the digital hardware variant connected.";
    case PICO_ANALOGUE_HARDWARE_VERSION_NOT_SUPPORTED:
        return "The driver does not support the analog hardware variant connected.";
    case PICO_UNABLE_TO_CONVERT_TO_RESISTANCE:
        return "Converting a channel's ADC value to resistance has failed.";
    case PICO_DUPLICATED_CHANNEL:
        return "The channel is listed more than once in the function call.";
    case PICO_INVALID_RESISTANCE_CONVERSION:
        return "The range cannot have resistance conversion applied.";
    case PICO_INVALID_VALUE_IN_MAX_BUFFER:
        return "An invalid value is in the max buffer.";
    case PICO_INVALID_VALUE_IN_MIN_BUFFER:
        return "An invalid value is in the min buffer.";
    case PICO_SIGGEN_FREQUENCY_OUT_OF_RANGE:
        return "When calculating the frequency for phase conversion, the frequency is "
               "greater than that supported by the current variant.";
    case PICO_EEPROM2_CORRUPT:
        return "The device's EEPROM is corrupt. Contact Pico Technology support: "
               "https://www.picotech.com/tech-support.";
    case PICO_EEPROM2_FAIL:
        return "The EEPROM has failed.";
    case PICO_SERIAL_BUFFER_TOO_SMALL:
        return "The serial buffer is too small for the required information.";
    case PICO_SIGGEN_TRIGGER_AND_EXTERNAL_CLOCK_CLASH:
        return "The signal generator trigger and the external clock have both been set. "
               "This is not allowed.";
    case PICO_WARNING_SIGGEN_AUXIO_TRIGGER_DISABLED:
        return "The AUX trigger was enabled and the external clock has been enabled, so "
               "the AUX has been automatically disabled.";
    case PICO_SIGGEN_GATING_AUXIO_NOT_AVAILABLE:
        return "The AUX I/O was set as a scope trigger and is now being set as a signal "
               "generator gating trigger. This is not allowed.";
    case PICO_SIGGEN_GATING_AUXIO_ENABLED:
        return "The AUX I/O was set by the signal generator as a gating trigger and is "
               "now being set as a scope trigger. This is not allowed.";
    case PICO_RESOURCE_ERROR:
        return "A resource has failed to initialise ";
    case PICO_TEMPERATURE_TYPE_INVALID:
        return "The temperature type is out of range";
    case PICO_TEMPERATURE_TYPE_NOT_SUPPORTED:
        return "A requested temperature type is not supported on this device";
    case PICO_TIMEOUT:
        return "A read/write to the device has timed out";
    case PICO_DEVICE_NOT_FUNCTIONING:
        return "The device cannot be connected correctly";
    case PICO_INTERNAL_ERROR:
        return "The driver has experienced an unknown error and is unable to recover "
               "from this error";
    case PICO_MULTIPLE_DEVICES_FOUND:
        return "Used when opening units via IP and more than multiple units have the "
               "same ip address";
    case PICO_WARNING_NUMBER_OF_SEGMENTS_REDUCED:
        return "";
    case PICO_CAL_PINS_STATES:
        return "the calibration pin states argument is out of range";
    case PICO_CAL_PINS_FREQUENCY:
        return "the calibration pin frequency argument is out of range";
    case PICO_CAL_PINS_AMPLITUDE:
        return "the calibration pin amplitude argument is out of range";
    case PICO_CAL_PINS_WAVETYPE:
        return "the calibration pin wavetype argument is out of range";
    case PICO_CAL_PINS_OFFSET:
        return "the calibration pin offset argument is out of range";
    case PICO_PROBE_FAULT:
        return "the probe's identity has a problem";
    case PICO_PROBE_IDENTITY_UNKNOWN:
        return "the probe has not been identified";
    case PICO_PROBE_POWER_DC_POWER_SUPPLY_REQUIRED:
        return "enabling the probe would cause the device to exceed the allowable "
               "current limit";
    case PICO_PROBE_NOT_POWERED_WITH_DC_POWER_SUPPLY:
        return "the DC power supply is connected; enabling the probe would cause the "
               "device to exceed the allowable current limit";
    case PICO_PROBE_CONFIG_FAILURE:
        return "failed to complete probe configuration";
    case PICO_PROBE_INTERACTION_CALLBACK:
        return "failed to set the callback function, as currently in current callback "
               "function";
    case PICO_UNKNOWN_INTELLIGENT_PROBE:
        return "the probe has been verified but is not known on this driver";
    case PICO_INTELLIGENT_PROBE_CORRUPT:
        return "the intelligent probe cannot be verified";
    case PICO_PROBE_COLLECTION_NOT_STARTED:
        return "the callback is null, probe collection will only start when first "
               "callback is a none null pointer";
    case PICO_PROBE_POWER_CONSUMPTION_EXCEEDED:
        return "the current drawn by the probe(s) has exceeded the allowed limit";
    case PICO_WARNING_PROBE_CHANNEL_OUT_OF_SYNC:
        return "the channel range limits have changed due to connecting or disconnecting "
               "a probe the channel has been enabled";
    case PICO_DEVICE_TIME_STAMP_RESET:
        return "The time stamp per waveform segment has been reset.";
    case PICO_WATCHDOGTIMER:
        return "An internal error has occurred and a watchdog timer has been called.";
    case PICO_IPP_NOT_FOUND:
        return "The picoipp.dll has not been found.";
    case PICO_IPP_NO_FUNCTION:
        return "A function in the picoipp.dll does not exist.";
    case PICO_IPP_ERROR:
        return "The Pico IPP call has failed.";
    case PICO_SHADOW_CAL_NOT_AVAILABLE:
        return "Shadow calibration is not available on this device.";
    case PICO_SHADOW_CAL_DISABLED:
        return "Shadow calibration is currently disabled.";
    case PICO_SHADOW_CAL_ERROR:
        return "Shadow calibration error has occurred.";
    case PICO_SHADOW_CAL_CORRUPT:
        return "The shadow calibration is corrupt.";
    case PICO_DEVICE_MEMORY_OVERFLOW:
        return "the memory onboard the device has overflowed";
    default:
        return "unknown";
    }
}

inline std::string get_error_message(PICO_STATUS status)
{
    return fmt::format("{} - {}", status_to_string(status), status_to_string_verbose(status));
}

} // namespace gr::picoscope::detail
