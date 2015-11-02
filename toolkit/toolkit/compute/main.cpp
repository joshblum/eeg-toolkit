#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "../storage/backends.hpp"
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
  if (argc <= 4)
  {
    float start_time, end_time;
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
    if (argc == 4)
    {
      start_time = atof(argv[2]);
      end_time = atof(argv[3]);
    }
    else
    {
      // default times
      start_time = 0.0;
      end_time = 4.0;
    }
    printf("Using mrn: %s, start_time: %.2f, end_time %.2f\n", mrn, start_time, end_time);
    spec_params_t spec_params;
    StorageBackend backend;
    get_eeg_spectrogram_params(&spec_params, &backend, mrn, start_time, end_time);
    fmat spec_mat = fmat(spec_params.nfreqs, spec_params.nblocks);
    example_spectrogram(spec_mat, &spec_params);
    backend.close_array(mrn);

    example_change_points(spec_mat);

    float* spec_arr = (float*) malloc(sizeof(float) * spec_params.nblocks * spec_params.nfreqs);
    // reopen file
    get_eeg_spectrogram_params(&spec_params, &backend, mrn, start_time, end_time);
    example_spectrogram_as_arr(spec_arr, &spec_params);
    backend.close_array(mrn);
    free(spec_arr);

  }
  else
  {
    printf("\nusage: spectrogram <mrn> <start_time> <end_time>\n\n");
  }
  return 1;
}

