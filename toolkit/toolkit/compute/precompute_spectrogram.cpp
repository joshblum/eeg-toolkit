#include <string>
#include <armadillo>
#include <math.h>

#include "../storage/backends.hpp"
#include "helpers.hpp"
#include "eeg_spectrogram.hpp"

using namespace std;
using namespace arma;

/*
 * Compute and store the spectrogram data for
 * the given `mrn` for all available time
 */
void precompute_spectrogram(string mrn)
{
  StorageBackend backend;
  backend.open_array(mrn);

  int fs = backend.get_fs(mrn);
  int nsamples = backend.get_array_len(mrn);

  int chunk_size = 100000;
  int nchunks = ceil(nsamples / (float) chunk_size);
  cout << "Computing " << nchunks << " chunks and " << nsamples << " samples." << endl;


  float start_time, end_time;
  int start_offset, end_offset;
  fmat spec_mat;

  // Create array for writing
  start_time = 0;
  end_time = samples_to_hours(fs, nsamples);

  string cached_mrn_name = backend.mrn_to_cached_mrn_name(mrn);
  SpecParams spec_params = SpecParams(&backend, mrn, start_time, end_time);
  backend.create_array(cached_mrn_name, spec_params.nblocks, spec_params.nfreqs);
  backend.open_array(cached_mrn_name);
  for (int ch = 0; ch < NUM_DIFF; ch++)
  {
    start_offset = 0;
    end_offset = min(nsamples, chunk_size);

    for (; end_offset <= nsamples; end_offset = min(end_offset + chunk_size, nsamples))
    {
      start_time = samples_to_hours(fs, start_offset);
      end_time = samples_to_hours(fs, end_offset);
      spec_params = SpecParams(&backend, mrn, start_time, end_time);
      eeg_spectrogram(&spec_params, ch, spec_mat);

      // TODO(joshblum): write output to array store

      start_offset = end_offset;
      // ensure we write the last part of the samples
      if (end_offset == nsamples)
      {
        break;
      }

      if (!((end_offset / chunk_size) % 50))
      {
        cout << "Wrote " << end_offset / chunk_size << " chunks for ch: " << CH_NAME_MAP[ch] << endl;
      }
    }
  }
  backend.close_array(mrn);
}

int main(int argc, char* argv[])
{
  if (argc <= 4)
  {
    string mrn;
    if (argc == 2)
    {
      mrn = argv[1];
    }
    else
    {
      // default medial record number
      mrn = "007";
    }
    cout << "Using mrn: " << mrn << " and backend: " << TOSTRING(BACKEND) << endl;
    precompute_spectrogram(mrn);
  }
  else
  {
    cout << "\nusage: ./precompute_spectrogram <mrn>\n" << endl;
  }
  return 1;
}
