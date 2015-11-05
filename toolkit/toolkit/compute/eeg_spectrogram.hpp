#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H

// #define ARMA_NO_DEBUG // enable for no bounds checking
#include <armadillo>
#include "../storage/backends.hpp"
#include "../config.hpp"

using namespace arma;
using namespace std;

typedef struct spec_params
{
  string mrn; // patient medical record number
  StorageBackend* backend; // array storage backend
  float start_time; // start of the spectrogram
  float end_time; // end of spectogram
  int spec_len; // length of the spectrogram
  int fs; // sample rate
  int nfft; // number of samples for fft
  int nstep; // number of steps
  int shift; // shift size for windows
  int nsamples; // number of samples in the spectrogram
  int nblocks; // number of blocks
  int nfreqs; // number of frequencies
} spec_params_t;

void print_spec_params_t(spec_params_t* spec_params);
void get_eeg_spectrogram_params(spec_params_t* spec_params, StorageBackend* backend,
                                string mrn, float start_time, float end_time);
// does not need spec params
void eeg_spectrogram_wrapper(string mrn, float start_time,
                              float end_time, int ch, fmat& spec_mat);
void eeg_spectrogram(spec_params_t* spec_params, int ch, fmat& spec_mat);

#endif // SPECTROGRAM_H

