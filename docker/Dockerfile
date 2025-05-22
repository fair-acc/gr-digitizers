FROM ghcr.io/fair-acc/gr4-build-container:latest
LABEL authors="akrimm+rstein"

# Install pkg-config
RUN sudo apt-get update && sudo apt-get -y install build-essential pkg-config && sudo apt-get clean && sudo rm -rf /var/lib/apt/lists/*

# Install timing system dependencies: etherbone"
RUN sudo apt-get update \
    && sudo apt-get -y install build-essential autoconf automake libtool \
    && cd $(mktmp -d) \
    && git clone --branch v2.1.4 --depth=1 https://gitlab.com/ohwr/project/etherbone-core.git \
    && cd etherbone-core/api \
    && touch ChangeLog \
    && sed -e "s%AC_MSG_ERROR%AC_MSG_NOTICE%g" -i configure.ac \
    && autoreconf -i \
    && ./configure \
    && make -j \
    && sudo make install \
    && rm -rf ./* \
    && sudo apt-get clean \
    && sudo rm -rf /var/lib/apt/lists/*

# Install timing system dependencies: saftlib"
RUN sudo apt-get update \
    && sudo apt-get -y install libsigc++-2.0-dev libxslt1-dev libboost-all-dev \
    && cd $(mktmp -d) \
    && git clone --branch v3.1.4 --depth=1 https://github.com/GSI-CS-CO/saftlib.git \
    && cd saftlib \
    && ./autogen.sh \
    && ./configure \
    && make \
    && sudo make install \
    && rm -rf ./* \
    && sudo apt-get clean \
    && sudo rm -rf /var/lib/apt/lists/*

# Install picoscope libraries https://www.picotech.com/downloads/linux
RUN sudo apt-get update \
    && ( wget -O - https://labs.picotech.com/Release.gpg.key | sudo gpg --dearmor -o /usr/share/keyrings/picotech.gpg ) \
    && (echo 'deb [signed-by=/usr/share/keyrings/picotech.gpg] https://labs.picotech.com/rc/picoscope7/debian/ picoscope main' | sudo tee /etc/apt/sources.list.d/picoscope.list )  \
    && sudo apt update \
    && ( sudo apt install -y udev libusb-1.0-0-dev libps3000a libps4000a libps5000a libps6000 libps6000a libx11-dev libgl1-mesa-dev libsdl2-dev || true ) \
    && sudo apt-get clean \
    && sudo rm -rf /var/lib/apt/lists/*

USER user
ENTRYPOINT ["/bin/bash", "-c"]
