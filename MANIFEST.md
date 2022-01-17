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
A longer, multi-line description of gr-digitizers_39.
You may use some *basic* Markdown here.
If left empty, it will try to find a README file instead.

TODO:
move digitizer_39:
sudo cp -r /usr/local/lib/python3/dist-packages/digitizers_39/ /usr/lib/python3/dist-packages/
ALT:
-DCMAKE_INSTALL_PREFIX="/usr/"

TODO:
find alt for ps4000a (NO symlink)

TDOD:
FIX BOOST !!!

https://github.com/fair-acc/gr-digitizers/issues/47

Generating: '/home/neumann/boiler_plate.py'

Executing: /usr/bin/python3 -u /home/neumann/boiler_plate.py

Traceback (most recent call last):
  File "/home/neumann/boiler_plate.py", line 35, in <module>
    import digitizers_39
  File "/usr/lib/python3/dist-packages/digitizers_39/__init__.py", line 18, in <module>
    from .digitizers_39_python import *
ImportError: /usr/local/lib/x86_64-linux-gnu/libgnuradio-digitizers_39.so.1.0.0: undefined symbol: _ZN5boost6chrono12steady_clock3nowEv



How to manually bind your C++ Code to python/gnuradio
=====================================================

create a new module and block using gr_modtool.
Inthe following \<BLOCKNAME\> refers to the name of the block you are adding. 

The following files need to be edited/added:

* _python/bindings/\<BLOCKNAME\>\_python.cc_

* _lib/\<BLOCKNAME\>\_impl.h and *.cc_

* _grc/\<MODULENAME\>\_\<BLOCKNAME\>.block.yml_

* _include/\<BLOCKNAME\>\_source.h_



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

for further information further:
https://wiki.gnuradio.org/index.php/GNU_Radio_3.9_OOT_Module_Porting_Guide#CMakeLists.txt_changes_to_fix_OOT_module_testing


In _lib/\<BLOCKNAME\>\_impl.h and *.cc:_
---------------  
  
Explicitly implement inhereted functions required. Call parents' equivalents within.
**TODO:** how to better describe this

In _grc/\<MODULENAME\>\_\<BLOCKNAME\>.block.yml:_
---------------  

Add required functions as "callbacks":

    templates:
        imports: import pulsed_power_daq
        make: "<MODULENAME>.<BLOCKNAME>(${paramter_1}, True)"
        callbacks:
        - FUNCTION_NAME(${parameter_1}, ${parameter_2})


In _include/\<BLOCKNAME\>\_source.h:_
---------------  

Explicitly add inhereted virtual functions.  
**TODO:** how to better describe this
