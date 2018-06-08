#include <stdio.h>
#include <stdlib.h>

#include "StepResponse.h"

// graphics related classes
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TH1F.h>
#include "TH1D.h"

#include <TMultiGraph.h>
#include <TCanvas.h>
#include <TColor.h>
#include <TLegend.h>


void plotStepResponse()
{
  
  Double_t samplingPeriod = 1.0/(500e6); // [s] digitizer sampling period (e.g. 1/(500 MS/s))    
  Int_t    nSamples = 2000;              // number of samples acquired by digitizer
  
  Double_t amplitude = 1.1;       // [a.u.] step response amplitude (e.g. RF kicker amplitude in [kV])
  Double_t noiseAmplitude = 0.02; // [a.u.] measurement noise
  Double_t timeOffset = 500e-9;   // [s] step response offset (e.g. RF kicker delay [s])    
  Double_t kickerTimeConstant = 200e-9;     // [s] second-order time-constant (N.B. no 'two pi') 1/\omega_0
  Double_t dampingConstant = 0.05;          // [] second-order damping coefficient \zeta
  
  
  
  // declare some graphs for plotting
  TGraphErrors *gr_raw      = generateStepResponse(nSamples, samplingPeriod, amplitude, timeOffset, kickerTimeConstant, dampingConstant, noiseAmplitude, kFALSE, kFALSE);
  TGraphErrors *gr_filtered = generateStepResponse(nSamples, samplingPeriod, amplitude, timeOffset, kickerTimeConstant, dampingConstant, noiseAmplitude, kTRUE, kFALSE);
  TGraphErrors *gr_noise    = generateStepResponse(nSamples, samplingPeriod, amplitude, timeOffset, kickerTimeConstant, dampingConstant, noiseAmplitude, kTRUE, kTRUE);
 
   
  
  
  
  
  // declare some graphs for plotting and formating
  Double_t plotXAxisUnitScaling = 1e6; // [us] time base for plotting
  TGraphErrors *gr_raw_plot = rescaleXAxis(gr_raw, plotXAxisUnitScaling);
  TGraphErrors *gr_filtered_plot = rescaleXAxis(gr_filtered, plotXAxisUnitScaling);
  TGraphErrors *gr_noise_plot = rescaleXAxis(gr_noise, plotXAxisUnitScaling);
  
  TMultiGraph *mgraph = new TMultiGraph();      
   mgraph->Add(gr_raw_plot,  "L");       
   mgraph->Add(gr_noise_plot,"L");
   mgraph->Add(gr_filtered_plot,"L");

   
   gr_raw_plot->SetLineColor(1);
   gr_raw_plot->SetLineWidth(1);
   gr_raw_plot->SetLineStyle(1);
   gr_filtered_plot->SetLineColor(2);
   gr_filtered_plot->SetLineWidth(2);
   gr_filtered_plot->SetLineStyle(1);
   gr_noise_plot->SetLineColor(4);
   gr_noise_plot->SetLineWidth(1);
   gr_noise_plot->SetLineStyle(1);
   
 // plot time domain traces
 TCanvas *c1 = new TCanvas("stepResponse","step response",200,10,1200,900);
  c1->SetGridx();
  c1->SetGridy();


 mgraph->Draw("A");

 TH1F *hist = mgraph->GetHistogram();
 hist->GetYaxis()->SetTitleOffset(1.25);

 hist->SetYTitle("amplitude [a.u]");
 hist->SetXTitle("time [us]");
 hist->GetYaxis()->SetRangeUser(-0.1, +1.7); 
 //hist->GetXaxis()->SetRangeUser(0, 14); 


 TLegend *legend = new TLegend(0.6,0.83,0.975,0.975);
   legend->AddEntry(gr_raw_plot,"step response","l");        
   legend->AddEntry(gr_filtered_plot,"filtered response","l");   
   legend->AddEntry(gr_noise_plot,"response with noise","l");
   legend->SetFillColor(0);
   legend->Draw();
      
 c1->Modified();
 c1->Update();
 
}

