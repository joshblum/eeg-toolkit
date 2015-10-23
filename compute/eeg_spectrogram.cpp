#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fftw3.h>

#include "edf_backend.hpp"
#include "helpers.hpp"
#include "eeg_spectrogram.hpp"


using namespace arma;

void print_spec_params_t(spec_params_t* spec_params)
{
  printf("spec_params: {\n");
  printf("\tmrn: %s\n", spec_params->mrn);
  printf("\tstartTime: %.2f\n", spec_params->startTime);
  printf("\tendTime: %.2f\n", spec_params->endTime);
  printf("\thdl: %d\n", spec_params->hdl);
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

float get_valid_startTime(float startTime)
{
  return fmax(0, startTime);
}

float get_valid_endTime(int spec_len, int fs, float endTime)
{
  return fmin((float) spec_len / (fs * 60 * 60), endTime);
}

int get_nsamples(int spec_len, int fs, float duration)
{
  // get number of samples required for our duration
  return fmin(spec_len, fs * 60 * 60 * duration);
}

int get_nblocks(int nsamples, int nfft, int shift)
{
  return (nsamples - nfft) / shift + 1;
}

int get_nfreqs(int nfft)
{
  return nfft / 2 + 1;
}

int get_fs(edf_hdr_struct* hdr)
{
  return ((double)hdr->signalparam[0].smp_in_datarecord / (double)hdr->datarecord_duration) * EDFLIB_TIME_DIMENSION;
}

void get_eeg_spectrogram_params(spec_params_t* spec_params,
    char* mrn, float startTime, float endTime)
{
  // TODO(joshblum): implement full multitaper method
  // and remove hard coding
  spec_params->mrn = mrn;
  spec_params->startTime = startTime;
  spec_params->endTime = endTime;

  edf_hdr_struct* hdr = (edf_hdr_struct*) malloc(sizeof(edf_hdr_struct));
  load_edf(hdr, mrn);
  spec_params->hdl = hdr->handle;

  // check for errors
  if (hdr->filetype < 0)
  {
    spec_params->hdl = -1;
    spec_params->fs = 0;
    spec_params->shift = 0;
    spec_params->nstep = 0;
    spec_params->nfft = 0;
    spec_params->nsamples = 0;
    spec_params->nblocks = 0;
    spec_params->nfreqs = 0;
    spec_params->spec_len = 0;
  } else {
    spec_params->fs = get_fs(hdr);

    int data_len = get_data_len(hdr);
    int pad = 0;
    spec_params->shift = spec_params->fs * 4;
    spec_params->nstep = spec_params->fs * 1;
    spec_params->nfft = get_nfft(spec_params->shift, pad);
    spec_params->nsamples = get_nsamples(data_len, spec_params->fs, endTime - startTime);
    spec_params->startTime = get_valid_startTime(startTime);
    spec_params->endTime = get_valid_endTime(data_len, spec_params->fs, endTime);

    // ensure startTime is before endTime
    if (spec_params->startTime > spec_params->endTime)
    {
      float tmp = spec_params->startTime;
      spec_params->startTime = spec_params->endTime;
      spec_params->endTime = tmp;
    }
    spec_params->nblocks = get_nblocks(spec_params->nsamples,
        spec_params->shift, spec_params->nstep);
    spec_params->nfreqs = get_nfreqs(spec_params->nfft);
    spec_params->spec_len = spec_params->nsamples / spec_params->fs;

  }
}

float* create_buffer(int n)
{
  float* buf = (float*) malloc(sizeof(float) * n);
  if (buf == NULL)
  {
    printf("\nmalloc error\n");
    exit(1);
  }
  return buf;
}

void get_array_data(spec_params_t* spec_params, int ch, int n, float *buf)
{
  //TODO(joshblum): support different types of backends
  //global config? variable passed in?
  read_edf_data(spec_params->hdl, ch, n, buf);
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
void STFT(frowvec& diff, spec_params_t* spec_params, fmat& spec_mat)
{
  fftw_complex    *data, *fft_result;
  fftw_plan       plan_forward;
  int nfft = spec_params->nfft;

  int nstep = spec_params->nstep;

  int nblocks = spec_params->nblocks;
  int nfreqs = spec_params->nfreqs;
  int nsamples = spec_params->nsamples;

  data = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * nfft);
  fft_result = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * nfft);
  // TODO keep plans in memory until end, create plans once and cache?
  // TODO look into using arma memptr instead of copying data
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
    fftw_execute( plan_forward );

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

void eeg_file_wrapper(char* mrn, float startTime, float endTime, int ch, fmat& spec_mat)
{
  spec_params_t spec_params;
  get_eeg_spectrogram_params(&spec_params, mrn, startTime, endTime);
  print_spec_params_t(&spec_params);
  eeg_spectrogram(&spec_params, ch, spec_mat);
}

void eeg_spectrogram_as_arr(spec_params_t* spec_params, int ch, float* spec_arr)
{
  if (spec_arr == NULL)
  {
    spec_arr = (float*) malloc(sizeof(float) * spec_params->nblocks * spec_params->nfreqs);

  }
  fmat spec_mat;
  eeg_spectrogram(spec_params, ch, spec_mat);
  serialize_spec_mat(spec_params, spec_mat, spec_arr);
}

void eeg_spectrogram(spec_params_t* spec_params, int ch, fmat& spec_mat)
{
  if (spec_params->hdl == -1)
  {
    cout << "invalid handle" << endl;
    return;
  }
  spec_mat.set_size(spec_params->nblocks, spec_params->nfreqs);
  // TODO reuse buffers
  // TODO chunking?
  // write edf method to do diff on the fly?
  int nsamples = spec_params->nsamples;
  float* buf1 = create_buffer(nsamples);
  float* buf2 = create_buffer(nsamples);

  // nfreqs x nblocks matrix
  spec_mat.fill(0);

  int ch_idx1, ch_idx2;
  ch_idx1 = DIFFERENCE_PAIRS[ch].ch_idx[0];
  get_array_data(spec_params, ch_idx1, nsamples, buf1);

  for (int i = 1; i < NUM_DIFFS; i++)
  {
    ch_idx2 = DIFFERENCE_PAIRS[ch].ch_idx[i];
    get_array_data(spec_params, ch_idx2, nsamples, buf2);

    // TODO use rowvec::fixed with fixed size chunks
    frowvec v1 = frowvec(buf1, nsamples);
    frowvec v2 = frowvec(buf2, nsamples);
    frowvec diff = v2 - v1;

    // fill in the spec matrix with fft values
    STFT(diff, spec_params, spec_mat);
    std::swap(buf1, buf2);
  }
  // TODO serialize spec_mat output for each channel
  spec_mat /=  (NUM_DIFFS - 1); // average diff spectrograms
  spec_mat = spec_mat.t(); // transpose the output

  free(buf1);
  free(buf2);
}
/*
 * Transform the mat to a float* for transfer
 * via websockets
 */
void serialize_spec_mat(spec_params_t* spec_params, fmat& spec_mat, float* spec_arr)
{
  if (spec_params->hdl == -1)
  {
    return;
  }
  for (int i = 0; i < spec_params->nfreqs; i++)
  {
    for (int j = 0; j < spec_params->nblocks; j++)
    {
      *(spec_arr + i + j * spec_params->nfreqs) = (float) spec_mat(i, j);
    }
  }
}
