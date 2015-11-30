#include "backends.hpp"

#include<string>
#include<armadillo>

#include "../helpers.hpp"

using namespace std;
using namespace arma;

/*
 * Convert an EDF file to array format.  If `desired_size > 0`, the output will
 * have extra data to match the `desired_size`.  Otherwise, the file will be
 * converted directly with no additional data.
 */
void edf_to_array(string mrn, StorageBackend* backend, size_t desired_size)
{
  // Capture start time
  unsigned long long start = getticks();

  EDFBackend edf_backend;
  edf_backend.open_array(mrn);

  int fs = edf_backend.get_fs(mrn);
  int nsamples = edf_backend.get_nsamples(mrn);
  int ncols = NCHANNELS;
  int nrows;

  if (desired_size != 0)
  {
    nrows = desired_size/(sizeof(float) * ncols);
  }
  else
  {
    nrows = nsamples;
  }

  ArrayMetadata metadata = ArrayMetadata(fs, nrows, nrows, ncols);
  backend->create_array(mrn, &metadata);
  backend->open_array(mrn);
  cout << "Converting mrn: " << mrn << " with " << nsamples << " samples and fs=" << fs <<endl;
  cout << "Array metadata: " << metadata.to_string() << endl;;

  int ch, start_read_offset, end_read_offset, start_write_offset, end_write_offset;
  for (int i = 0; i < ncols; i++)
  {
    ch = CHANNEL_ARRAY[i];
    start_read_offset = 0;
    start_write_offset = 0;
    end_read_offset = min(nsamples, READ_CHUNK_SIZE);
    end_write_offset = min(end_read_offset - start_read_offset, READ_CHUNK_SIZE);
    frowvec chunk_buf = frowvec(end_read_offset);

    // read chunks from each signal and write them
    for (; end_read_offset <= nsamples; end_read_offset = min(end_read_offset + READ_CHUNK_SIZE, nsamples))
    {
      if (end_read_offset - start_read_offset != READ_CHUNK_SIZE) {
        chunk_buf.resize(end_read_offset - start_read_offset);
      }

      edf_backend.read_array(mrn, ch, start_read_offset, end_read_offset, chunk_buf);
      backend->write_array(mrn, ch, start_write_offset, end_write_offset, chunk_buf);

      if ((desired_size == 0 && end_read_offset == nsamples) || end_write_offset >= nrows)
      {
        break;
      }

      end_write_offset = min(end_write_offset + end_read_offset - start_read_offset, nrows);
      start_read_offset = end_read_offset;
      start_write_offset = end_write_offset;

      // Accounts if we get many small chunks at the end to fill the desired_size
      if (end_read_offset == nsamples && start_read_offset != 0)
      {
        start_read_offset = max(0, end_read_offset - READ_CHUNK_SIZE);
      }
    }

    if (!(ch % 2))
    {
      cout << "Wrote ch: " << ch << endl;
    }
  }
  cout << "Write complete" << endl;

  // Logging for experiments
  double diff_secs = ticks_to_seconds(getticks() - start);
  cout << EXPERIMENT_TAG << mrn << "," << TOSTRING(BACKEND) << "," << desired_size << "," << READ_CHUNK_SIZE << "," << diff_secs << endl;
}

