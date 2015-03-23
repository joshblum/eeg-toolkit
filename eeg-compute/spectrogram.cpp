#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fftw3.h>
#include <armadillo>

#include "edflib.h"
#include "spectrogram.h"

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

void print_spec_params_t(spec_params_t* spec_params) {
  printf("spec_params: {\n");
  printf("\tnfft: %d\n", spec_params->nfft);
  printf("\tNstep: %d\n", spec_params->Nstep);
  printf("\tshift: %d\n", spec_params->shift);
  printf("\tnblocks: %d\n", spec_params->nblocks);
  printf("\tnfreqs: %d\n", spec_params->nfreqs);
  printf("\tspec_len: %d\n", spec_params->spec_len);
  printf("\tfs: %d\n", spec_params->fs);
  printf("}\n");
}

int get_nfreqs(int nfft) {
  return nfft / 2 + 1;
}

int get_nblocks(int data_len, int nfft, int shift) {
  return (data_len - nfft) / shift + 1;
}

int get_nsamples(int data_len, int fs, float duration) {
  return min(data_len, fs * 60 * 60 * duration);
}

int get_next_pow_2(unsigned int v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

int get_fs(edf_hdr_struct* hdr) {
  return ((double)hdr->signalparam[0].smp_in_datarecord / (double)hdr->datarecord_duration) * EDFLIB_TIME_DIMENSION;
}

void get_eeg_spectrogram_params(spec_params_t* spec_params,
    edf_hdr_struct* hdr, float duration) {
    // TODO(joshblum): implement full multitaper method
    // and remove hard coding
    spec_params->fs = get_fs(hdr);
    int data_len = hdr->datarecords_in_file;
    int pad = 0;
    int Nwin = spec_params->fs * 1.5;
    spec_params->Nstep = spec_params->fs * 0.2;
    spec_params->nfft = max(get_next_pow_2(Nwin) + pad, Nwin);
    spec_params->shift= spec_params->nfft * 0.5;
    spec_params->nsamples = get_nsamples(data_len, spec_params->fs, duration);
    spec_params->nblocks = get_nblocks(data_len,
        spec_params->nfft, spec_params->shift);
    spec_params->nfreqs = get_nfreqs(spec_params->nfft);
}

void load_edf(edf_hdr_struct* hdr, char* filename) {
  if(edfopen_file_readonly(filename, hdr, EDFLIB_READ_ALL_ANNOTATIONS)) {
    switch(hdr->filetype) {
      case EDFLIB_MALLOC_ERROR                : printf("\nmalloc error\n\n");
                                                break;
      case EDFLIB_NO_SUCH_FILE_OR_DIRECTORY   : printf("\ncan not open file, no such file or directory\n\n");
                                                break;
      case EDFLIB_FILE_CONTAINS_FORMAT_ERRORS : printf("\nthe file is not EDF(+) or BDF(+) compliant\n"
                                                       "(it contains format errors)\n\n");
                                                break;
      case EDFLIB_MAXFILES_REACHED            : printf("\nto many files opened\n\n");
                                                break;
      case EDFLIB_FILE_READ_ERROR             : printf("\na read error occurred\n\n");
                                                break;
      case EDFLIB_FILE_ALREADY_OPENED         : printf("\nfile has already been opened\n\n");
                                                break;
      default                                 : printf("\nunknown error\n\n");
                                                break;
    }
    exit(1);
  }
}

double* create_buffer(int n) {
    double* buf = (double * ) malloc(sizeof(double[n]));
    if (buf == NULL) {
        printf("\nmalloc error\n");
        edfclose_file(hdl);
        exit(1);
    }
    return buf;
}

int read_samples(int handle, int edfsignal, int n, double *buf) {
  n = edfread_physical_samples(hdl, ch2, n, buf);

  if (n == -1) {
      printf("\nerror: edf_read_physical_samples()\n");
      edfclose_file(hdl);
      free(buf);
      exit(1);
  }
  return n;
}

// Create a hamming window of windowLength samples in buffer
void hamming(int windowLength, double* buffer) {
 for(int i = 0; i < windowLength; i++) {
   buffer[i] = 0.54 - (0.46 * cos( 2 * PI * (i / ((windowLength - 1) * 1.0))));
 }
}

// copy pasta http://ofdsp.blogspot.co.il/2011/08/short-time-fourier-transform-with-fftw3.html
void STFT(rowvec diff, spec_params_t spec_params, mat specs) {

    fftw_real    *data, *fft_result;
    fftw_plan       plan_forward;
    int nfft = spec_params.nfft
    int shift = spec_params.shift
    int nblocks = spec_params.nblocks
    int nfreqs = spec_params.nfreqs
    int nsamples = spec_params.nsamples;

    data = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * nfft);
    fft_result = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * nfft);
    // TODO keep plans in memory until end
    plan_forward = fftw_plan_r2r_1d(nfft, data, fft_result,
        FFTW_FORWARD, FFTW_ESTIMATE);

    // Create a hamming window of appropriate length
    float window[nfft];
    hamming(nfft, window);

    for (int idx = 0; idx < nblocks; idx++) {
      // Copy the chunk into our buffer
      if (idx*shift + nfft > nsamples) {
        // get the last chunk
        int upper_bound = (nfft + idx*shift) - nsamples;
        for (int i = 0; i < upper_bound; i++) {
          data[i] = (*diff)[idx*shift + i] * window[i];
        }
        for(int i = upper_bound; i < nfft; i++) {
          data[i] = 0;
        }
      } else {
        for(int i = 0; i < nfft; i++) {
          // TODO vector multiplication?
          data[i] = (*diff)[idx*shift + i] * window[i];
        }
      }

      // Perform the FFT on our chunk
      fftw_execute( plan_forward );

      /* Uncomment to see the raw-data output from the FFT calculation
      std::cout << "Column: " << chunkPosition << std::endl;
      for(i = 0 ; i < nfft ; i++ ) {
       fprintf( stdout, "fft_result[%d] = { %2.2f, %2.2f }\n",
         i, fft_result[i][0], fft_result[i][1] );
      }
       */

      // Copy the first (nfft/2 + 1) data points into your spectrogram.
      // We do this because the FFT output is mirrored about the nyquist
      // frequency, so the second half of the data is redundant. This is how
      // Matlab's spectrogram routine works.
      // TODO: change maybe?
      // http://www.fftw.org/fftw2_doc/fftw_2.html
        for (i = 0; i < nfft/2 + 1; i++) {
            specs(i, idx) = fft_result[i]*fft_result[i];
        }
  }

  fftw_destroy_plan( plan_forward );

  fftw_free( data );
  fftw_free( fft_result );
}


