#include "backends.hpp"

#include<string>
#include<armadillo>

using namespace std;
using namespace arma;

template<typename T>
void edf_to_array(AbstractStorageBackend<T>* backend, string mrn)
{
  EDFBackend edf_backend;
  edf_backend.open_array(mrn);

  int fs = edf_backend.get_fs(mrn);
  int nsamples = edf_backend.get_nsamples(mrn);
  int nrows = nsamples;
  int ncols = NCHANNELS;

  cout << "Converting mrn: " << mrn << " with " << nsamples << " samples and fs=" << fs <<endl;
  ArrayMetadata metadata = ArrayMetadata(fs, nsamples, nrows, ncols);
  backend->create_array(mrn, &metadata);
  backend->open_array(mrn);

  int ch, start_offset, end_offset;
  for (int i = 0; i < ncols; i++)
  {
    ch = CHANNEL_ARRAY[i];
    start_offset = 0;
    end_offset = min(nsamples, READ_CHUNK_SIZE);
    frowvec chunk_buf = frowvec(end_offset);

    // read chunks from each signal and write them
    for (; end_offset <= nsamples; end_offset = min(end_offset + READ_CHUNK_SIZE, nsamples))
    {
      if (end_offset - start_offset != READ_CHUNK_SIZE) {
        chunk_buf.resize(end_offset - start_offset);
      }
      edf_backend.read_array(mrn, ch, start_offset, end_offset, chunk_buf);
      backend->write_array(mrn, ch, start_offset, end_offset, chunk_buf);

      start_offset = end_offset;
      // ensure we write the last part of the samples
      if (end_offset == nsamples)
      {
        break;
      }

      if (!(ch % 2 || (end_offset / READ_CHUNK_SIZE) % 10)) // print for even channels every 10 chunks (40MB)
      {
        cout << "Wrote " << end_offset / READ_CHUNK_SIZE << " chunks for ch: " << ch << endl;
      }
    }
  }

  cout << "Write complete" << endl;
}

// Explicit template declarations
// BinaryBackend
template void edf_to_array(AbstractStorageBackend<ArrayMetadata>* backend, string mrn);

// HDF5Backend
template void edf_to_array(AbstractStorageBackend<DataSet>* backend, string mrn);

// TileDBBackend
template void edf_to_array(AbstractStorageBackend<tiledb_cache_pair>* backend, string mrn);
