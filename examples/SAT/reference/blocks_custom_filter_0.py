"""
Embedded Python Blocks:

Each time this file is saved, GRC will instantiate the first class it finds
to get ports and parameters of your block. The arguments to __init__  will
be the parameters. All of them are required to have default values!
"""

import numpy as np
from gnuradio import gr
from gnuradio import blocks
from gnuradio import filter
from gnuradio.filter import firdes

class blk(gr.decim_block):  # other base classes are sync_block, basic_block, decim_block, interp_block
    """Embedded Python Block example - simple if/else for various filter implementation"""

    def __init__(self, decimation             = 1, 
                       sampling_Frequency     = 1000.0, 
                       lower_CutOff_Frequency = 0.0, 
                       upper_CutOff_Frequency = 10.0, 
                       transition_Frequency   = 1.0, 
                       forward_Taps = [], feedback_Taps = ()):  # only default arguments here
        """arguments to this function show up as parameters in GRC"""
        #gr.sync_block.__init__( self, name='Custom Filter Block',   # will show up in GRC
        #    in_sig=[np.float32],
        #    out_sig=[np.float32]
        #)
        gr.decim_block.__init__(self, 
            name    = "Custom Filter Block",
            in_sig  = [np.float32],
            out_sig = [np.float32],
            decim   = decimation
        )
        #gr.hier_block2.__init__(
        #    self, "Custom Filter Block",
        #    gr.io_signature(1, 1, gr.sizeof_float*1),
        #    gr.io_signature(1, 2, gr.sizeof_float*1)
        #)

        # if an attribute with the same name as a parameter is found,
        # a callback is registered (properties work, too).
        self.decimation = decimation
        
        ##################################################
        # Parameters
        ##################################################
        self.decimation = decimation
        self.sampling_Frequency    = sampling_Frequency
        self.lower_CutOff_Frequency = lower_CutOff_Frequency
        self.upper_CutOff_Frequency = upper_CutOff_Frequency
        self.transition_Frequency  = transition_Frequency
        self.forward_Taps          = forward_Taps
        self.feedback_Taps         = feedback_Taps
        #self.FIR_Taps              = None;
        #self.filter                = None;
        
        ##################################################
        # init filter
        ##################################################
        self.initFilter()
        self.testFilter()

    def testFilter(self):
        print "test filter routine,", "#forward taps =", len(self.forward_Taps), "#feedback taps =", len(self.feedback_Taps)
        return
        
    def initFilter(self):
        
        #self.lock()
        #self.disconnect(self.filter)
        
        
        # check of 'feedback_Taps' is not empty then, iirMode = true;
        if len(self.feedback_Taps)<=0:
            print "feedback taps are empty -> use FIR implementation"
            if len(self.forward_Taps)<=0:
                print "-> use parameterized FIR implementation" 
                if (self.lower_CutOff_Frequency <= 0):                    
                    self.FIR_Taps = firdes.low_pass(self.decimation, self.sampling_Frequency, self.upper_CutOff_Frequency, self.transition_Frequency, firdes.WIN_HAMMING, 6.76)
                    #print "compute low-pass with #taps = ", len(self.FIR_Taps), " ", self.FIR_Taps
                else:
                    self.FIR_Taps = firdes.band_pass(self.decimation, self.sampling_Frequency, self.lower_CutOff_Frequency, self.upper_CutOff_Frequency, self.transition_Frequency, firdes.WIN_HAMMING, 6.76)                        
                    #print "compute band-pass with #taps = ", len(self.FIR_Taps), " ", self.FIR_Taps
            else: 
                print "-> use custom FIR coeficients"
                self.FIR_Taps = self.forward_Taps
            
            print "filter taps (init) = ", len(self.FIR_Taps)        
            self.filter = filter.fir_filter_fff(self.decimation, self.FIR_Taps)            
        else: 
            if (len(self.forward_Taps) > 0) and (len(self.feedback_Taps) > 0):
                print "-> use IIR implementation"                
            else:
                print "something went wrong w.r.t. FIR<->IIR selection -> choose safe fall back (bypass)"
                self.forward_Taps = (1)
                self.feedback_Taps = (0)
            self.filter = filter.iir_filter_ffd(self.forward_Taps, self.feedback_Taps, False)
    
        print "before making connections"
        ##################################################
        # Connections
        ##################################################
        #self.connect((self.filter, 0), (self, 0))
        #self.connect((self, 0), (self.filter, 0))
        #self.unlock()
        print "after making connections"
        return

    def work(self, input_items, output_items):
        """example: multiply with constant"""
        #output_items[0][:] = input_items[0]
        #output_items[0][:] = input_items[0] * self.decimation
        #output_items = self.filter(input_items)
        #output_items[0][:] = self.filter(input_items[0])
        output_items[0][:] = self.filter.filter(input_items[0])
        return len(output_items[0])
        #return nout
