// @(#)src/:$Name:  $:$Id: PeakFitter,v 1.0 2017/07/24 14:41:33 rstein Exp $
// Author: Ralph J. Steinhagen 2017-05-24 R.Steinhagen@GSI.de
#ifndef PEAK_FITTER_H_
#define PEAK_FITTER_H_

class SpectrumPeakFitter : TObject {
private:
    Int_t       fnBins;
    Double_t    *frequency;

public:
	SpectrumPeakFitter(Int_t nBins) {
        const Char_t *functionName = "SpectrumPeakFitter(Int_t nBins)";
        
        if (nBins <=0) {
            Error(functionName, "nBins %i is <= 0", nBins);
        }
        
        fnBins = nBins;
        frequency = new Double_t[fnBins];
        
        for (int i=0; i < fnBins; i++) {
                frequency[i] = 0.5*(Double_t)i/(Double_t)fnBins;
        }
    }
    
    SpectrumPeakFitter(Double_t *frequencyScale, Int_t nBins) { 
        const Char_t *functionName = "SpectrumPeakFitter(Double_t *frequencyScale, Int_t nBins)";
        if (nBins <=0) {
            Error(functionName, "nBins %i is <= 0", nBins);
        }
        
        if (frequencyScale == NULL) {
            Error(functionName, "NULL frequencyScale");
        }
        
        fnBins = nBins;
        frequency = new Double_t[fnBins];
        memcpy(frequency, frequencyScale, fnBins*sizeof(Double_t));        
    }
    
    ~SpectrumPeakFitter() {
        if (frequency!= NULL) delete frequency;
    }
	
	Int_t roughPeakPositionEstimate(Double_t *magnitudeSpectrum, Double_t fmin = 0.0, Double_t fmax = 0.5) {
        Int_t imax = -1;
        Double_t max = -DBL_MAX;
        
        for (int i=0; i < fnBins; i++) {
            if ((frequency[i] >= fmin) && (frequency[i] <= fmax)) {
                Double_t val = magnitudeSpectrum[i];
                
                if (val > max) {
                    imax =i;
                    max = val;
                }
            }
        }
        
        return imax;
    }
	
	Double_t gaussPeakPositionEstimate(Double_t *magnitudeSpectrum, Int_t coarseIndex) {        
        // some boundary checks                
        if (coarseIndex <= 0) {
            return 0;
        }
        
        if (coarseIndex > fnBins-1) {
            return fnBins-1;
        }
        
        Double_t left   = magnitudeSpectrum[coarseIndex-1];
        Double_t center = magnitudeSpectrum[coarseIndex+0];
        Double_t right  = magnitudeSpectrum[coarseIndex+1];
            
        return coarseIndex + 0.5*log(right/left)/log(pow(center,2)/(left*right));
    }
    
    Double_t gaussPeakPositionEstimate(Double_t *magnitudeSpectrum, Double_t fmin = 0.0, Double_t fmax = 0.5) {        
        Int_t coarseIndex = roughPeakPositionEstimate(magnitudeSpectrum, fmin, fmax);
        
        return gaussPeakPositionEstimate(magnitudeSpectrum, coarseIndex);
    }
};


#endif
