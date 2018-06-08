// @(#)src/:$Name:  $:$Id: IIRFilter,v 1.0 2010/09/20 14:41:33 rstein Exp $
// Author: Ralph J. Steinhagen 2010-09-20  <http://consult.cern.ch/xwho/people?Ralph+Steinhagen>
#ifndef IIR_FILTER_H_
#define IIR_FILTER_H_

class LinearInterpolation {
private:
	Double_t fvalue;
	Double_t fset;
	Double_t frate;
	Double_t ftimeConstant;
	Double_t fsampling;

public:
	LinearInterpolation(Double_t initalValue, Double_t timeConstant, Double_t Ts)
	{
		fvalue = fset = initalValue;
		ftimeConstant = timeConstant;
		fsampling = Ts;
		frate = 0.0;
	}

	void SetTimeConstant(Double_t val)
	{
		if (ftimeConstant <= 0.0) {
			ftimeConstant = 1.0;
		}

		ftimeConstant = val;
		frate = (fset - fvalue) / ftimeConstant * fsampling;
	}

	Double_t update(Double_t newValue, Bool_t instantaneous = kFALSE)
	{
		if (instantaneous == kTRUE) {
			fset = fvalue = newValue;
			frate = 0.0;
			return fvalue;
		}

		if (newValue != fset) {
			fset = newValue;
			frate = (fset - fvalue) / ftimeConstant * fsampling;
		}

		if (fabs(fvalue - fset) > fabs(frate)) {
			fvalue += frate;
		} else {
			fvalue = fset;
		}

		return fvalue;
	}
};

class FirstOrder {
private:
	Double_t fvalue;
	Double_t fset;
	Double_t ftimeConstant;
	Double_t fsampling;

public:
	FirstOrder(Double_t initalValue, Double_t timeConstant, Double_t Ts)
	{
		fvalue = fset = initalValue;
		ftimeConstant = timeConstant;
		fsampling = Ts;
	}

	void SetTimeConstant(Double_t val)
	{
		if (ftimeConstant == val) {
			return;
		}

		if (ftimeConstant <= 0.0) {
			ftimeConstant = 1.0;
		}

		ftimeConstant = val;
	}

	Double_t update(Double_t newValue, Bool_t instantaneous = kFALSE)
	{
		if (instantaneous == kTRUE) {
			fset = fvalue = newValue;
			return fvalue;
		}

		if (newValue != fset) {
			fset = newValue;
		}

		Double_t alpha = TMath::Pi() * fsampling / (fsampling + ftimeConstant);
		fvalue = (1 - alpha) * fvalue + alpha * fset;

		return fvalue;
	}
};

class SecondOrder {
private:
	Double_t fvalue;
	Double_t fx[3], fy[3];
	Double_t a0, a1, a2;
	Double_t b0, b1, b2;
	Double_t fset;
	Double_t ftimeConstant;
	Double_t fsampling, fdamping;
	Bool_t fdcUnityGain;

	void calculateFilterCoefficients(Bool_t dcUnityGain)
	{
		for (int i = 0; i < 3; i++) {
			fx[i] = fset;
			fy[i] = fset;
		}

		Double_t bw = fdamping; // bandwidth in radians
		Double_t omega = TMath::TwoPi() * fsampling / ftimeConstant; // resonance frequency in terms of sampling frequency
		Double_t r = 1.0 - bw / 2.0;
		//Double_t r = 0.99;

		b0 = 1.0;
		b1 = 2.0 * r * TMath::Cos(omega);
		b2 = -r * r;

		// resonator with zero gain at resonance frequency
		if (!dcUnityGain) {
			a0 = (1.0 - r * r) / 2.0;
			a1 = 0.0;
			a2 = -1.0 * a0;
		} else {
			// resonator with zero DC gain
			a0 = 1.0 - b1 - b2;
			a1 = 0;
			a2 = 0;
		}
	}

public:
	SecondOrder(Double_t initalValue, Double_t timeConstant, Double_t damping, Double_t Ts, Bool_t dcUnityGain = kTRUE)
	{
		fvalue = fset = initalValue;
		ftimeConstant = timeConstant;
		fdamping = damping;
		fsampling = Ts;
		fdcUnityGain = dcUnityGain;

		calculateFilterCoefficients(fdcUnityGain);
	}

