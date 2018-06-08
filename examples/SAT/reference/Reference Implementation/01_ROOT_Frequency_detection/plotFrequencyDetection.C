#include "TMath.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TMultiGraph.h"
#include "TColor.h"
#include "TRandom.h"
#include "TSpectrum.h"
#include "TTimeStamp.h"

#include "TCanvas.h"
#include "TH1F.h"
#include "TH1D.h"
#include "TH1.h"
#include "TF1.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TMath.h"

#include "TLegend.h"


#include "FourierTransform.h"
#include "PeakFitter.h"

#ifndef PI
#define PI  3.141592653589793238462643383279502884197169399375
#endif

#ifndef TWO_PI
#define TWO_PI  (2*PI)
#endif

#ifndef DegToRad
#define DegToRad (TWO_PI/360)
#endif



void plotFrequencyDetection()
{

  TGraph *grp_gauss = new TGraph();
  grp_gauss->SetLineColor(4);
  grp_gauss->SetFillColor(4);
  grp_gauss->SetLineWidth(2);

  
  
  
   
  Int_t n_samples=1024;
  Int_t damping=100;
  Int_t nOverS = 2;  
  
  Int_t magSize = nOverS*n_samples/2;
  Double_t *magnitudeSpectrum = new Double_t[magSize]; 

  FourierTransform fourierTrafo(n_samples, nOverS);  
  SpectrumPeakFitter peakFitter(magSize);
  
  Double_t *input = new Double_t[n_samples];
  memset(input, 0x01, n_samples*sizeof(Double_t));
  
  
  


  for (Int_t j=0; j< 1999-1; j+=1) {
	if (j%200==0) printf("%f %% done\n", ((double)j)/2000.0*100.0);
	Double_t freq = 328.0/(double)(n_samples) -0.5/(double)(n_samples) + 1.0/(double)(n_samples)/1999.0*(j+1);
	//Double_t freq = 0.31 -0.5/(double)(n_samples) + 1.0/(double)(n_samples)/1999.0*(j+1);
	
	for (Int_t i=0; i< n_samples; i++) {
		Double_t time=i;
		Double_t t0 = 100*0;
		Double_t noise = gRandom->Gaus(0, 0.1);
		Double_t dampi = exp(-time/damping)/exp(-t0/damping);
		Double_t mod   = 0*0.001*sin(TMath::TwoPi()*0.005*time);
		dampi=1.0;
		Double_t val = dampi*sin(TMath::TwoPi()*(freq*(time-t0) + mod))+noise;
		if (i<t0) val=noise;
        
        
        
        
        //val = sin(TMath::TwoPi()*freq*(time-t0))+noise;
		val*= window_hann(i, (nOverS*n_samples)); //window
		//val*= window_hann(i, (nOverS*n_samples), 2); //window
        
        input[i] = val;
  	}
   
    fourierTrafo.SetPoints(input);
    fourierTrafo.GetMagnitudeSpectrum(magnitudeSpectrum);

   //return;


    Double_t val_gauss = peakFitter.gaussPeakPositionEstimate(magnitudeSpectrum, 0.0, 0.5);
	
		
	Double_t val_x = (freq-328.0/(double)(n_samples))*(nOverS*n_samples);
	Double_t ref = val_x*0.5;
	
    if (ref==0) ref=1e6;
	ref=1;
	
	grp_gauss->SetPoint(j,  val_x, fabs(val_gauss-freq*(nOverS*n_samples))/ref);
    
    //printf("freq %f val_gauss %f diff %+f\n", freq*(nOverS*n_samples), val_gauss, (freq*(nOverS*n_samples)-val_gauss));
  }


 grp_gauss->Sort();

 TMultiGraph *mgraph = new TMultiGraph();
   mgraph->Add(grp_gauss,"LE3");


 TCanvas *c1 = new TCanvas("c1","interpolation error",200,10,1200,900);
  c1->SetGridx();
  c1->SetGridy();
  c1->SetLogy();

 mgraph->Draw("A");

 TH1F *hist = mgraph->GetHistogram();
 hist->GetYaxis()->SetTitleOffset(1.2);
 hist->SetYTitle("|#Deltaf_{err}| [1/n]");
 hist->SetXTitle("#Deltaf [1/n]");
 hist->GetXaxis()->SetRangeUser(0.001,+0.51);
 hist->GetYaxis()->SetRangeUser(1e-7,2e3);

  TLegend *legend = new TLegend(0.75,0.85,0.975,0.975);   
   legend->AddEntry(grp_gauss, "Gaussian","l");
   legend->SetFillColor(0);
   legend->Draw();
 hist->SetStats(0);
 hist->SetTitle(0);
}
