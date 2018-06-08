// @(#)src/:$Name:  $:$Id: RootFitterWrapper,v 1.0 2017/08/16 14:41:33 rstein Exp $
// Author: Ralph J. Steinhagen 2017-08-16
#ifndef ROOT_FITTER_WRAPPER_H_
#define ROOT_FITTER_WRAPPER_H_


#define MAX_PARAMETER_LENGTH 200

//#define VALID_PARAMETER_CHARACTER "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_"
#define VALID_PARAMETER_CHARACTER "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_+-*/^()[]{}:;,.&|=><!"


/*
 * Function to check input (potentially user-supplied) input for malformed (e.g. not terminated) strings, invalid/unsupported characters (buffer overflow protection)
 * 
 * author: R.Steinhagen@GSI.de
 * 
 */
Bool_t checkString(const Char_t* input) {    
    const Char_t *functionName="Bool_t checkString(..)";
    
    if (input == NULL) {
        Error(functionName, "const Char_t* input may not be NULL");
        return kFALSE;
    }
        
    Int_t len = strlen(input);
    if ((len < 0) || (len > MAX_PARAMETER_LENGTH)) {
        Error(functionName, "'strlen() = %i' must be < %i (MAX_PARAMETER_LENGTH)", len, MAX_PARAMETER_LENGTH);
        return kFALSE;
    }
    
    Int_t span = strspn(input, VALID_PARAMETER_CHARACTER);
    if (input[span] != 0) {
        Error(functionName, "found invalid/unsupported character in '%s' at position %i out of %i characters\n", input, span, len);
        return kFALSE;
    }
 
    // all string checks passed
    return kTRUE;
}

/*
 * 
 * Wrapper function around ROOT fitter classes. Implements some convenience functions and (more verbose/specific) input variable checks
 * 
 * fitterOptions:
 * "W"	Set all weights to 1; ignore error bars
 * "Q"	Quiet mode (minimum printing)
 * "V"	Verbose mode (default is between Q and V)
 * "E"	Perform better Errors estimation using Minos technique
 * "M"	More. Improve fit results. It uses the IMPROVE command of TMinuit (see TMinuit::mnimpr). This algorithm attempts to improve the found local minimum by searching for a better one.
 * "R"	Use the Range specified in the function range
 * "N"	Do not store the graphics function, do not draw
 * "0"	Do not plot the result of the fit. By default the fitted function is drawn unless the option "N" above is specified.
 * 
 * function default: "0NEQR"
 * 
 * author: R.Steinhagen@GSI.de
 * 
 * return:  chiSquare/NDF of the fit
 * 
 */

