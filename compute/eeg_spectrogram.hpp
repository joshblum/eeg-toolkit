#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H

#include <fftw3.h>
// #define ARMA_NO_DEBUG // enable for no bounds checking
#include <armadillo>
#include "edflib.h"

#ifdef __cplusplus
extern "C" {
#endif

using namespace arma;

// add fields for hdr, filename and duration
typedef struct spec_params
{
  char* filename; // filename data is read from (stream name)
  float duration; // length of spectrogram produced
  int hdl; // file handler for hdr file
  int spec_len; // length of the spectrogram
  int fs; // sample rate
  int nfft; // number of samples for fft
  int nstep; // number of steps
  int shift; // shift size for windows
  int nsamples; // number of samples in the spectrogram
  int nblocks; // number of blocks
  int nfreqs; // number of frequencies
} spec_params_t;

typedef enum
{
  LL = 0,
  LP,
  RP,
  RL
} ch_t;

// map channel names to index
typedef enum
{
  C3 = 0,
  C4 = 1,
  O1 = 2,
  O2 = 3,
  F3 = 7,
  F4 = 8,
  F7 = 9,
  F8 = 10,
  FP1 = 12,
  FP2 = 13,
  P3 = 15,
  P4 = 16,
  T3 = 18,
  T4 = 19,
  T5 = 20,
  T6 = 21,
} ch_idx_t;

static const int NUM_DIFFS = 5;
typedef struct ch_diff
{
  ch_t ch; // channel name
  // store an array of channels used
  // in the diff. The diff is computed by
  // subtracting channels: (i+1) - (i)
  ch_idx_t ch_idx[NUM_DIFFS];
} ch_diff_t;


static const int NUM_CH = 4;
const ch_diff_t DIFFERENCE_PAIRS[NUM_CH] =
{
  // (FP1 - F7)
  // (F7 - T3)
  // (T3 - T5)
  // (T5 - O1)
  {.ch = LL, .ch_idx = {FP1, F7, T3, T5, O1}},

  // (FP2 - F3)
  // (F3 - C3)
  // (C3 - P3)
  // (P3 - O1)
  {.ch = LP, .ch_idx = {FP1, F3, C3, P3, O1}},

  // (FP2 - F4)
  // (F4 - C4)
  // (C4 - P4)
  // (P4 - O2)
  {.ch = RP, .ch_idx = {FP2, F4, C4, P4, O2}},

  // (FP2 - F8)
  // (F8 - T4)
  // (T4 - T6)
  // (T6 - O2)
  {.ch = RL, .ch_idx = {FP2, F8, T4, T6, O2}},
};

void print_hdr_cache();
edf_hdr_struct* get_hdr_cache(const char *filename);
void set_hdr_cache(edf_hdr_struct* hdr);
void pop_hdr_cache(const char* filename);
void log_time_diff(unsigned long long ticks);
double ticks_to_seconds(unsigned long long ticks);
unsigned long long getticks();
void print_spec_params_t(spec_params_t* spec_params);
int get_data_len(edf_hdr_struct* hdr);
int get_nfft(int shift, int pad);
int get_nsamples(int data_len, int fs, float duration);
int get_nblocks(int data_len, int fs, int shift);
int get_nfreqs(int nfft);
int get_next_pow_2(unsigned int v);
int get_fs(edf_hdr_struct* hdr);
void get_eeg_spectrogram_params(spec_params_t* spec_params,
                                char* filename, float duration);
void load_edf(edf_hdr_struct* hdr, char* filename);
void close_edf(char* filename);
void cleanup_spectrogram(char* filename, fmat spec_mat);
float* create_buffer(int n);
int read_samples(int handle, int edfsignal, int n, float *buf);
void hamming(int windowLength, float* buf);
void STFT(frowvec& diff, spec_params_t* spec_params, fmat& specs);
void eeg_file_spectrogram_handler(char* filename, float duration, int ch, fmat& spec_mat);
void eeg_spectrogram_as_arr(spec_params_t* spec_params, int ch, float* spec_arr);
void eeg_spectrogram(spec_params_t* spec_params, int ch, fmat& spec_mat);
void serialize_spec_mat(spec_params_t* spec_params, fmat& spec_mat, float* spec_arr);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // SPECTROGRAM_H
