#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "eeg_spectrogram.hpp"
#include "eeg_change_point.hpp"

#define NUM_SAMPLES 10

using namespace arma;


void print_frowvec(char* name, frowvec vector)
{
  printf("%s: [ ", name);
  for (int i = 0; i < NUM_SAMPLES; i++)
  {
    printf("%.2f, ", vector[i]);
  }
  printf("]\n");
}


void example_spectrogram(fmat& spec_mat, spec_params_t* spec_params)
{
  unsigned long long start = getticks();
  print_spec_params_t(spec_params);
  eeg_spectrogram(spec_params, LL, spec_mat);
  unsigned long long end = getticks();
  log_time_diff(end - start);
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
  unsigned long long end = getticks();
  log_time_diff(end - start);
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
void example_change_points(fmat& spec_mat)
{
  cp_data_t cp_data;
  init_cp_data_t(&cp_data, spec_mat.n_rows);
  get_change_points(spec_mat, &cp_data);
  printf("Total change points found: %d\n", cp_data.total_count);
  print_frowvec("cp", cp_data.cp);
  print_frowvec("yp", cp_data.yp);
  print_frowvec("cu", cp_data.cu);
  print_frowvec("cl", cp_data.cl);
  print_frowvec("mu", cp_data.mu);
  print_frowvec("m", cp_data.m);
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
