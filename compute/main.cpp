#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "helpers.hpp"
#include "eeg_spectrogram.hpp"
#include "eeg_change_point.hpp"

#define NUM_SAMPLES 10

using namespace arma;



void example_spectrogram(fmat& spec_mat, spec_params_t* spec_params)
{
  unsigned long long start = getticks();
  print_spec_params_t(spec_params);
  eeg_spectrogram(spec_params, LL, spec_mat);
  log_time_diff("example_spectrogram:", start);
  printf("Spectrogram shape as_mat: (%d, %d)\n",
         spec_params->nblocks, spec_params->nfreqs);
  printf("Sample data: [\n[ ");
  for (int i = 0; i < NUM_SAMPLES; i++)
  {
    for (int j = 0; j < NUM_SAMPLES; j++)
    {
      printf("%.5f, ", spec_mat(i, j));
    }
    printf("],\n[ ");
  }
  printf("]\n");
}

void example_spectrogram_as_arr(float* spec_arr, spec_params_t* spec_params)
{
  unsigned long long start = getticks();
  print_spec_params_t(spec_params);
  eeg_spectrogram_as_arr(spec_params, LL, spec_arr);
  log_time_diff("example_spectrogram_as_arr:", start);
  printf("Spectrogram shape: (%d, %d)\n",
         spec_params->nblocks, spec_params->nfreqs);

  printf("Sample data as_arr: [\n[ ");
  for (int i = 0; i < NUM_SAMPLES; i++)
  {
    for (int j = 0; j < NUM_SAMPLES; j++)
    {
      printf("%.5f, ", *(spec_arr + i + j * spec_params->nfreqs));
    }
    printf("],\n[ ");
  }
  printf("]]\n");
}

int main(int argc, char *argv[])
{
  if (argc <= 3)
  {
    float duration;
    char* mrn;
    if (argc >= 2)
    {
      mrn = argv[1];
    }
    else
    {
      // default medial record number
      mrn = "007";
    }
    if (argc == 3)
    {
      duration = atof(argv[2]);
    }
    else
    {
      duration = 4.0; // default duration
    }
    printf("Using mrn: %s, duration: %.2f hours\n", mrn, duration);
    spec_params_t spec_params;
    get_eeg_spectrogram_params(&spec_params, mrn, duration);
    if (spec_params.hdl == -1) {
      exit(1);
    }
    fmat spec_mat = fmat(spec_params.nfreqs, spec_params.nblocks);
    example_spectrogram(spec_mat, &spec_params);
    close_edf(mrn);

    example_change_points(spec_mat);

    float* spec_arr = (float*) malloc(sizeof(float) * spec_params.nblocks * spec_params.nfreqs);
    // reopen file
    get_eeg_spectrogram_params(&spec_params, mrn, duration);
    example_spectrogram_as_arr(spec_arr, &spec_params);
    close_edf(mrn);
    free(spec_arr);

  }
  else
  {
    printf("\nusage: spectrogram <mrn> <duration>\n\n");
  }
  return 1;
}
