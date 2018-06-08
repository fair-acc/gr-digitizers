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

class blk(gr.sync_block):  # sync_block, other base classes are basic_block, decim_block, interp_block
    """Embedded Python Block example - a simple multiply const"""

    def __init__(self, decimation = 1.0, 
                       sampling_Frequency    = 1000.0, 
                       lower_CutOff_Frequency = 0.0, 
                       upper_CutOff_Frequency = 10.0, 
                       transition_Frequency  = 1.0, 
                       forward_Taps = [], feedback_Taps = []):  # only default arguments here
        """arguments to this function show up as parameters in GRC"""
        gr.sync_block.__init__(
            self,
            name='Custom Filter Block',   # will show up in GRC
            in_sig=[np.float32],
            out_sig=[np.float32]
        )
        # if an attribute with the same name as a parameter is found,
        # a callback is registered (properties work, too).
        
        
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
        
        # check of 'feedback_Taps' is not empty then, iirMode = true;
        if len(self.feedback_Taps)<=0:
            print "feedback taps are empty -> use FIR implementation"
            if len(self.forward_Taps)<=0:
                print "feed-forward taps are empty -> use parameterized FIR implementation"
                if (self.lower_CutOff_Frequency <= 0):
                    self.FIR_Taps = firdes.low_pass(decimation, self.sampling_Frequency, self.upper_CutOff_Frequency, self.transition_Frequency, firdes.WIN_HAMMING, 6.76)
                else:
                    self.FIR_Taps = firdes.band_pass(decimation, self.sampling_Frequency, self.lower_CutOff_Frequency, self.upper_CutOff_Frequency, self.transition_Frequency, firdes.WIN_HAMMING, 6.76)                        
            else: 
                print "feed-forward taps are not empty -> use custom FIR coeficients"
                self.FIR_Taps = self.forward_Taps
            
            print "filter taps (init) = ", len(self.FIR_Taps)
            self.filter = filter.fir_filter_fff(decimation, self.FIR_Taps)
        else: 
            if (len(self.forward_Taps)>0) and (len(self.feedback_Taps)>0):
                print "use IIR implementation"
                self.filter = filter.iir_filter_ffd(self.forward_Taps, self.feedback_Taps, False)
            else:
                print "something went wrong w.r.t. FIR<->IIR selection"    

                

    def work(self, input_items, output_items):
        """example: multiply with constant"""
        output_items[0][:] = input_items[0] * self.decimation
        return len(output_items[0])

    def get_upper_CutOff_Frequency(self):
        return self.upper_CutOff_Frequency

    def set_upper_CutOff_Frequency(self, upper_CutOff_Frequency):
        print "inside the setter\n"
        self.upper_CutOff_Frequency = upper_CutOff_Frequency
         # check of 'feedback_Taps' is not empty then, iirMode = true;
        if (self.lower_CutOff_Frequency <= 0):            
            self.FIR_Taps = firdes.low_pass(1, self.sampling_Frequency, self.upper_CutOff_Frequency, self.transition_Frequency, firdes.WIN_HAMMING, 6.76)
        else:
            self.FIR_Taps = firdes.band_pass(1, self.sampling_Frequency, self.lower_CutOff_Frequency, self.upper_CutOff_Frequency, self.transition_Frequency, firdes.WIN_HAMMING, 6.76)

        print "set_upper_CutOff_Frequency:: filter taps = ", len(self.FIR_Taps)
        self.filter.set_taps(self.FIR_Taps)    

