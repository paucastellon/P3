/// @file

#include <iostream>
#include <fstream>
#include <string.h>
#include <errno.h>
#include <cmath> 
#include <algorithm>
#include "wavfile_mono.h"
#include "pitch_analyzer.h"

#include "docopt.h"

#define FRAME_LEN   0.030 /* 30 ms. */
#define FRAME_SHIFT 0.015 /* 15 ms. */

using namespace std;
using namespace upc;

static const char USAGE[] = R"(
get_pitch - Pitch Estimator 

Usage:
    get_pitch [options] <input-wav> <output-txt>
    get_pitch (-h | --help)
    get_pitch --version

Options:
    --llindar-rmax FLOAT  llindar de decisio sonor o sord per al maxim secundari de la autocorrelacio [default: 0.41]
    -h, --help  Show this screen
    --version   Show the version of the project

Arguments:
    input-wav   Wave file with the audio signal
    output-txt  Output file: ASCII file with the result of the estimation:
                    - One line per frame with the estimated f0
                    - If considered unvoiced, f0 must be set to f0 = 0
)";

void CenterClipping(vector<float>& signal){
//Recorremos la señal para encontrar el máximo
  float max = fabs(signal[0]);
  float th = 0;
  for(long unsigned int i = 0; i<signal.size(); i++){
    if (fabs(signal[i])>max){
      max = fabs(signal[i]);
    }
  }
  //Situamos el umbral en un 8% del máximo de la señal, con el fin de tener en cuenta diferentes volúmenes
  // sonoros
  th = max * 0.08;

  //Eliminamos todo lo que esté por debajo del umbral
  for(long unsigned int i = 0; i<signal.size(); i++){
    if(fabs(signal[i])<=th){
      signal[i]=0;
    }
  }
}

//Coeficientes del filtro paso bajo (32 coeficientes)
std::vector<float> coeficientes = {
    0.0027, 0.0040, 0.0066, 0.0103, 0.0148, 0.0201, 0.0261, 0.0327, 0.0398, 0.0472,
    0.0548, 0.0625, 0.0701, 0.0775, 0.0847, 0.0914, 0.0977, 0.1036, 0.1090, 0.1138,
    0.1179, 0.1213, 0.1239, 0.1257, 0.1267, 0.1267, 0.1267, 0.1257, 0.1239, 0.1213,
    0.1179, 0.1138, 0.1090, 0.1036, 0.0977, 0.0914
};

//Creamos la función del filtro paso bajo
void LPF(const std::vector<float>& input, std::vector<float>& output, const std::vector<float>& coefficients) {
    size_t filterSize = coefficients.size();
    size_t inputSize = input.size();
    output.resize(inputSize, 0.0f);

    // Aplicamos la convolución
    for (size_t n = 0; n < inputSize; ++n) {
        for (size_t k = 0; k < filterSize; ++k) {
            if (n >= k) { // Verifica límites del vector
                output[n] += coefficients[k] * input[n - k];
            }
        }
    }
}

//Función que usamos para ordenar tramas
void ordenar(std::vector<float>& window) {
    for (long unsigned int i = 0; i < window.size() - 1; i++) {
        for (long unsigned int j = 0; j < window.size() - 1 - i; j++) {
            if (window[j] < window[j + 1]) { // Ordenamos de mayor a menor
                std::swap(window[j], window[j + 1]);
            }
        }
    }
}

//Función para calcular el filtro de mediana
void MedianFilter(std::vector<float>& signal) {
    int w_size = 5; // Tamaño de la ventana
    int half_w = (w_size + 1) / 2;  // Índice de la mediana (para ventana impar)

    // Recorremos todas las tramas de la señal
    for (size_t i = 0; i <= signal.size() - w_size; i++) {
        
        // Copiar la ventana actual en un nuevo vector
        std::vector<float> window(signal.begin() + i, signal.begin() + i + w_size);

        // Ordenar la ventana con el algoritmo de burbuja
        ordenar(window);
        
        // Tomar la mediana de la ventana ordenada (el valor en la mitad)
        float mediana = window[half_w - 1];  // La mediana está en la posición half_w-1
        
        // Asignar la mediana a todos los valores en la ventana en el vector original
        for (int j = 0; j < w_size; j++) {
            signal[i + j] = mediana;
        }
        i = i+w_size-1;
    }

   
}

int main(int argc, const char *argv[]) {
	/// \TODO
	///  Modify the program syntax and the call to **docopt()** in order to
	///  add options and arguments to the program.
  /// \DONE Afegida opció i paràmetre llindar rmax 
  
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
        {argv + 1, argv + argc},	// array of arguments, without the program name
        true,    // show help if requested
        "2.0");  // version string

	std::string input_wav = args["<input-wav>"].asString();
	std::string output_txt = args["<output-txt>"].asString();
  float llindar_rmax = stof(args["--llindar-rmax"].asString());

  // Read input sound file
  unsigned int rate;
  vector<float> x;
  if (readwav_mono(input_wav, rate, x) != 0) { 
    cerr << "Error reading input file " << input_wav << " (" << strerror(errno) << ")\n";
    return -2;
  }
  // x = Input signal
  // rate = sample freq

  int n_len = rate * FRAME_LEN; // Tamaño ventana
  int n_shift = rate * FRAME_SHIFT; // Tamaño desplazamiento 

  // Define analyzer
  PitchAnalyzer analyzer(n_len, rate, PitchAnalyzer::RECT, 50, 500, llindar_rmax);

  /// \TODO
  /// Preprocess the input signal in order to ease pitch estimation. For instance,
  /// central-clipping or low pass filtering may be used.
  /// \DONE Center clipping i filtre pas-baix aplicat

  vector<float> filtered_x;
  LPF(x,filtered_x,coeficientes);
   CenterClipping(filtered_x);

  // Iterate for each frame and save values in f0 vector
  vector<float>::iterator iX;
  vector<float> f0;
  for (iX = filtered_x.begin(); iX + n_len < filtered_x.end(); iX = iX + n_shift) {
    float f = analyzer(iX, iX + n_len);
    f0.push_back(f);
  }

  /// \TODO
  /// Postprocess the estimation in order to supress errors. For instance, a median filter
  /// or time-warping may be used.
  /// \DONE Filtre de mediana implementat
  MedianFilter(x);

   // Creación de fichero para comprobar el correcto funcionamiento del filtro de mediana
    std::string filename = "mediana.txt";
    std::ofstream file(filename);
    for (const float& value : x) {
        file << value << std::endl;
    }
    file.close();
 
  // Write f0 contour into the output file
  ofstream os(output_txt);
  if (!os.good()) {
    cerr << "Error reading output file " << output_txt << " (" << strerror(errno) << ")\n";
    return -3;
  }

  os << 0 << '\n'; //pitch at t=0
  for (iX = f0.begin(); iX != f0.end(); ++iX) 
    os << *iX << '\n';
  os << 0 << '\n';//pitch at t=Dur

  return 0;
}
