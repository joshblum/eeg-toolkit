#include <stdio.h>
#include <stdlib.h>

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

int eeg_file_spectrogram(char* filename, float duration) {
  edf_hdr_struct hdr;
  spec_params_t spec_params;
  load_edf(&hdr, filename);
  get_eeg_spectrogram_params(&spec_params, &hdr, duration);
  print_spec_params_t(&spec_params);
  return 1; // success
}