	void SetTimeConstant(Double_t timeConstant, Double_t damping)
	{
		if (timeConstant == ftimeConstant && damping == fdamping) {
			return;
		}

		if (timeConstant <= 0.0) {
			ftimeConstant = 1.0;
		}

		ftimeConstant = timeConstant;
		fdamping = damping;
		calculateFilterCoefficients(fdcUnityGain);
	}

	Double_t update(Double_t newValue, Bool_t instantaneous = kFALSE)
	{
		if (instantaneous == kTRUE) {
			fset = fvalue = newValue;
			for (int i = 0; i < 3; i++) {
				fx[i] = fset;
				fy[i] = fset;
			}
			return fvalue;
		}

		if (newValue != fset) {
			fset = newValue;
		}

		fx[0] = fset;
		fy[0] = (a0 * fx[0] + a1 * fx[1] + a2 * fx[2] + b1 * fy[1] + b2 * fy[2]) / b0;

		// shift value registers
		fx[2] = fx[1];
		fx[1] = fx[0];

		fy[2] = fy[1];
		fy[1] = fy[0];

		fvalue = fy[0];

		return fvalue;
	}
};

class Cubic {
private:
	Double_t fvalue;
	Double_t fset;
	Double_t ftimeConstant;
	Double_t fsampling;
	FirstOrder* fterm1;
	SecondOrder* fterm2;

public:
	Cubic(Double_t initalValue, Double_t timeConstant, Double_t Ts)
	{
		fvalue = fset = initalValue;
		ftimeConstant = timeConstant;
		fsampling = Ts;
		fterm1 = new FirstOrder(fset, ftimeConstant, 1.5 * fsampling);
		fterm2 = new SecondOrder(fset, 3 * ftimeConstant, 1.2 * TMath::TwoPi() * (fsampling / (ftimeConstant)), 1.5 * fsampling);
	}

	void SetTimeConstant(Double_t timeConstant)
	{
		if (timeConstant == ftimeConstant) {
			return;
		}

		if (timeConstant <= 0.0) {
			ftimeConstant = 1.0;
		}

		ftimeConstant = timeConstant;

		if (fterm1 != NULL) delete fterm1;
		if (fterm2 != NULL) delete fterm2;

		fterm1 = new FirstOrder(fset, ftimeConstant, 1.5 * fsampling);
		fterm2 = new SecondOrder(fset, 3 * ftimeConstant, 1.2 * TMath::TwoPi() * (fsampling / (ftimeConstant)), 1.5 * fsampling);
	}

	Double_t update(Double_t newValue, Bool_t instantaneous = kFALSE)
	{
		if (instantaneous == kTRUE) {
			fset = fvalue = newValue;
			fterm1->update(fset, instantaneous);
			fterm2->update(fset, instantaneous);
			return fvalue;
		}

		if (newValue != fset) {
			fset = newValue;
		}

		//Double_t alpha = TMath::Pi() * fsampling / (fsampling + ftimeConstant);
		fvalue = fterm1->update(fterm2->update(fset));

		return fvalue;
	}
};


class VariableSecondOrder {
private:
	Bool_t   fisLowPass;
	Double_t fvalue;
	Double_t fx[3], fy[3];
	Double_t a0, a1, a2;
	Double_t b0, b1, b2;
	Double_t fset;
	Double_t ftimeConstant;
	Double_t fsampling;
	Double_t fn;
	Int_t    ffilterType;

