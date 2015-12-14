#include <string>
#include <fstream>
#include "backends.hpp"
#include "../compute/eeg_spectrogram.hpp"
#include "../helpers.hpp"

using namespace std;

void viz_to_file(string mrn, size_t desired_size, string type)
{
  StorageBackend backend;

  for (int ch = 0; ch < NUM_DIFF; ch++)
  {
    string ch_name = CH_NAME_MAP[ch];
    string cached_mrn = backend.mrn_to_cached_mrn_name(mrn, ch_name);
    string path = mrn_to_filename(cached_mrn, type);

    // don't convert if the file already exists.
    if (file_exists(path)) {
      cout << "File : " << path << " already exists." << endl;
      // return;
    }

    if (!backend.array_exists(cached_mrn)) // we need to precompute it
    {
      if (!backend.array_exists(mrn)) // we need to convert it
      {
        cout << "Converting edf." << endl;
        desired_size = gigabytes_to_bytes(desired_size);
        edf_to_array(mrn, &backend, desired_size);
      }
      cout << "Precomputing spectrogram." << endl;
      precompute_spectrogram(mrn, &backend);
    }

    cout << "Converting channel " << ch_name << " to " << type << "." << endl;

    ofstream file;
    if (type == "bin") {
      file.open(path, ios::trunc|ios::binary);
    } else if (type == "csv") {
      file.open(path, ios::trunc);
    } else {
      cout << "Incorrect type: " << type << endl;
      return;
    }

    backend.open_array(cached_mrn);

    int nsamples = backend.get_nsamples(cached_mrn);
    int ncols = backend.get_ncols(cached_mrn);

    int start_offset, end_offset;
    cell_t cell;
    start_offset = 0;
    end_offset = min(nsamples, READ_CHUNK_SIZE);
    fmat chunk_mat = fmat(ncols, end_offset);

    // read chunks from each signal and write them
    for (; end_offset <= nsamples; end_offset = min(end_offset + READ_CHUNK_SIZE, nsamples))
    {
      if (end_offset - start_offset != READ_CHUNK_SIZE) {
        chunk_mat.resize(ncols, end_offset - start_offset);
      }
      backend.read_array(cached_mrn, start_offset, end_offset, chunk_mat);

      // write chunk_mat to file
      for (uword i = 0; i < chunk_mat.n_rows; i++)
      {
        for (uword j = 0; j < chunk_mat.n_cols; j++)
        {
          // x coord, y coord, attribute
          cell.x = start_offset + i;
          cell.y = j;
          cell.sample = chunk_mat(i, j);
          type == "bin" ? file.write((char*) &cell, CELL_SIZE) : file << cell.x << "," << cell.y << "," << cell.sample << endl;
        }
      }

      start_offset = end_offset;
      // ensure we write the last part of the samples
      if (end_offset == nsamples)
      {
        break;
      }
    }

    file.close();
  }
  cout << type << " conversion complete." << endl;
}

/*
 * Command line program to convert an
 * EDF file for the given `mrn` to the specified backend
 */
int main(int argc, char* argv[])
{
  if (argc <= 4)
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

    size_t desired_size;
    if (argc >= 3)
    {
      desired_size = atoi(argv[2]);
    }
    else
    {
      desired_size = 0;
    }

    string type;
    if (argc >= 4)
    {
      type = argv[3];
      type = type == "bin" || type == "csv" ? type : "csv";
    }
    else
    {
      type = "csv";
    }

    cout << "Using mrn: " << mrn << " backend: " << TOSTRING(BACKEND) << " with type: " << type << " and desired_size: " << desired_size << "GB" << endl;
    viz_to_file(mrn, desired_size, type);
  }
  else
  {
    cout << "\nusage: ./viz_to_file <mrn> <desired_size> <type>\n" << endl;
  }
  return 1;
}

