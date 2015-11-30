#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H

// #define ARMA_NO_DEBUG // enable for no bounds checking

#include <armadillo>
#include "../storage/backends.hpp"
#include "../config.hpp"

using namespace arma;
using namespace std;

class SpecParams
{
  private:
    int get_nfft(int pad);
    float clip_time(int nsamples, float time);
    float get_valid_start_time(int nsamples);
    float get_valid_end_time(int nsamples);
    int get_nsamples(int data_len, float duration);
    int get_nblocks(int nsamples);
    int get_nfreqs();

  public:
    string mrn; // patient medical record number
    StorageBackend* backend; // array storage backend
    float start_time; // start time of the spectrogram
    float end_time; // end time of spectogram
    int start_offset; // start offset of raw data
    int end_offset; // end offset of raw data
    int spec_start_offset; // start offset of spectrogram data
    int spec_end_offset; // start offset of spectrogram data
    int fs; // sample rate
    int nfft; // number of samples for fft
    int nstep; // number of steps
    int shift; // shift size for windows
    int nsamples; // number of samples in the spectrogram
    int nblocks; // number of blocks
    int nfreqs; // number of frequencies

    void print();
    SpecParams(StorageBackend* backend,
        string mrn, float start_time, float end_time);
};

// does not need spec params
void eeg_spectrogram_wrapper(string mrn, float start_time,
                              float end_time, int ch, fmat& spec_mat);
void eeg_spectrogram(SpecParams* spec_params, int ch, fmat& spec_mat);
void precompute_spectrogram(string mrn, StorageBackend* backend);

#endif // SPECTROGRAM_H

