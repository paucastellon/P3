/// @file

#include <iostream>
#include <math.h>
#include "pitch_analyzer.h"
#include <fstream>
using namespace std;

/// Name space of UPC
namespace upc {
  void PitchAnalyzer::autocorrelation(const vector<float> &x, vector<float> &r) const {

    for (unsigned int l = 0; l < r.size(); ++l) {
  		/// \TODO Compute the autocorrelation r[l]
      /// \DONE autocorrelacion calculada.
      r[l]=0;
      for(unsigned int n=l; n<x.size();n++){
        r[l] += x[n]*x[n-l];
      }
      r[l] = r[l]/x.size();
    }

    if (r[0] == 0.0F) //to avoid log() and divide zero 
      r[0] = 1e-10; 
  }

  void PitchAnalyzer::set_window(Window win_type) {
    if (frameLen == 0)
      return;

    window.resize(frameLen);

    switch (win_type) {
    case HAMMING: {
      /// \TODO Implement the Hamming window
      /// \DONE Implementació de la finestra de Hamming
        float a0 = 0.53836;
        float a1 = 0.46164;
        float N = frameLen;
        vector<float> hamming(N);

        for(unsigned int n = 0;n<N-1; n++){
          hamming[n] = a0-a1*cos(2*M_PI*n/(N-1));
        }

      // //Creación de fichero para comprobar la correcta creación de la ventana de hamming
      //   std::string filename = "hamming.txt";
      //   std::ofstream file(filename);
      //   for(const float &value : hamming){
      //     file << value << std::endl;
      //   }
      //     file.close();

      window=hamming;
      break;
    }
    
    case RECT:
    default:
      window.assign(frameLen, 1);
    }
  }

  void PitchAnalyzer::set_f0_range(float min_F0, float max_F0) {
    npitch_min = (unsigned int) samplingFreq/max_F0;
    if (npitch_min < 2)
      npitch_min = 2;  // samplingFreq/2

    npitch_max = 1 + (unsigned int) samplingFreq/min_F0;

    //frameLen should include at least 2*T0
    if (npitch_max > frameLen/2)
      npitch_max = frameLen/2;
  }

  bool PitchAnalyzer::unvoiced(float pot, float r1norm, float rmaxnorm) const {
    /// \TODO Implement a rule to decide whether the sound is voiced or not.
    /// * You can use the standard features (pot, r1norm, rmaxnorm),
    ///   or compute and use other ones.
    /// \DONE Situat el llindar al màxim secundari normalitzat de la autocorrelació i condició de r1norm.
    float th_1 = 0.55; 
    if(rmaxnorm<this->llindar_rmax){
      return true;
    }
    if(r1norm<th_1){
      return true;
    }
    return false;
  }

  float PitchAnalyzer::compute_pitch(vector<float> & x) const {
    if (x.size() != frameLen)
      return -1.0F;

    //Window input frame
    for (unsigned int i=0; i<x.size(); ++i)
      x[i] *= window[i];

    vector<float> r(npitch_max);

    //Compute correlation
    autocorrelation(x, r);
    
    //vector<float>::const_iterator iR = r.begin(), iRMax = iR;

    /// \TODO 
	/// Find the lag of the maximum value of the autocorrelation away from the origin.<br>
	/// Choices to set the minimum value of the lag are:
	///    - The first negative value of the autocorrelation.
	///    - The lag corresponding to the maximum value of the pitch.
  ///   In either case, the lag should not exceed that of the minimum value of the pitch.
  /// \DONE 
  ///Calculat el màxim de la autocorrelació i guardat a rMax.
    ///	   .

    float rMax = r[npitch_min];
    unsigned int lag = npitch_min;
    
    for(unsigned int l = npitch_min; l<npitch_max; l++){
      if(r[l]>rMax){
        lag = l;
        rMax = r[l];
      }
    }
  
    float pot = 10 * log10(r[0]);

    //You can print these (and other) features, look at them using wavesurfer
    //Based on that, implement a rule for unvoiced
    //change to #if 1 and compile
#if 0
    if (r[0] > 0.0F)
      cout << pot << '\t' << r[1]/r[0] << '\t' << r[lag]/r[0] << endl;
#endif
    
    if (unvoiced(pot, r[1]/r[0], r[lag]/r[0]))
       return 0;
            
    else
      return (float) samplingFreq/(float) lag;
  }
}