#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <fftw3.h>

#include "../storage/backends.hpp"
#include "../helpers.hpp"
#include "eeg_spectrogram.hpp"


using namespace arma;
using namespace std;

void SpecParams::print()
{
  cout << "spec_params: {" << endl;
  cout << "\tmrn: " << mrn << endl;
  cout << "\tstart_time: " << setprecision(2) << start_time << endl;
  cout << "\tend_time: " << setprecision(2) << end_time << endl;
  cout << "\tstart_offset: " << start_offset << endl;
  cout << "\tend_offset: " << end_offset << endl;
  cout << "\tspec_start_offset: " << spec_start_offset << endl;
  cout << "\tspec_end_offset: " << spec_end_offset << endl;
  cout << "\tnfft: " << nfft << endl;
  cout << "\tnstep: " << nstep << endl;
  cout << "\tshift: " << shift << endl;
  cout << "\tnsamples: " << nsamples << endl;
  cout << "\tnblocks: " << nblocks << endl;
  cout << "\tnfreqs: " << nfreqs << endl;
  cout << "\tfs: " << fs << endl;
  cout << "}" << endl;
}

int SpecParams::get_nfft(int pad)
{
  return fmax(get_next_pow_2(shift) + pad, shift);
}

float SpecParams::get_valid_start_time()
{
  return fmax(0, start_time);
}

float SpecParams::get_valid_end_time(int data_len)
{
  return fmin(data_len / (fs * 60.0 * 60.0), end_time);
}

int SpecParams::get_nsamples(int data_len, float duration)
{
  // get number of samples required for our duration
  return fmin(data_len, hours_to_samples(fs, duration));
}

int SpecParams::get_nblocks(int nsamples)
{
  if (nsamples == 0)
  {
    return 0;
  }
  return (nsamples - nfft) / shift + 1;
}

int SpecParams::get_nfreqs()
{
  return nfft / 2 + 1;
}

SpecParams::SpecParams(StorageBackend* backend,
                       string mrn, float _start_time, float _end_time)
{
  // TODO(joshblum): implement full multitaper method
  // and remove hard coding
  this->mrn = mrn;
  this->backend = backend;
  start_time = _start_time;
  end_time = _end_time;
  backend->open_array(mrn);

  fs = backend->get_fs(mrn);


  int data_len = backend->get_array_len(mrn);
  int pad = 0;
  shift = fs * 4;
  nstep = fs * 1;
  nfft = get_nfft(pad);
  start_time = get_valid_start_time();
  end_time = get_valid_end_time(data_len);

  // ensure start_time is before end_time
  if (start_time > end_time)
  {
    float tmp = start_time;
    start_time = end_time;
    end_time = tmp;
  }
  float duration = end_time - start_time;
  nsamples = get_nsamples(data_len, duration);
  nblocks = get_nblocks(nsamples);
  nfreqs = get_nfreqs();

  start_offset = hours_to_samples(fs, start_time);
  end_offset = hours_to_samples(fs, end_time) - 1; // exclusive range
  spec_start_offset = get_nblocks(start_offset);
  spec_end_offset = get_nblocks(end_offset - start_offset + 1);
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
void STFT(SpecParams* spec_params, frowvec& diff, fmat& spec_mat)
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

void eeg_spectrogram(SpecParams* spec_params, int ch, fmat& spec_mat)
{

  // nfreqs x nblocks matrix
  spec_mat.set_size(spec_params->nblocks, spec_params->nfreqs);
  spec_mat.fill(0); // can we eliminate this?

  // write edf method to do diff on the fly?
  int nsamples = spec_params->nsamples;

  int ch_idx1, ch_idx2;
  ch_idx1 = DIFFERENCE_PAIRS[ch].ch_idx[0];
  int start_offset = spec_params->start_offset;
  int end_offset = spec_params->end_offset;

  // should this just move to the spec_params struct?
  frowvec vec1 = frowvec(nsamples);
  frowvec vec2 = frowvec(nsamples);
  spec_params->backend->read_array(spec_params->mrn, ch_idx1, start_offset, end_offset, vec1);

  for (int i = 1; i < NUM_DIFFS; i++)
  {
    ch_idx2 = DIFFERENCE_PAIRS[ch].ch_idx[i];
    spec_params->backend->read_array(spec_params->mrn, ch_idx2, start_offset, end_offset, vec2);
    frowvec diff = vec2 - vec1;

    // fill in the spec matrix with fft values
    STFT(spec_params, diff, spec_mat);
    swap(vec1, vec2);
  }
  spec_mat /=  (NUM_DIFFS - 1); // average diff spectrograms
  spec_mat = spec_mat.t(); // transpose the output
}