Double_t fitResponse(TGraphErrors *input, const Char_t* formula, const Double_t minX, const Double_t maxX, 
                     const Int_t nParameters, Char_t* parameterName[], Double_t parameterValue[], Double_t parameterError[], Double_t parameterRangeMin[], Double_t parameterRangeMax[], 
                     const Char_t *fitterOptions = "0NEQR") {
    
    const Char_t *functionName="Double_t fitResponse(..)";
    
    
    // check formula pointers        
    if (checkString(formula) == kFALSE) {
        Error(functionName, "malformed formula");
        return -1;
    }
        
    // check syntax of formula expression
    TFormula *testFormula = new TFormula("localTest",formula);
    if (!testFormula->IsValid()) {
        Error(functionName,"invalid formula '%s' - abort", formula);
        delete testFormula;
        return -1;
    } else {
        delete testFormula;
    }
    
    // check for NULL input  pointers
    if (input == NULL) {
        Error(functionName, "TGraphErrors *input may not be NULL");
        return -1;
    }

    // check for empty graphs
    Int_t nData = input->GetN();
    if (nData<=0) {
        Error(functionName, "TGraphErrors may not have 0 length");
        return -1;        
    }
    
    // check input parameter for NaN & INF
    for (int i=0; i < nData; i++) {
        Double_t x,y,ex,ey;
        input->GetPoint(i, x, y);
        ex = input->GetErrorX(i);
        ey = input->GetErrorX(i);
        
        if ((isfinite(x) == 0) || (isfinite(y) == 0)) {    
            Error(functionName, "(i=%i, x=%+e, y=%+e)-tuple contains infinite or NaN value", i, x, y);
            return -1;
        }
        
        if ((isfinite(ex) == 0) || (isfinite(ey) == 0)) {    
            Error(functionName, "(i=%i, ex=%+e, ey=%+e)-error tuple contains infinite or NaN value", i, ex, ey);
            return -1;
        }
    }
    
    Double_t xmin = TMath::MinElement(nData, input->GetX());
    Double_t xmax = TMath::MaxElement(nData, input->GetX());
    
    // check for NaN or +- INF
    if ((isfinite(xmin) == 0) || (isfinite(xmax) == 0)) {
        Error(functionName, "minX %+e or maxX %+e parameter is infinite or NaN", minX, maxX);
        return -1;
    }    
 
    // check fitting range consistency
    if ((maxX < minX) || (minX == maxX)) {
        Error(functionName, "minX %+e and maxX %+e parameters inverted or equal", minX, maxX);
        return -1;        
    }
    
    // check for parameter count
    if (nParameters < 0) {
        Error(functionName, "nParameters parameter smaller than zero %i", nParameters);
        return -1;        
    }
    
    // check for NaN, +- Inf, dimension etc. of subsequent vectors
    for (int i=0; i < nParameters; i++) {
        
        if (checkString(parameterName[i]) == kFALSE) {
            Error(functionName, "malformed parameter name at index %i", i);
            return -1;
        }
          
        if (isfinite(parameterValue[i]) == 0) {
            Error(functionName, "paramater-%i initial value '%+e' is infinite or NaN", i, parameterValue[i]);
            return -1;
        }
        
        if (isfinite(parameterError[i]) == 0) {
            Error(functionName, "paramater-%i initial error '%+e' is infinite or NaN", i, parameterError[i]);
            return -1;
        }
        
         if ((isfinite(parameterRangeMin[i]) == 0) || (isfinite(parameterRangeMax[i]) == 0)) {
            Error(functionName, "paramater-%i range [%+e, %+e] contains infinite or NaN value", i, parameterRangeMin[i], parameterRangeMax[i]);
            return -1;
        }
        
        if (parameterRangeMin[i] > parameterRangeMax[i]) {
            Error(functionName, "paramater-%i range [%+e, %+e] inverted", i, parameterRangeMin[i], parameterRangeMax[i]);
            return -1;
        }
        
        if ((parameterValue[i] < parameterRangeMin[i]) || (parameterValue[i]> parameterRangeMax[i])) {
            Error(functionName, "paramater-%i initial value %+e outside of range [%+e, %+e]", i, parameterValue[i], parameterRangeMin[i], parameterRangeMax[i]);
            return -1;
        }
        
    }
    
    // check for fitter options    
    if (checkString(fitterOptions) == kFALSE) {
        Error(functionName, "malformed fitterOptions");
        return -1;
        }
    
    
    
    TF1 *func = new TF1("f1",formula, minX, maxX); 
  
    for (int i=0; i < nParameters; i++) {
        func->SetParName(i, parameterName[i]);
        if (parameterRangeMin[i] != parameterRangeMax[i]) {
            func->SetParameter(i, parameterValue[i]);
            func->SetParLimits(i, parameterRangeMin[i], parameterRangeMax[i]);
        } else {
            // 'max-min' range is '0' -> fix parameter for fitting
            func->FixParameter(i, parameterValue[i]);
        }
        //func->SetParError(i, parameterError[i]);
        func->SetParError(i, 0.0);
    }
    
    input->Fit(func, fitterOptions);

    for (int i=0; i < nParameters; i++) {
        parameterValue[i] = func->GetParameter(i);    
        parameterError[i] = func->GetParError(i);
    }
   
    Double_t chiSquare = func->GetChisquare();
    Int_t    NDF = func->GetNDF();
   
   
    delete func;
        
    return chiSquare/NDF;
}

#endif /* ROOT_FITTER_WRAPPER_H_ */
