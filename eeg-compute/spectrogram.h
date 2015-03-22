#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H


#ifdef __cplusplus
extern "C" {
#endif

typedef struct spec_params {
  int spec_len; // length of the spectrogram
  int fs; // sample rate
  int nfft; // number of samples for fft
  int Nstep; // number of steps
  int shift; // shift size for windows
  int nsamples; // number of samples in the spectrogram
  int nblocks; // number of blocks
  int nfreqs; // number of frequencies
} spec_params_t;

void print_spec_params_t(spec_params_t* spec_params);
int get_nfreqs(int nfft);
int get_nblocks(int data_len, int fs, int shift);
int get_nsamples(int data_len, int fs, float duration);
int get_next_pow_2(unsigned int v);
int get_fs(edf_hdr_struct* hdr);
void get_eeg_spectrogram_params(spec_params_t* spec_params,
    edf_hdr_struct* hdr, float duration);
void load_edf(edf_hdr_struct* hdr, char* filename);
int eeg_file_spectrogram(char* filename, float duration);
int main(int argc, char* argv[]);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // SPECTROGRAM_H
