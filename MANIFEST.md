title: The DIGITIZERS_39 OOT Module
brief: Short description of gr-digitizers_39
tags: # Tags are arbitrary, but look at CGRAN what other authors are using
  - sdr
author:
  - Author Name <authors@email.address>
copyright_owner:
  - Copyright Owner 1
license:
gr_supported_version: # Put a comma separated list of supported GR versions here
#repo: # Put the URL of the repository here, or leave blank for default
#website: <module_website> # If you have a separate project website, put it here
#icon: <icon_url> # Put a URL to a square image here that will be used as an icon on CGRAN
---

TODO:
move digitizer_39:
sudo cp -r /usr/local/lib/python3/dist-packages/digitizers_39/ /usr/lib/python3/dist-packages/
ALT:
-DCMAKE_INSTALL_PREFIX="/usr/"

Setup
========

Note: This setup is only tested and working on the following OS:

    Ubuntu 20.04.3 LTS
    Codename "focal"


Run setup.sh as root to install required dependencies.
It adds a symlink for picoscope lib and installs Visual Studio Code for development, if desired.

    cd build
    cmake ..
    sudo make install
optionally use 

    sudo make install -j4 
or higher numbers to compile with multiple cores.

    sudo ldconfig

Run custom python binds
========

In order to run the bound python code you'll have to export the correct python dist to PythonPath:

    export PYTHONPATH=/usr/local/lib/python3/dist-packages:$PYTHONPATH
    gnuradio-companion

Noto: Otherwise you'll receive a "Module not found" exception.

Process after adding or heavily modifying files
============

Cmake runs binding functions, which sometimes requires a "clean" slate.  

    gr_modtool bind <BLOCKNAME>
    cd build
    make clean
    cmake ..
    sudo make install
    sudo ldconfig



How to manually bind your C++ Code to python/gnuradio
=====================================================

In simple cases 

    gr_modtool bind <BLOCKNAME>
should do all the work for you. But in some cases (probably relating to a few std-lib expressions) the underlying pybind11 lib throws errors and doesn't properly bind. This section explains, what you need to do in this case.

In the following \<BLOCKNAME\> refers to the name of the block you are adding, \<MODULENAME\> is the module which contains the block. 

Create a new module and block using gr_modtool.

    gr_modtool newmod <MODULENAME>
    cd <MODULENAME>
    gr_modtool add <BLOCKNAME>
See https://wiki.gnuradio.org/index.php/OutOfTreeModules for further information.


For manual binding these following files need to be edited/added:

* _python/bindings/\<BLOCKNAME\>\_python.cc_

* _lib/\<BLOCKNAME\>\_impl.h and *.cc_

* _grc/\<MODULENAME\>\_\<BLOCKNAME\>.block.yml_

* _include/\<BLOCKNAME\>\_source.h_



In _grc/\<MODULENAME\>\_\<BLOCKNAME\>.block.yml:_
---------------  

Add required functions as "callbacks". These usually are setter functions for parameters.

    templates:
        imports: import pulsed_power_daq
        make: "<MODULENAME>.<BLOCKNAME>(${paramter_1}, True)"
        callbacks:
        - FUNCTION_NAME(${parameter_1}, ${parameter_2})


In _lib/\<BLOCKNAME\>\_impl.h and *.cc:_
---------------  
  
If you work with callback functions, you need to explicitly add inherited functions to your header file.
For Example if your \<BLOCKNAME\>.h inherits from \<PARENTBLOCK\>, which then defines 

    virtual void foo() = 0;

and declares *foo()* in it's .cc file. 
Gnuradio does not seem to find those functions, if you don't explicitly call those parent functions within the inherited block.
In this example:

    void <BLOCKNAME>::foo(){
      <PARENTBLOCK>::foo();
    }



In _include/\<BLOCKNAME\>.h:_
---------------  

Explicitly add inhereted virtual functions. 
In above case, simply defining *foo()*  in your \<BLOCKNAME\>_impl.h and *.cc does not suffice. You need to add *foo() * to \<BLOCKNAME\>.h as well for gnuradio to find it.

    virtual void foo() = 0;

In _python/bindings/\<BLOCKNAME\>\_python.cc:_
----------------------------------------
  
  

Inheriting implicitly from blocks doesnt work.  
Example:  

    py::class_<<PARENT_BLOCKNAME>,  
               std::shared_ptr<<BLOCKNAME>>>(m, "<BLOCKNAME>", D(<BLOCKNAME>))

You need to explicitly add the blocks you are inheriting from. In this example from sync_block:

    py::class_<block_name,  
                gr::sync_block,  
                gr::block,  
                gr::basic_block,  
                std::shared_ptr<<BLOCKNAME>>>(m, "<BLOCKNAME>", (<BLOCKNAME>))  

Add your functions similar to make as:  

    .def("FUNCTION_NAME", &<BLOCKNAME>::FUNCTION_NAME, py::arg("parameter_1"), py::arg("parameter_2"))

for further information:
https://wiki.gnuradio.org/index.php/GNU_Radio_3.9_OOT_Module_Porting_Guide#CMakeLists.txt_changes_to_fix_OOT_module_testing




How to add c++ unit tests
=====================================================

In *libs* folder create a qa file. Include your file to be tested as *#include "file"* and *#include <boost/test/unit_test.hpp>* .
You can put your test cases in suites for better compartmentalization. Each test is surrounded by *BOOST_AUTO_TEST_CASE(test_name);* and *BOOST_AUTO_TEST_SUITE_END();*

Then add your qa file to *lib/CMakeLists.txt* under *list(APPEND test_project_sources ... )*