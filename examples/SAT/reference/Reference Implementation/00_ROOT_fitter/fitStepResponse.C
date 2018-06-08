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

#include "RootFitterWrapper.h"

void fitStepResponse()
{
  
  Double_t samplingPeriod = 1.0/(500e6); // [s] digitizer sampling period (e.g. 1/(500 MS/s))    
  Int_t    nSamples = 2000;              // number of samples acquired by digitizer
  
  Double_t amplitude = 1.1;       // [a.u.] step response amplitude (e.g. RF kicker amplitude in [kV])
  Double_t noiseAmplitude = 0.02; // [a.u.] measurement noise
  Double_t timeOffset = 500e-9;   // [s] step response offset (e.g. RF kicker delay [s])    
  Double_t kickerTimeConstant = 200e-9;     // [s] second-order time-constant (N.B. no 'two pi') 1/\omega_0
  Double_t dampingConstant = 0.05;          // [] second-order damping coefficient \zeta
  
  
  
  // declare some graphs for plotting
  Bool_t applyFilter = kTRUE;
  Bool_t addNoise = kTRUE;
  TGraphErrors *gr_input    = generateStepResponse(nSamples, samplingPeriod, amplitude, timeOffset, kickerTimeConstant, dampingConstant, noiseAmplitude, applyFilter, addNoise);
 
   
  TFormula *testFormula1 = new TFormula("test","2*x*[0]+2");  
  Info("main()","testFormula1->IsValid() = %i (should be '1')\n", testFormula1->IsValid());
  TFormula *testFormula2 = new TFormula("test","2*x*[0]+2+b");  
  Info("main()","testFormula2->IsValid() = %i (should be '0')\n", testFormula2->IsValid());
  printf("\n\n\n");
  
  
  
//   Double_t fitResponse(TGraphErrors *input, const Char_t* formula, const Double_t fittingRangeMinX, const Double_t fittingRangeMaxX, 
//                     const Int_t nParameters, Char_t* parameterName[], Double_t parameterValue[], Double_t parameterError[], Double_t parameterRangeMin[], Double_t parameterRangeMax[], 
//                     const Char_t *fitterOptions = "0NQR") {
//       [..]
//   }
   const Char_t *formula = "[1]*(0.5+0.5*TMath::Erf((x-[0])/[2]))";
   const Double_t fittingRangeMinX = 0;
   const Double_t fittingRangeMaxX = nSamples*samplingPeriod;
   const Int_t nParameters = 3;
   Char_t *parameterName[] = {"delay", "amplitude", "slope"};
   Char_t *parameterUni[] = {"s", "V", "a.u."}; 
   Double_t parameterInitialValue[] = { 2.0e-6, 1.0, 1.0};
   Double_t *parameterError = new Double_t[nParameters];
   Double_t parameterRangeMin[] = { fittingRangeMinX, 0.0, 0.0};
   Double_t parameterRangeMax[] = { fittingRangeMaxX, 2.0, 10.0};
   const Char_t *fitterOptions = "Q0NER";
      
   Info("main()","start fitting routine");
   Double_t chiSquare = fitResponse(gr_input, formula, fittingRangeMinX, fittingRangeMaxX, nParameters, parameterName, parameterInitialValue, parameterError, parameterRangeMin, parameterRangeMax, fitterOptions);
   Info("main()","finished fitting routine chisquare/NDF =%f", chiSquare);   
   
   
   // create function to draw parameterized function
   TF1 *f1 = new TF1("f1", formula, fittingRangeMinX, fittingRangeMaxX); 
   for (int i=0; i < nParameters; i++) {
        f1->SetParameter(i, parameterInitialValue[i]);
        f1->SetParLimits(i, parameterRangeMin[i], parameterRangeMax[i]);
        f1->SetParName(i, parameterName[i]);         
        //printf("parameter %10s = %+e +- %+e\n", parameterName[i], parameterInitialValue[i], parameterError[i]);
   }    
   
  // recompute fitting (for debugging purposes) 
  if (kFALSE) {
        Info("main()","start manual fitting routine");
        gr_input->Fit(f1,"0NER");  
        printf("\n\n\n");
  }
   
  TF1 *f2 = new TF1("f2","(x>[0])*[1] *(1-exp(-[3]*[2]*(x-[0]))/sqrt(1-[3]^2)*cos([2]*sqrt(1-[3]^2)*(x-[0])-atan([3]/sqrt(1-[3]^2))))", fittingRangeMinX, fittingRangeMaxX); 
   f2->SetParameter(0, 0);
   f2->SetParLimits(0, fittingRangeMinX, fittingRangeMaxX);
   f2->SetParName(0,   "delay");
   
   f2->SetParameter(1, 1.5);
   f2->SetParLimits(1, 0.0, 2.0);
   f2->SetParName(1,   "amplitude");
   
   f2->SetParameter(2, 2*TMath::TwoPi()/kickerTimeConstant);
   //f2->SetParLimits(2,0.0, 2.0);
   f2->SetParName(2, "omega_n");
   
   
   f2->SetParameter(3, 0.05);
   f2->SetParLimits(3, 0.0, 0.9);
   f2->SetParName(3,   "zeta");
   
   // fit to second-order response -- analytic description
   gr_input->Fit(f2, "0NER"); 
   
   
   
   
  TF1 *func = f2;
  for (int n=0; n < func->GetNpar(); n++) {
        printf("parameter %i = %lf +- %lf\n", n, func->GetParameter(n), func->GetParError(n));
  }
  printf("nParameters = %i\n", func->GetNpar());
  printf("Ndf = %i\n", func->GetNDF());
  printf("chi-square/NDF/nPar =%f\n", func->GetChisquare()/func->GetNDF());
  
  
  
  
  TGraphErrors *gr_fit1 = new TGraphErrors(*gr_input);
  for (int i=0; i < gr_fit1->GetN(); i++) {
        Double_t x,y;
        gr_fit1->GetPoint(i,x,y);
        
        y= f1->Eval(x);
        //printf("%i: %+e %+e\n", i, x,y);
        gr_fit1->SetPoint(i,x,y);
  }
  
  
  TGraphErrors *gr_fit2 = new TGraphErrors(*gr_input);
  for (int i=0; i < gr_fit2->GetN(); i++) {
        Double_t x,y;
        gr_fit2->GetPoint(i,x,y);
        
        y= f2->Eval(x);
        //printf("%i: %+e %+e\n", i, x,y);
        gr_fit2->SetPoint(i,x,y);
  }
  
  
  
  
  
  
  
  
  // declare some graphs for plotting and formating
  Double_t plotXAxisUnitScaling = 1e6; // [us] time base for plotting
  TGraphErrors *gr_input_plot = rescaleXAxis(gr_input, plotXAxisUnitScaling);
  TGraphErrors *gr_fit1_plot = rescaleXAxis(gr_fit1, plotXAxisUnitScaling);
  TGraphErrors *gr_fit2_plot = rescaleXAxis(gr_fit2, plotXAxisUnitScaling);
  
  TMultiGraph *mgraph = new TMultiGraph();      
   mgraph->Add(gr_input_plot,"L");
   mgraph->Add(gr_fit1_plot,"L");
   mgraph->Add(gr_fit2_plot,"L");
    
   gr_input_plot->SetLineColor(4);
   gr_input_plot->SetLineWidth(1);
   gr_input_plot->SetLineStyle(1);
   
   gr_fit1_plot->SetLineColor(3);
   gr_fit1_plot->SetLineWidth(1);
   gr_fit1_plot->SetLineStyle(2);
   
   gr_fit2_plot->SetLineColor(2);
   gr_fit2_plot->SetLineWidth(1);
   gr_fit2_plot->SetLineStyle(1);
   
 // plot time domain traces
 TCanvas *c1 = new TCanvas("fitStepResponse","fit step response example",200,10,1200,900);
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
   legend->AddEntry(gr_input_plot,"raw response","l");
   legend->AddEntry(gr_fit1_plot,"simple fit","l");
   legend->AddEntry(gr_fit2_plot,"complex fit","l");
   legend->SetFillColor(0);
   legend->Draw();
      
 c1->Modified();
 c1->Update();
 
}

