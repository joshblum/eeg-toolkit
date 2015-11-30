#include <string>
#include <fstream>
#include "backends.hpp"
#include "../compute/eeg_spectrogram.hpp"
#include "../helpers.hpp"

using namespace std;

void viz_to_csv(string mrn)
{
  StorageBackend backend;
  cout << "Converting mrn: " << mrn << " with backend: " << TOSTRING(BACKEND) << endl;
  if (!backend.array_exists(mrn))
  {
    cout << "Converting to array: " << mrn << endl;
    edf_to_array(&backend, mrn);
  }

  fmat mat;
  ofstream file;
  for (int ch = 0; ch < NUM_DIFF; ch++)
  {
    string ch_name = CH_NAME_MAP[ch];
    string cached_mrn_name = backend.mrn_to_cached_mrn_name(mrn, ch_name);
    string path = cached_mrn_name + ".csv";

    if (file_exists(path)) {
      cout << "File : " << path << " already exists." << endl;
      continue;
    }
    if (!backend.array_exists(cached_mrn_name))
    {
      cout << "Precomputing spectrogram: " << cached_mrn_name << endl;
      precompute_spectrogram(mrn, &backend);
    }
    cout << "Converting: " << cached_mrn_name << endl;

    backend.open_array(cached_mrn_name);
    mat = fmat(backend.get_ncols(cached_mrn_name), backend.get_nrows(cached_mrn_name));
    backend.read_array(cached_mrn_name, 0, backend.get_nrows(cached_mrn_name) - 1, mat);
    file.open(path, ios::trunc);
    mat = mat.t();
    for (uword i = 0; i < mat.n_rows; i++)
    {
      for (uword j = 0; j < mat.n_cols; j++)
      {
        file << i << "," << j << "," << mat(i,j) << endl;
      }
    }
    backend.close_array(cached_mrn_name);
    file.close();
  }
}

/*
 * Command line program to convert cached spectrogram output to a CSV format
 */
int main(int argc, char* argv[])
{
  if (argc <= 3)
  {
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

    cout << "Using mrn: " << mrn << endl;
    viz_to_csv(mrn);
  }
  else
  {
    cout << "\nusage: ./viz_converter <mrn>\n" << endl;
  }
  return 1;
}

