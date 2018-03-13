# Manual testing procedure (PicoScope 3000a)


# Preconditions


# Test cases

## Common

### Downsampling modes

TODO: add automated test

### Error propagation

When downsampling is none error should be 3% of the range or similar


## AI Channels

### Range

The following flowgraph can be used: TODO

For each AI channel and input range combination (200mV, ..., 20V) follow the following steps
 - Connect selected channel to the signal generator outputing some signal (e.g. sine) with
   the desired voltage amplitued
 - Verify if signal is properly displayed on the display

### Coupling (AC/DC)

 TODO

### Range offset

Test with some valid ranges

±250 mV (20 mV, 50 mV, 100 mV, 200 mV ranges)
±2.5 V (500 mV, 1 V, 2 V ranges)
±20 V (5 V, 10 V, 20 V ranges)

## DI Channels

### Logic level

Configure, connect input generator, test


## Triggers (rapid block)

For this test the following example GRC file can be used: TODO.

For each channel (and a few rendomly selected DI inputs)
 - Connect signal generator to the selected input channel
 - Configure trigger
 - Run acquisition
 - Generate trigger (try raisign, falling, low, high)

