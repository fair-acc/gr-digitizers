# White-Rabbit based timing blocks and test utilities

This directory contains the timing realated gnuradio blocks along with some testing utilites and setup instructions.

# Dependencies

For actually using the blocks and utilities built by this you will also need actual timing receiver hardware, the
[wishbone kernel module](https://ohwr.org/project/fpga-config-space/tree/realtime_fixes/pcie-wb) has to be loaded and the saftlibd daemon has to be running and configured properly.

`etherbone` and `saftlib` require a few additional packages. On Ubuntu you can install them with:

```shell
sudo apt-get update && sudo apt-get -y install build-essential autoconf automake libtool libsigc++-2.0-dev libxslt1-dev libboost-all-dev
```

```shell
git clone --branch v2.1.4 --depth=1 https://gitlab.com/ohwr/project/etherbone-core.git
cd etherbone-core/api
./autogen.sh
./configure
make -j
sudo make install
```

```shell
git clone --branch v3.2.2 --depth=1 https://github.com/GSI-CS-CO/saftlib.git
cd saftlib
./autogen.sh
./configure
make -j
sudo make install
```

# Setting up usage with an actual timing card

- build `etherbone` and `saftlib` as described above
- build `gr-digitizers` with `-DENABLE_TIMING=True`
- load the required kernel modules if the card is connected via PCIe: `modprobe pcie_wb`
  - verify they are loaded correctly: `lsmod | grep wb`, `dmesg | tail`
- start saftd demon and attach timing card: `saftd tr0:dev/wbm0` (or `saftd tr0:dev/ttyUSB0`), where `dev/wbm0` (or `dev/ttyUSB0`) is the device path of the wishbone device without leading slash)
- if there is no physical timing signal plugged into the card, the card has to be switched to master mode:
  - `eb-console dev/wbm0` (or `eb-console dev/ttyUSB0`)
  - type `mode master` and hit return. The console should print a message that the timing mode is changed.
  - if you connect timing card via USB you may have to close `saftd` first, change mode and run `saftd` again.
  - `eb-console` is included in [bel_projects](https://github.com/GSI-CS-CO/bel_projects)/`tools`.
    If you only need the single tool and have installed `etherbone` on your system by other means you can build only `eb-console`:
    ```bash
    $ git clone https://github.com/GSI-CS-CO/bel_projects
    $ cd bel_projects/tools
    $ make EB=/usr/include eb-console # or point the EB variable to whatever prefix you installed etherbone to
    $ ./eb-console dev/wbm0 # or dev/ttyUSB0
    ```
  - enable output and check if it's enabled (in the output search for `OutputEnable: On`):
    ```bash
    $ saft-io-ctl tr0 -n IO3 -o 1
    $ saft-io-ctl tr0 -n IO3
    ```
- verify that everything works with `saft-ctl`:
  - ```bash
    $ saft-ctl tr0 -ijkst
    saftlib source version                  : saftlib 3.0.3 (6aab401-dirty): Aug 29 2023 09:50:19
    saftlib build info                      : built by unknown on Jan  1 1980 00:00:00 with localhost running
    devices attached on this host   : 1
    device: /de/gsi/saftlib/tr0, name: tr0, path: dev/wbm0, gatewareVersion : 6.1.2
    --gateware version info:
    ---- Mon Aug 09 08:48:31 CEST 2021
    ---- fallout-v6.1.2
    ---- Arria V (5agxma3d4f27i3)
    ---- CentOS Linux release 7.9.2009 (Core), kernel 3.10.0-1160.36.2.el7.x86_64
    ---- pexaria5 +db[12] +wrex1
    ---- Timing Group Jenkins <csco-tg@gsi.de>
    ---- tsl021.acc.gsi.de
    ---- pci_control
    ---- Version 18.1.0 Build 625 09/12/2018 SJ Standard Edition
    ---- fallout-3847
    6.1.2
    current temperature (Celsius): 48
    WR locked, time: 0x001181787bda0268
    receiver free conditions: 255, max (capacity of HW): 0(256), early threshold: 4294967296 ns, latency: 4096 ns
    sinks instantiated on this host: 1
    /de/gsi/saftlib/tr0/software/_77 (minOffset: -1000000000 ns, maxOffset: 1000000000 ns)
    -- actions: 0, delayed: 0, conflict: 0, late: 0, early: 0, overflow: 0 (max signalRate: 10Hz)
    -- conditions: 0
    ```
  - snoop on all events: `saft-ctl tr0 snoop 0x0 0x0 0x0` and on another terminal inject an event: `saft-ctl tr0 inject 0x1154000140000000 0x15 1000000` to verify it arrives.
  - finally launch `build/blocklib/timing/test-timing` to show the UI of the debug utility
