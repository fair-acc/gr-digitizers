
# gr-digitizers

`gr-digitizers` is a collection of gnuradio blocks for signal processing.

- It supports the usage of some [picotech](https://www.picotech.com/) soft oscilloscope devices as signal source.
- In order to allign the data in time, gnuradio tags are used which are send together with the data.
- Each data signal as well has a corresponding [rms](https://en.wikipedia.org/wiki/Root_mean_square)-error signal which is passed down the flowgraph together with the data.

## Building and install

In a nutshell we have three main dependencies:
 - [GNU Radio](https://www.gnuradio.org/) (boost, fftw, and some other libraries are required via dependency tree)
 - [ROOT](https://root.cern/)
 - [PicoScope Drivers](https://www.picotech.com/downloads/linux).

GNU Radio and ROOT could be compiled/linked statically and required libraries could be in principle copied
and deployed together with the test deploy unit. The only problem is PicoScope library (well libraries)
which needs to be installed into the /opt/picoscope directory.

Download and install PicoScope drivers from here: [https://www.picotech.com/downloads/linux](https://www.picotech.com/downloads/linux).

Afterwards, you can build `gr-digitizers` like this:

```shell
$ mkdir gr-digitizers/build
$ cd gr-digitizers/build
$ cmake .. -DENABLE_GR_LOG=1 \
           -DENABLE_STATIC_LIBS=ON
$ make
$ ./lib/test_digitizers_test.sh # runs unit tests
$ make install
```
- For debug output add the following to cmake: `-DDEBUG_ENABLED=1`
- Configure the install location with `-DCMAKE_INSTALL_PREFIX=/path/to/gr-digitizer/install/location` 

By default only unit tests wihtout any HW dependencies are executed. In order to execute PicoScope 3000a
related tests execute the below command (from you build directory):

```shell
$ lib/test-digitizers --enable-ps3000a-tests
```

Note, HW related tests cannot be run in parallel.

## Usage

If you installed into a folder different from `/opt/gnuradio`, you will need to add the location of the new blocks into your `~/.gnuradio/config.conf`

```shell
$ cat ~/.gnuradio/config.conf 
[grc]
local_blocks_path=/path/to/gr-digitizer/install/location/share/gnuradio/grc/blocks
```

Check [this directory](examples) for a wide range of examples.