	void calculateFilterCoefficients(Int_t filterType)
	{
		for (int i = 0; i < 3; i++) {
			fx[i] = fset;
			fy[i] = fset;
		}
		
		// implements H(s) = g / (s^2 + p*s + g) filter
		
		// pole choices
		Double_t filterCoefficient[3][2] = {
		  { /* p= */ 1, /*g=*/ sqrt(2)}, // Butterworth filter
		  { /* p= */ 1, /*g=*/ 2},       // critically damped filter
		  { /* p= */ 3, /*g=*/ 3}};      // Bessel filter
		  
		if (filterType<0 || filterType>2) {
		  filterType = 0;
		}
		
		Double_t n = fn; // number of filter passes
		Double_t p = filterCoefficient[filterType][0];
		Double_t g = filterCoefficient[filterType][1];

		Double_t fnorm = 1; // normalised cutoff frequency
		if ((ftimeConstant>0) && (fsampling>0)) {
		    fnorm = fsampling/ftimeConstant;
		}
		
		Double_t c; // correction factor for 3dB frequency pass
		switch (filterType) {
		  case 1: { // critically damped
		      c = 1.0/TMath::Sqrt((TMath::Power(2,1.0/(2*n))-1));
		  } break;
		  case 2: { // Bessel
		      if (fisLowPass) {
			c = 1.0/TMath::Sqrt(TMath::Sqrt(TMath::Power(2,1.0/n)-0.75)-0.5)/TMath::Sqrt(3);
		      } else {
			c = 1.0/TMath::Sqrt(TMath::Sqrt(TMath::Power(2,1.0/n)-0.75)-0.5)*TMath::Sqrt(3);
		      }
		  }
		  default:
		  case 0: { // Butterworth
		      c = TMath::Power(TMath::Power(2,1.0/n) -1, -0.25);
		  }
		}		
		
		Double_t fcorr = fisLowPass?(c*fnorm):(0.5- c*fnorm);
		
		if (fisLowPass) {
		  // ensure stability criterion
		  if ((fcorr>=0) && (fcorr<(1.0/8.0))) {
		    // OK
		  } else {
		    // not OK
		    fcorr = (1.0/8.0);
		  }
		} else {
		   // ensure stability criterion
		  if ((fcorr>=(3.0/8.0)) && (fcorr<(1.0/2.0))) {
		    // OK
		  } else {
		    // not OK
		    fcorr = (3.0/8.0);
		  }
		}
		
		Double_t omega0 = TMath::Tan(TMath::Pi()*fcorr);
		printf("fnorm = %f, c = %f, omega = %f\n", fnorm, c, omega0);
		
		Double_t K1 = p*omega0;
		Double_t K2 = g*omega0*omega0;
		
		if (fisLowPass) {
		  a0 = K2/(1 +K1 +K2);
		  a1 = 2*a0;
		  a2 = a0;
		  b0 = 1.0;
		  b1 = 2*a0*(1.0/K2-1);
		  b2 = 1 - (a0 +a1 +a2 +b1);
		} else {
		  a0 = K2/(1 +K1 +K2);
		  a1 = -2*a0;
		  a2 = a0;
		  b0 = 1.0;
		  b1 = -2*a0*(1.0/K2-1);
		  b2 = 1 - (a0 +a1 +a2 +b1);		  
		}
	}

public:
	VariableSecondOrder(Double_t initalValue, Double_t timeConstant, Int_t filterType, Double_t Ts, Double_t n = 1, Bool_t isLowPass = kTRUE)
	{
		fvalue = fset = initalValue;
		ftimeConstant = timeConstant;
		ffilterType = filterType;
		fsampling = Ts;
		fn = n;
		fisLowPass = isLowPass;
		calculateFilterCoefficients(filterType);
	}

	void SetTimeConstant(Double_t timeConstant)
	{
		if (timeConstant == ftimeConstant) {
			return;
		}

		if (timeConstant <= 0.0) {
			ftimeConstant = 1.0;
		}

		ftimeConstant = timeConstant;
		calculateFilterCoefficients(ffilterType);
	}

	Double_t update(Double_t newValue, Bool_t instantaneous = kFALSE)
	{
		if (instantaneous == kTRUE) {
			fset = fvalue = newValue;
			for (int i = 0; i < 3; i++) {
				fx[i] = fset;
				fy[i] = fset;
			}
			return fvalue;
		}

		if (newValue != fset) {
			fset = newValue;
		}

		fx[0] = fset;
		fy[0] = (a0 * fx[0] + a1 * fx[1] + a2 * fx[2] + b1 * fy[1] + b2 * fy[2]) / b0;

		// shift value registers
		fx[2] = fx[1];
		fx[1] = fx[0];

		fy[2] = fy[1];
		fy[1] = fy[0];

		fvalue = fy[0];

		return fvalue;
	}
};

#endif /*IIR_FILTER_H_*/
