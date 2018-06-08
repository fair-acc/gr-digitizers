// @(#)src/:$Name:  $:$Id: PeakFitter,v 1.0 2017/07/24 14:41:33 rstein Exp $
// Author: Ralph J. Steinhagen 2017-05-24 R.Steinhagen@GSI.de
#ifndef FOURIER_TRANSFORM_H_
#define FOURIER_TRANSFORM_H_


double window_hann(int j, int n)
{
    double a = 2.0*PI/(n-1), w;

    w = 0.5 - 0.5*cos(a*j);
    return (w);
}

double window_blackman(int j, int n)
{
    double a = 2.0*PI/(n-1), w;

    w = 0.42 - 0.5*cos(a*j) + 0.08*cos(2*a*j);
    return (w);
}

double window_hamming(int j, int n)
{
    /* double a = 2.0*PI/(n-1), w; */
    /* The BBQ FPGA uses the following slightly different hamming definition */
    double a = 2.0*PI/(n), w;   

    w = 0.53836 - 0.46164*cos(a*j);
    return (w);
}

double window_exp(int j, int n)
{
    /* double a = exp(i/(3*n)); */
    double a = 3.0*n, w;   

    w = exp(j/a)/exp(0);
    return (w);
}


double window_hann(int j, int n, int m)
{
    double a = PI/(double)n, w;

    w = 1.0*pow(sin(a*j), m);
    return (w);
}


class FourierTransform : TObject {
    private:
        Int_t fnSamples;
        Int_t foverSamplingFactor;
        Int_t fnSize;
        Double_t verNormalise; // vertical spectrum normalisation factor '1/N' (FFTW definition)
        
        TVirtualFFT *flocalFFTW;
    
    public:
        
        FourierTransform(Int_t nSamples, Int_t overSamplingFactor = 2) {
             const Char_t *functionName = "FourierTransform(Int_t nSamples)";
        
            if (nSamples <=0) {
                Error(functionName, "nSamples %i is <= 0", nSamples);
            }        
            fnSamples = nSamples;
            
            if (nSamples < 1) {
                Error(functionName, "foverSamplingFactor %i is < 1 -> set to '1'", nSamples);
                foverSamplingFactor = 1;
            }        
            foverSamplingFactor = overSamplingFactor;
            
            fnSize = fnSamples*overSamplingFactor;
            verNormalise = 1.0/fnSize;
            
            Info(functionName, "plan speed optimised FFTW plan -- this may take some time");
            //flocalFFTW = TVirtualFFT::FFT(1, &fnSize, "R2C ES K"); // rough estimate
            //flocalFFTW = TVirtualFFT::FFT(1, &fnSize, "R2C P K"); // patient estimate
            flocalFFTW = TVirtualFFT::FFT(1, &fnSize, "R2C EX K"); // exhaustive optimal estimate
            
            Info(functionName, "plan speed optimised FFTW plan -- this may take some time -- DONE");
            if (!flocalFFTW) {
                Error(functionName, "could not allocate TVirtualFFT::FFT(..)");
                return;
            }
        }
                
        
        ~FourierTransform() {
            const Char_t *functionName = "~FourierTransform()";
            Info(functionName,"destructor colled");
            
            if (flocalFFTW == NULL) delete flocalFFTW;
        }
        
    
        void SetPoints(const Double_t *in) {

//             for (int i=0; i < fnSize; i++) {
//                     printf("input[%i] = %f\n", i, in[i]);
//             }
            for (Int_t i=0; i < fnSamples; i++) {              
                flocalFFTW->SetPoint(i, in[i]);
            }
            
            // zero padding
            for (Int_t i=fnSamples; i < fnSize; i++) {
                flocalFFTW->SetPoint(i, 0.0); 
            }
            
            //printf("after\n");
            flocalFFTW->Transform();
        }
        
        void GetMagnitudeSpectrum(Double_t *magnitudeSpectrum) {
                if (magnitudeSpectrum == NULL) {
                    magnitudeSpectrum = new Double_t[fnSize/2];
                }
                
                for (int i = 0; i < fnSize/2; i++) {
                    Double_t re, im;
                    flocalFFTW->GetPointComplex(i, re, im);
                    magnitudeSpectrum[i] = TMath::Sqrt(re*re + im*im)/verNormalise;
                    //printf("magnitudeSpectrum[%i] =%f\n", i, magnitudeSpectrum[i]);
                }
        }
    
};

#endif /*MY_FFT_CLASS_H_*/
