#include <stdio.h>
#include <stdlib.h>

#include "edflib.h"
#include "eeg_spectrogram.h"

int NUM_SAMPLES = 10;

void example_spectrogram(char* filename, float duration)
{
  printf("Using filename: %s, duration: %.2f hours\n", filename, duration);
  unsigned long long start = getticks();
  spec_params_t spec_params;
  get_eeg_spectrogram_params(&spec_params, filename, duration);
  float* out = (float *) malloc(sizeof(float) * spec_params.nblocks * spec_params.nfreqs);
  print_spec_params_t(&spec_params);
  eeg_spectrogram_handler(&spec_params, LL, out);
  unsigned long long end = getticks();
  log_time_diff(end - start);
  printf("Spectrogram shape: (%d, %d)\n",
         spec_params.nblocks, spec_params.nfreqs);

  printf("Sample data: [[ ");
  for (int i = 0; i < NUM_SAMPLES * NUM_SAMPLES; i++)
  {
    if (i != 0 && !(i % NUM_SAMPLES))
    {
      printf("],\n[ ");
    }
    printf("%.5f, ", *(out + i));
  }
  printf("]]\n");
}

int main(int argc, char *argv[])
{
  if (argc <= 3)
  {
    float duration;
    char* filename;
    if (argc >= 2)
    {
      filename = argv[1];
    }
    else
    {
      // default filename
      filename = "/Users/joshblum/Dropbox (MIT)/MIT-EDFs/MIT-CSAIL-007.edf";
    }
    if (argc == 3)
    {
      duration = atof(argv[2]);
    }
    else
    {
      duration = 4.0; // default duration
    }
    example_spectrogram(filename, duration);
  }
  else
  {
    printf("\nusage: spectrogram <filename> <duration>\n\n");
  }
  return 1;
}
