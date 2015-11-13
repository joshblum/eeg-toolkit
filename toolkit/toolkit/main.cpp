#include <string>
#include <armadillo>

#include "storage/backends.hpp"
#include "compute/helpers.hpp"
#include "compute/eeg_spectrogram.hpp"
#include "compute/eeg_change_point.hpp"

using namespace std;
using namespace arma;

#define NSAMPLES 10

void example_spectrogram(fmat& spec_mat, SpecParams* spec_params)
{
  unsigned long long start = getticks();
  spec_params->print();
  eeg_spectrogram(spec_params, LL, spec_mat);
  log_time_diff("example_spectrogram:", start);
  printf("Spectrogram shape as_mat: (%d, %d)\n",
         spec_params->nblocks, spec_params->nfreqs);
  printf("Sample data: [\n[ ");
  for (int i = 0; i < NSAMPLES; i++)
  {
    for (int j = 0; j < NSAMPLES; j++)
    {
      printf("%.5f, ", spec_mat(i, j));
    }
    printf("],\n[ ");
  }
  printf("]\n");
}

void compute_example(string mrn, float start_time, float end_time)
{
  StorageBackend backend;
  SpecParams spec_params = SpecParams(&backend, mrn, start_time, end_time);
  fmat spec_mat = fmat(spec_params.nfreqs, spec_params.nblocks);
  example_spectrogram(spec_mat, &spec_params);
  backend.close_array(mrn);

  // example_change_points(spec_mat);
}

void storage_example(string mrn)
{
  StorageBackend backend;
  backend.edf_to_array(mrn);
  backend.open_array(mrn);
  cout << "fs: " << backend.get_fs(mrn) << " data_len: " << backend.get_array_len(mrn) << endl;
  frowvec buf = frowvec(NSAMPLES);
  backend.read_array(mrn, C3, 0, NSAMPLES - 1, buf);
  for (int i = 0; i < NSAMPLES; i++)
  {
    cout << " " << buf(i);
  }
  cout << endl;
}

int main(int argc, char* argv[])
{
  if (argc <= 4)
  {
    float start_time, end_time;
    string mrn;
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
    printf("Using mrn: %s, start_time: %.2f, end_time %.2f and backend: %s\n",
       mrn.c_str(), start_time, end_time, TOSTRING(BACKEND));
    compute_example(mrn, start_time, end_time);
    // storage_example(mrn);
  }
  else
  {
    cout << "\nusage: ./main <mrn> <start_time> <end_time>\n" << endl;
  }
  return 1;
}

