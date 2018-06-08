#ifndef STEP_RESPONSE_H_
#define STEP_RESPONSE_H_

// some generic ROOT libraries
#include <TRandom2.h>
#include <TGraph.h>
#include <TGraphErrors.h>


#include "IIRFilter.h" // C++ classes for first, second, cubic and modified second order IIR low-pass filters


/*
 * Simple function to illustrate second-order filtered Heaviside step response
 * 
 */
TGraphErrors *generateStepResponse(Int_t nSamples, Double_t samplingPeriod, Double_t amplitude, Double_t timeOffset, Double_t kickerTimeConstant, Double_t dampingConstant, Double_t noiseAmplitude, Bool_t applyFilter = kTRUE, Bool_t addNoise = kTRUE) {
    TGraphErrors *gr = new TGraphErrors();
    
    // 2nd order filter response as generic kicker signal simulation
    Double_t filterInitialValue = 0.0;     // initial filter value
    SecondOrder *filter = new SecondOrder(filterInitialValue, kickerTimeConstant, dampingConstant, samplingPeriod, kTRUE); 
    TRandom2 *random = new TRandom2(0);
    
    // generate demo radial loop modulation, initial transient is due to slow round-in
    // let the modulation start a bit earlier to cut-off the rounding in for the magnitude sepctra
    for (int i=0; i < nSamples; i++) {   
      Double_t time = i*samplingPeriod;
      Double_t val = amplitude*(((time-timeOffset)>0)?1.0:0.0); // Heaviside step function        
      Double_t valFiltered = filter->update(val);
      //Double_t noise = random->Gaus(0.0, noiseAmplitude); // different between calls
      Double_t noise = 2.0 * 3*noiseAmplitude * (random->Rndm()-0.5); // the same sequence of numbers between different calls
            
      gr->SetPoint(i, time, (applyFilter?valFiltered:val) + (addNoise?noise:0.0));     
      gr->SetPointError(i, 0.0, (addNoise?noiseAmplitude:0.0));
    }  

    delete filter;
    delete random;
    
    return gr;
}


TGraphErrors *rescaleXAxis(TGraphErrors *g,double scale)
{
    TGraphErrors *ret = new TGraphErrors();
    
    for (int i=0; i < g->GetN(); i++) {
        Double_t x,y,dx,dy;
        g->GetPoint(i,x,y);
        dx = g->GetErrorX(i);
        dy = g->GetErrorY(i);
        ret->SetPoint(i, x*scale, y);
        ret->SetPointError(i, dx, dy);
    }    
 
    return ret;
}


#endif /*STEP_RESPONSE_H_*/
