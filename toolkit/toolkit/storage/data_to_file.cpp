#include <string>
#include <fstream>
#include "backends.hpp"
#include "../helpers.hpp"

using namespace std;

string mrn_to_filename(string mrn, string format)
{
  return DATADIR + mrn + "." + format;
}

void data_to_file(string mrn, string type)
{
  string path = mrn_to_filename(mrn, type);
  // don't convert if the file already exists.
  if (file_exists(path)) {
    cout << "File : " << path << " already exists." << endl;
    return;
  }

  ofstream file;
  if (type == "bin") {
    file.open(path, ios::trunc|ios::binary);
  } else if (type == "csv") {
    file.open(path, ios::trunc);
  } else {
    cout << "Incorrect type: " << type << endl;
    return;
  }

  StorageBackend backend;
  backend.open_array(mrn);

  int nchannels = NCHANNELS;
  int nsamples = backend.get_nsamples(mrn);

  int ch, start_offset, end_offset;
  cell_t cell;
  for (int i = 0; i < nchannels; i++)
  {
    ch = CHANNEL_ARRAY[i];
    start_offset = 0;
    end_offset = min(nsamples, READ_CHUNK_SIZE);
    frowvec chunk_buf = frowvec(end_offset); // store samples from each channel here

    // read chunks from each signal and write them
    for (; end_offset <= nsamples; end_offset = min(end_offset + READ_CHUNK_SIZE, nsamples))
    {
      if (end_offset - start_offset != READ_CHUNK_SIZE) {
        chunk_buf.resize(end_offset - start_offset);
      }
      backend.read_array(mrn, ch, start_offset, end_offset, chunk_buf);

      // write chunk_mat to file
      for (uword i = 0; i < chunk_buf.n_elem; i++)
      {
        // x coord, y coord, attribute
        cell.x = CH_REVERSE_IDX[ch];
        cell.y = start_offset + i;
        cell.sample = chunk_buf(i);
        type == "bin" ? file.write((char*) &cell, CELL_SIZE) : file << cell.x << "," << cell.y << "," << cell.sample << endl;
      }

      start_offset = end_offset;
      // ensure we write the last part of the samples
      if (end_offset == nsamples)
      {
        break;
      }
    }

    if (!(ch % 2))
    {
      cout << "Wrote ch: " << ch << endl;
    }
  }

  file.close();
}

/*
 * Command line program to convert an
 * EDF file for the given `mrn` to the specified backend
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

    string type;
    if (argc == 3)
    {
      type = argv[2];
      type = type == "bin" || type == "csv" ? type : "csv";
    }
    else
    {
      type = "csv";
    }

    cout << "Using mrn: " << mrn << " backend: " << TOSTRING(BACKEND) << " with type: " << type << endl;
    data_to_file(mrn, type);
  }
  else
  {
    cout << "\nusage: ./data_to_file <mrn> <type>\n" << endl;
  }
  return 1;
}

