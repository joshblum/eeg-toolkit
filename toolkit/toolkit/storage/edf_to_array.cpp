#include "backends.hpp"

#include<string>
#include<armadillo>

using namespace std;
using namespace arma;

/*
 * Convert an EDF file to array format.  If `desired_size > 0`, the output will
 * have extra data to match the `desired_size`.  Otherwise, the file will be
 * converted directly with no additional data.
 */
template<typename T>
void edf_to_array(AbstractStorageBackend<T>* backend, string mrn, size_t desired_size)
{
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

  ArrayMetadata metadata = ArrayMetadata(fs, nsamples, nrows, ncols);
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
    end_write_offset = min(nsamples, READ_CHUNK_SIZE);
    frowvec chunk_buf = frowvec(end_read_offset);

    size_t cur_row_size = 0;
    // read chunks from each signal and write them
    for (; end_read_offset <= nsamples; end_read_offset = min(end_read_offset + READ_CHUNK_SIZE, nsamples))
    {
      end_write_offset = min(end_write_offset + READ_CHUNK_SIZE, nrows);

      if (end_read_offset - start_read_offset != READ_CHUNK_SIZE) {
        chunk_buf.resize(end_read_offset - start_read_offset);
      }

      edf_backend.read_array(mrn, ch, start_read_offset, end_read_offset, chunk_buf);
      backend->write_array(mrn, ch, start_write_offset, end_write_offset, chunk_buf);

      if ((desired_size == 0 && end_read_offset == nsamples) || end_write_offset >= nrows)
      {
        break;
      }

      start_read_offset = end_read_offset;
      start_write_offset = end_write_offset;

      // Accounts if we get many small chunks at the end to fill the desired_size
      if (end_read_offset == nsamples && start_read_offset != 0)
      {
        start_read_offset = max(0, end_read_offset - READ_CHUNK_SIZE);
      }

      if (!(ch % 2 || (end_write_offset / READ_CHUNK_SIZE) % 10)) // print for even channels every 10 chunks (40MB)
      {
        cout << "Wrote " << end_write_offset / READ_CHUNK_SIZE << " chunks for ch: " << ch << endl;
      cout << "start_write_offset: " << start_write_offset << " end_write_offset: " << end_write_offset << " cur_row_size: " << cur_row_size << endl;
      }
    }
  }

  cout << "Write complete" << endl;
}

// Explicit template declarations
// BinaryBackend
template void edf_to_array(AbstractStorageBackend<ArrayMetadata>* backend, string mrn, size_t desired_size);

// HDF5Backend
template void edf_to_array(AbstractStorageBackend<DataSet>* backend, string mrn, size_t desired_size);

// TileDBBackend
template void edf_to_array(AbstractStorageBackend<tiledb_cache_pair>* backend, string mrn, size_t desired_size);
