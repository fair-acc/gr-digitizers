
import os

try:
    from .picoscope3000a_python import *
except ImportError:
    dirname, filename = os.path.split(os.path.abspath(__file__))
    __path__.append(os.path.join(dirname, "bindings"))
    from .picoscope3000a_python import *
