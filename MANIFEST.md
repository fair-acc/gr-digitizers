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
