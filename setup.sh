#!/usr/bin/bash
#!/bin/bash
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

#Install python3.6
 apt update -y
 add-apt-repository ppa:deadsnakes/ppa -y
 apt update -y
 apt install python3.6 -y



#If the virtualenv package is not installed, run:
apt update -y
apt install python3-virtualenv -y

#Install virtualenvwrapper
 apt install python3-pip -y
pip3 install virtualenvwrapper

#add top PATH
echo "export VIRTUALENVWRAPPER_PYTHON='/usr/bin/python3'" >> ~/.bashrc
echo "export WORKON_HOME=$HOME/.virtualenvs" >> ~/.bashrc
echo "export PROJECT_HOME=$HOME/Devel" >> ~/.bashrc
echo "source /home/gsi/.local/bin/virtualenvwrapper.sh" >> ~/.bashrc


#Create (and activate) a virtual environment:
mkvirtualenv gnuradio_python --python=/usr/bin/python3.6
#(to deactivate write "deactivate" within the active terminal)

#install gnuradio 3.9
 add-apt-repository ppa:gnuradio/gnuradio-releases -y
 apt update -y
 apt install gnuradio -y
#https://wiki.gnuradio.org/index.php/UbuntuInstall#Focal_Fossa_.2820.04.29_through_Hirsute_Hippo_.2821.04.29
 apt install git cmake g++ libboost-all-dev libgmp-dev swig python3-numpy \
python3-mako python3-sphinx python3-lxml doxygen libfftw3-dev \
libsdl1.2-dev libgsl-dev libqwt-qt5-dev libqt5opengl5-dev python3-pyqt5 \
liblog4cpp5-dev libzmq3-dev python3-yaml python3-click python3-click-plugins \
python3-zmq python3-scipy python3-gi python3-gi-cairo gir1.2-gtk-3.0 \
libcodec2-dev libgsm1-dev -y

 apt install pybind11-dev python3-matplotlib libsndfile1-dev \
python3-pip libsoapysdr-dev soapysdr-tools -y
pip install pygccxml
pip install pyqtgraph

# apt install git
#git clone --branch digitizer39_power https://github.com/fair-acc/gr-digitizers.git

#install VSCode

#check for VSCode Installation
dpkg -s code > /dev/null
if [ $? -ne 0 ] 
then
    echo "Want to install VSCode? Y/n"
    read n 
    if [[ ( $n -eq 'Y' || $n -eq 'y' || $n -eq '' ) ]]
    then
        wget -q https://packages.microsoft.com/keys/microsoft.asc -O- |  apt-key add -
         add-apt-repository "deb [arch=amd64] https://packages.microsoft.com/repos/vscode stable main"
         apt update -y
         apt install code -y
    fi
fi

#install dependencies
 apt install libsndfile-dev -y
 apt install libuhd-dev -y
 apt install alsa-utils -y
 apt install doxygen -y
 apt install libzmq3-dev -y
 apt install qt5-default  -y
 apt install -y python3-pygccxml -y
 apt install libasound2-dev -y
 apt install libjack-jackd2-dev -y  # might need -y as argument
 apt install portaudio19-dev -y
 apt install libqwt-qt5-dev -y
 apt install libqt5svg5-dev -y
 apt install libsoapysdr-dev -y
 apt install -y soapysdr-tools -y #might be optional, above step could already provide the necessary library
 apt install libcodec2-dev -y
 bash -c 'echo "deb https://labs.picotech.com/debian/ picoscope main" >/etc/apt/sources.list.d/picoscope.list'
wget -O - https://labs.picotech.com/debian/dists/picoscope/Release.gpg.key |  apt-key add -
 apt update -y
 apt install picoscope -y
 apt install cmake -y
 apt install xterm -y


#create symlink from 
cd /usr/lib
 ln -s /opt/picoscope/lib/libps4000a.so .
