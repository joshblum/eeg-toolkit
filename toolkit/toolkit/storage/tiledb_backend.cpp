#include "backends.hpp"

#include<string>
#include<armadillo>

using namespace std;
using namespace arma;

#define DIM_NAME "__hide"
#define ATTR_NAME "sample"
#define ROW_NAME "samples"
#define COL_NAME "channels"
#define RANGE_SIZE 4

string TileDBBackend::mrn_to_array_name(string mrn)
{
  return mrn + "-tiledb";
}

string TileDBBackend::get_workspace()
{
  return DATADIR;
}

int TileDBBackend::get_fs(string mrn)
{
  return 256; // TODO(joshblum) store this in TileDB metadata when it's implemented
}

int TileDBBackend::get_array_len(string mrn)
{
  return 84992; // TODO(joshblum) store this in TileDB metadata when it's implemented
}

void TileDBBackend::create_array(string mrn, ArrayMetadata* metadata)
{

}

void TileDBBackend::open_array(string mrn)
{
  if (in_cache(mrn))
  {
    return;
  }
  TileDB_CTX* tiledb_ctx;
  tiledb_ctx_init(tiledb_ctx);
  string group = "";
  string workspace = get_workspace();
  string array_name = mrn_to_array_name(mrn);
  const char* mode = "r"; // TODO(joshblum): if read/write available this would be better
  int array_id = tiledb_array_open(tiledb_ctx, workspace.c_str(), group.c_str(), array_name.c_str(), mode);
  put_cache(mrn, tiledb_cache_pair(tiledb_ctx, array_id));
}

void TileDBBackend::read_array(string mrn, int ch, int start_offset, int end_offset, frowvec& buf)
{
  tiledb_cache_pair cache_pair = get_cache(mrn);
  TileDB_CTX* tiledb_ctx = cache_pair.first;
  int array_id = cache_pair.second;

  double* range = new double[RANGE_SIZE];
  range[0] = CH_REVERSE_IDX[ch];
  range[1] = CH_REVERSE_IDX[ch];
  range[2] = start_offset;
  range[3] = end_offset;

  const char* dim_names = DIM_NAME;
  int dim_names_num = 1;
  const char* attribute_names = ATTR_NAME;
  int attribute_names_num = 1;
  size_t cell_buf_size = sizeof(float) * buf.n_elem;
  tiledb_subarray_buf(tiledb_ctx, array_id, range,
      RANGE_SIZE, &dim_names, dim_names_num,
      &attribute_names, attribute_names_num,
      buf.memptr(), &cell_buf_size);
}

void TileDBBackend::read_array(string mrn, int start_offset, int end_offset, fmat& buf)
{
}

void TileDBBackend::write_array(string mrn, int ch, int start_offset, int end_offset, fmat& buf)
{
}

void TileDBBackend::close_array(string mrn)
{
  if (in_cache(mrn))
  {
    tiledb_cache_pair pair = get_cache(mrn);
    tiledb_array_close(pair.first, pair.second);
    pop_cache(mrn);
  }
}

void TileDBBackend::edf_to_array(string mrn)
{
  EDFBackend edf_backend;
  edf_backend.open_array(mrn);

  int nchannels = NCHANNELS;
  int nsamples = edf_backend.get_array_len(mrn);
  int fs = edf_backend.get_fs(mrn); // TODO(joshblum) store this in TileDB metadata when it's implemented
  cout << "Writing " << nsamples << " samples with fs=" << fs << "." << endl;

  string group = "";
  string workspace = get_workspace();
  string array_name = mrn_to_array_name(mrn);
  // csv of array schema
  string array_schema_str = array_name + ",1," + ATTR_NAME + ",2," + COL_NAME + "," + ROW_NAME + ",0," + to_string(nsamples) + ",0," + to_string(nchannels) + ",float32,int32,*,column-major,*,*,*,*";

  // Initialize TileDB
  TileDB_CTX* tiledb_ctx;
  tiledb_ctx_init(tiledb_ctx);

  cout << "Defining TileDB schema." << endl;
  // Store the array schema
  if (tiledb_array_define(tiledb_ctx, workspace.c_str(), group.c_str(), array_schema_str.c_str()))
  {
    tiledb_ctx_finalize(tiledb_ctx);
    return;
  }

  cout << "Opening TileDB array." << endl;
  const char* mode = "a";
  int array_id = tiledb_array_open(tiledb_ctx, workspace.c_str(), group.c_str(), array_name.c_str(), mode);

  cout << "Writing cells to TileDB." << endl;
  int ch, start_offset, end_offset;
  cell_t cell;
  for (int i = 0; i < nchannels; i++)
  {
    ch = CHANNEL_ARRAY[i];
    start_offset = 0;
    end_offset = min(nsamples, CHUNK_SIZE);
    frowvec chunk_buf = frowvec(end_offset); // store samples from each channel here

    // read chunks from each signal and write them
    for (; end_offset <= nsamples; end_offset = min(end_offset + CHUNK_SIZE, nsamples))
    {
      if (end_offset - start_offset != CHUNK_SIZE) {
        chunk_buf.resize(end_offset - start_offset);
      }
      edf_backend.read_array(mrn, ch, start_offset, end_offset, chunk_buf);

      // write chunk_mat to file
      for (uword i = 0; i < chunk_buf.n_elem; i++)
      {
        // x coord, y coord, attribute
        cell.x = CH_REVERSE_IDX[ch];
        cell.y = start_offset + i;
        cell.sample = chunk_buf(i);
        tiledb_cell_write_sorted(tiledb_ctx, array_id, &cell);
      }

      start_offset = end_offset;
      // ensure we write the last part of the samples
      if (end_offset == nsamples)
      {
        break;
      }

      if (!(ch % 2 || (end_offset / CHUNK_SIZE) % 10)) // print for even channels every 10 chunks (40MB)
      {
        cout << "Wrote " << end_offset / CHUNK_SIZE << " chunks for ch: " << ch << endl;
      }
    }
  }

  // Finalize TileDB
  tiledb_array_close(tiledb_ctx, array_id);
  tiledb_ctx_finalize(tiledb_ctx);
  cout << "Conversion complete." << endl;
}

