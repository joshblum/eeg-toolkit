#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fftw3.h>

#include "../storage/backends.hpp"
#include "helpers.hpp"
#include "eeg_spectrogram.hpp"


using namespace arma;
using namespace std;

void print_spec_params_t(spec_params_t* spec_params)
{
  printf("spec_params: {\n");
  printf("\tmrn: %s\n", spec_params->mrn.c_str());
  printf("\tstart_time: %.2f\n", spec_params->start_time);
  printf("\tend_time: %.2f\n", spec_params->end_time);
  printf("\tnfft: %d\n", spec_params->nfft);
  printf("\tnstep: %d\n", spec_params->nstep);
  printf("\tshift: %d\n", spec_params->shift);
  printf("\tnsamples: %d\n", spec_params->nsamples);
  printf("\tnblocks: %d\n", spec_params->nblocks);
  printf("\tnfreqs: %d\n", spec_params->nfreqs);
  printf("\tspec_len: %d\n", spec_params->spec_len);
  printf("\tfs: %d\n", spec_params->fs);
  printf("}\n");
}

int get_nfft(int shift, int pad)
{
  return fmax(get_next_pow_2(shift) + pad, shift);
}

float get_valid_start_time(float start_time)
{
  return fmax(0, start_time);
}

float get_valid_end_time(int spec_len, int fs, float end_time)
{
  return fmin((float) spec_len / (fs * 60 * 60), end_time);
}

int get_nsamples(int spec_len, int fs, float duration)
{
  // get number of samples required for our duration
  return fmin(spec_len, hours_to_nsamples(fs, duration));
}

int get_nblocks(int nsamples, int nfft, int shift)
{
  return (nsamples - nfft) / shift + 1;
}

int get_nfreqs(int nfft)
{
  return nfft / 2 + 1;
}

void get_eeg_spectrogram_params(spec_params_t* spec_params, StorageBackend* backend,
                                  string mrn, float start_time, float end_time)
{
  // TODO(joshblum): implement full multitaper method
  // and remove hard coding
  spec_params->mrn = mrn;
  spec_params->start_time = start_time;
  spec_params->end_time = end_time;
  spec_params->backend = backend;

  backend->load_array(mrn);

  spec_params->fs = backend->get_fs(mrn);
  int data_len = backend->get_data_len(mrn);
  int pad = 0;
  spec_params->shift = spec_params->fs * 4;
  spec_params->nstep = spec_params->fs * 1;
  spec_params->nfft = get_nfft(spec_params->shift, pad);
  spec_params->nsamples = get_nsamples(data_len, spec_params->fs, end_time - start_time);
  spec_params->start_time = get_valid_start_time(start_time);
  spec_params->end_time = get_valid_end_time(data_len, spec_params->fs, end_time);

  // ensure start_time is before end_time
  if (spec_params->start_time > spec_params->end_time)
  {
    float tmp = spec_params->start_time;
    spec_params->start_time = spec_params->end_time;
    spec_params->end_time = tmp;
  }
  spec_params->nblocks = get_nblocks(spec_params->nsamples,
                                     spec_params->shift, spec_params->nstep);
  spec_params->nfreqs = get_nfreqs(spec_params->nfft);
  spec_params->spec_len = spec_params->nsamples / spec_params->fs;
}


// Create a hamming window of windowLength samples in buffer
void hamming(int windowLength, float* buf)
{
  for (int i = 0; i < windowLength; i++)
  {
    buf[i] = 0.54 - (0.46 * cos( 2 * M_PI * (i / ((windowLength - 1) * 1.0))));
  }
}

static inline float abs(fftw_complex* arr, int i)
{
  return sqrt(arr[i][0] * arr[i][0] + arr[i][1] * arr[i][1]);
}

// copy pasta http://ofdsp.blogspot.co.il/2011/08/short-time-fourier-transform-with-fftw3.html
/*
 * Fill the `spec_mat` matrix with values for the spectrogram for the given diff.
 * `spec_mat` is expected to be initialized and the results are added to allow averaging
 */
void STFT(spec_params_t* spec_params, frowvec& diff, fmat& spec_mat)
{
  fftw_complex    *data, *fft_result;
  fftw_plan       plan_forward;
  int nfft = spec_params->nfft;

  int nstep = spec_params->nstep;

  int nblocks = spec_params->nblocks;
  int nfreqs = spec_params->nfreqs;
  int nsamples = spec_params->nsamples;

  data = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * nfft);
  fft_result = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * nfft);
  // TODO keep plans in memory until end, create plans once and cache?
  plan_forward = fftw_plan_dft_1d(nfft, data, fft_result,
                                  FFTW_FORWARD, FFTW_ESTIMATE);

  // Create a hamming window of appropriate length
  float window[nfft];
  hamming(nfft, window);

  for (int idx = 0; idx < nblocks; idx++)
  {
    // get the last chunk
    if (idx * nstep + nfft > nsamples)
    {
      int upper_bound = nsamples - idx * nstep;
      for (int i = 0; i < upper_bound; i++)
      {
        data[i][0] = diff(idx * nstep + i) * window[i];
        data[i][1] = 0.0;

      }
      for (int i = upper_bound; i < nfft; i++)
      {
        data[i][0] = 0.0;
        data[i][1] = 0.0;
      }
      break;
    }
    else
    {
      for (int i = 0; i < nfft; i++)
      {
        // TODO vector multiplication?
        data[i][0] = diff(idx * nstep  + i) * window[i];
        data[i][1] = 0.0;
      }
    }

    // Perform the FFT on our chunk
    fftw_execute(plan_forward);

    // TODO: change maybe?
    // http://www.fftw.org/fftw2_doc/fftw_2.html
    for (int i = 0; i < nfreqs; i++)
    {
      spec_mat(idx, i) += abs(fft_result, i) / nfft;
    }

    // Uncomment to see the raw-data output from the FFT calculation
    // printf("[ ");
    // for (int i = 0 ; i < 10 ; i++ )
    // {
    //   printf("%2.2f ", specs(i, idx));
    // }
    // printf("]\n");
  }

  fftw_destroy_plan(plan_forward);

  fftw_free(data);
  fftw_free(fft_result);
}

void eeg_spectrogram(spec_params_t* spec_params, int ch, fmat& spec_mat)
{

  // nfreqs x nblocks matrix
  spec_mat.set_size(spec_params->nblocks, spec_params->nfreqs);
  spec_mat.fill(0);

  // write edf method to do diff on the fly?
  int nsamples = spec_params->nsamples;

  int ch_idx1, ch_idx2;
  ch_idx1 = DIFFERENCE_PAIRS[ch].ch_idx[0];

  // should this just move to the spec_params struct?
  int startOffset = hours_to_nsamples(spec_params->fs, spec_params->start_time);
  int endOffset = hours_to_nsamples(spec_params->fs, spec_params->end_time);

  frowvec vec1 = frowvec(nsamples);
  frowvec vec2 = frowvec(nsamples);
  spec_params->backend->get_array_data(spec_params->mrn, ch_idx1, startOffset, endOffset, vec1);

  for (int i = 1; i < NUM_DIFFS; i++)
  {
    ch_idx2 = DIFFERENCE_PAIRS[ch].ch_idx[i];
    spec_params->backend->get_array_data(spec_params->mrn, ch_idx2, startOffset, endOffset, vec2);
    frowvec diff = vec2 - vec1;

    // fill in the spec matrix with fft values
    STFT(spec_params, diff, spec_mat);
    swap(vec1, vec2);
  }
  spec_mat /=  (NUM_DIFFS - 1); // average diff spectrograms
  spec_mat = spec_mat.t(); // transpose the output
}

