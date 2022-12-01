
# gr-digitizers

`gr-digitizers` is a collection of gnuradio blocks for signal processing.

- It supports the usage of some [picotech](https://www.picotech.com/) soft oscilloscope devices as signal source.
- In order to allign the data in time, gnuradio tags are used which are sent together with the data.
- Each data signal as well has a corresponding [rms](https://en.wikipedia.org/wiki/Root_mean_square)-error signal which is passed down the flowgraph together with the data.

## Building and install

In a nutshell, we have three main dependencies:

 - [GNU Radio](https://www.gnuradio.org/) (dev-4.0 branch)
 - [ROOT](https://root.cern/)
 - [PicoScope Drivers](https://www.picotech.com/downloads/linux).

In the following it is assumed that ROOT is installed to `/opt/root`, and the picoscope drivers
to `/opt/picoscope` (which is e.g. done when using the Ubuntu packages).

To build gr-digitizers, run:

```shell
$ meson setup builddir . -Dlibpicoscope_prefix=/opt/picoscope -Dlibroot_prefix=/opt/root
$ ninja
$ LD_LIBRARY_PATH=/opt/root/lib ninja test # run unit tests, optional
$ ninja install
```

By default, only unit tests without any HW dependencies are executed. In order to execute e.g. the PicoScope 3000a
related tests, execute the below command:

```shell
$ LD_LIBRARY_PATH=/opt/root/lib PICOSCOPE_RUN_TESTS=3000a ninja test
```

Running multiple HW tests together, like `PICOSCOPE_RUN_TESTS=3000a,4000a`, is technically possible
but not recommended, as meson runs the tests in parallel, which will cause issues with the drivers.

## Legal

Copyright (C) 2018-2022 FAIR -- Facility for Antiproton & Ion Research, Darmstadt, Germany

Unless otherwise noted this library follows:
SPDX-License-Identifier: LGPL-3.0-or-later

## Contributors

This library has been co-developed under contract with:

CERN, Geneva, Switzerland
KDAB, Berlin, Germany
Cosylab, Ljubljana, Slovenia
