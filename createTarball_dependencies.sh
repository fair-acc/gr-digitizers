#!/bin/sh

FOLDER_TO_TAR=usr
INSTALL_DIR_LIB=${FOLDER_TO_TAR}/lib
INSTALL_DIR_LIB64=${FOLDER_TO_TAR}/lib64
INSTALL_DIR_ETC=${FOLDER_TO_TAR}/etc
INSTALL_DIR_INCLUDE=${FOLDER_TO_TAR}/include
INSTALL_DIR_BIN=${FOLDER_TO_TAR}/bin
SYSTEM_LIB_DIR=/usr/lib64
PICOSCOPE_LIB_DIR=/opt/picoscope/lib
ROOT_INCLUDE_DIR=/opt/cern/root/include
ROOT_LIB_DIR=/opt/cern/root/lib
ROOT_ETC_DIR=/opt/cern/root/etc
GNURADIO_VERSION=3.7.10.1
BOOST_VERSION=1.53.0

TARBALL_NAME=DigitizerDependencies.tar

mkdir -p ${INSTALL_DIR_LIB64}
mkdir -p ${INSTALL_DIR_LIB}
mkdir -p ${INSTALL_DIR_ETC}
mkdir -p ${INSTALL_DIR_INCLUDE}

# gnuradio standard
cp ${SYSTEM_LIB_DIR}/libgnuradio-analog-${GNURADIO_VERSION}.so.0.0.0 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libgnuradio-blocks-${GNURADIO_VERSION}.so.0.0.0 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libgnuradio-fft-${GNURADIO_VERSION}.so.0.0.0 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libgnuradio-filter-${GNURADIO_VERSION}.so.0.0.0 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libgnuradio-pmt-${GNURADIO_VERSION}.so.0.0.0 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libgnuradio-runtime-${GNURADIO_VERSION}.so.0.0.0 ${INSTALL_DIR_LIB64}

# picotec drivers
cp ${SYSTEM_LIB_DIR}/libps3000a.so.2 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libps4000a.so.2 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libps6000.so.2 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libps6000.so.2 ${INSTALL_DIR_LIB64}
cp ${PICOSCOPE_LIB_DIR}/libpicoipp.so ${INSTALL_DIR_LIB64}
cp ${PICOSCOPE_LIB_DIR}/libiomp5.so ${INSTALL_DIR_LIB64}

# boost
cp ${SYSTEM_LIB_DIR}/libboost_chrono-mt.so.${BOOST_VERSION} ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libboost_date_time-mt.so.${BOOST_VERSION} ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libboost_filesystem-mt.so.${BOOST_VERSION} ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libboost_program_options-mt.so.${BOOST_VERSION} ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libboost_regex-mt.so.${BOOST_VERSION} ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libboost_system-mt.so.${BOOST_VERSION} ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libboost_thread-mt.so.${BOOST_VERSION} ${INSTALL_DIR_LIB64}

# system
cp ${SYSTEM_LIB_DIR}/libudev.so.1 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libicuuc.so.50 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libicui18n.so.50 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libicudata.so.50 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libfreetype.so.6 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libusb-1.0.so.0 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libcap.so.2 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libdw.so.1 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libattr.so.1 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libelf.so.1 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libbz2.so.1 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libvolk.so.1.3 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libfftw3f.so.3 ${INSTALL_DIR_LIB64}
cp ${SYSTEM_LIB_DIR}/libfftw3f_threads.so.3 ${INSTALL_DIR_LIB64}

# system - needed by "test-digitizers"
cp ${SYSTEM_LIB_DIR}/libcppunit-1.12.so.1 ${INSTALL_DIR_LIB64}


# ROOT
cp ${ROOT_LIB_DIR}/libCore.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libRIO.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libHist.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libGraf.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libGraf3d.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libGpad.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libTree.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libRint.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libPostscript.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libMatrix.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libPhysics.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libMathCore.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libThread.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libMultiProc.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libTreePlayer.so ${INSTALL_DIR_LIB64}
cp ${ROOT_LIB_DIR}/libNet.so ${INSTALL_DIR_LIB64}

# ROOT - needed by "test-digitizers"
cp ${ROOT_LIB_DIR}/libCling.so ${INSTALL_DIR_LIB}
cp ${ROOT_LIB_DIR}/libNet_rdict.pcm ${INSTALL_DIR_LIB}
cp ${ROOT_LIB_DIR}/libMathCore_rdict.pcm ${INSTALL_DIR_LIB}
cp ${ROOT_LIB_DIR}/libMatrix_rdict.pcm ${INSTALL_DIR_LIB}
cp ${ROOT_LIB_DIR}/libHist_rdict.pcm ${INSTALL_DIR_LIB}
cp ${ROOT_LIB_DIR}/libGraf_rdict.pcm ${INSTALL_DIR_LIB}
cp ${ROOT_LIB_DIR}/libGpad_rdict.pcm ${INSTALL_DIR_LIB}
cp ${ROOT_LIB_DIR}/libGraf3d_rdict.pcm ${INSTALL_DIR_LIB}
cp ${ROOT_LIB_DIR}/libTree_rdict.pcm ${INSTALL_DIR_LIB}
cp ${ROOT_LIB_DIR}/libTreePlayer_rdict.pcm ${INSTALL_DIR_LIB}
cp ${ROOT_LIB_DIR}/libMultiProc_rdict.pcm ${INSTALL_DIR_LIB}
cp ${ROOT_LIB_DIR}/libPhysics_rdict.pcm ${INSTALL_DIR_LIB}
cp ${ROOT_LIB_DIR}/libPostscript_rdict.pcm ${INSTALL_DIR_LIB}
cp ${ROOT_ETC_DIR}/allDict.cxx.pch ${INSTALL_DIR_ETC}
cp -r ${ROOT_ETC_DIR}/cling ${INSTALL_DIR_ETC}
cp -r ${ROOT_INCLUDE_DIR}/* ${INSTALL_DIR_INCLUDE}

# Volk and FFTW vector optimization tools
cp /usr/bin/volk_profile ${INSTALL_DIR_BIN}
cp /usr/bin/fftw-wisdom ${INSTALL_DIR_BIN}

tar cfv ${TARBALL_NAME} ${FOLDER_TO_TAR}
rm -rf ${FOLDER_TO_TAR}
gzip ${TARBALL_NAME}

cp ${TARBALL_NAME}.gz /common/export/fesa/arch/x86_64