int eeg_file_spectrogram(char * filename, float duration) {
    edf_hdr_struct hdr;
    spec_params_t spec_params;
    load_edf( & hdr, filename);
    get_eeg_spectrogram_params( & spec_params, & hdr, duration);
    print_spec_params_t( & spec_params);

    // TODO reuse buffers
    // TODO chunking?
    // write edf method to do diff on the fly?
    int hdl = hdr.handle;
    int n = spec_params.nsamples;
    buf1 = create_buffer(n);
    buf2 = create_buffer(n);

    int ch1, ch2, n1, n2;
    for (int i = 0; i < NUM_PAIRS; i++) {
      for (int j = 0; j < NUM_DIFFS - 1; j++) {
        ch1 = DIFFERENCE_PAIRS[i].ch_idx[j];
        ch2 = DIFFERENCE_PAIRS[i].ch_idx[j+1];
        n1 = read_samples(hdl, ch1, n, buf1);
        n2 = read_samples(hdl, ch2, n, buf2);
        rowrev v1 = rowvec::fixed<n>(const *buf1);
        rowvec v2 = rowvec::fixed<n>(const *buf2);
        rowvec diff = v2 - v1;
        // nfreqs x nblocks matrix
        mat specs = mat(spec_params.nfreqs, spec_params.nblocks, fill::zeros);
        // fill in the spec matrix with fft values
        STFT(diff, spec_params, spec);
      }
    }

  return 1; // success
}

