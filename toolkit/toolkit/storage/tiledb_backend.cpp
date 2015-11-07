#include "backends.hpp"
#include <sys/stat.h>
#include <fstream>

#define DIM_NAME "__hide"
#define ATTR_NAME "sample"
#define ROW_NAME "samples"
#define COL_NAME "channels"
#define RANGE_SIZE 4
#define CELL_SIZE (sizeof(int64_t) * 2 + sizeof(float)) // struct gets padded to 24 bytes

string TileDBBackend::mrn_to_array_name(string mrn)
{
  return mrn + "-tiledb";
}

string TileDBBackend::get_workspace()
{
  return DATADIR;
}

string TileDBBackend::mrn_to_filename(string mrn)
{
  return DATADIR + mrn + ".bin";
}


int TileDBBackend::get_fs(string mrn)
{
  return 256; // TODO(joshblum) store this in TileDB metadata when it's implemented
}

int TileDBBackend::get_data_len(string mrn)
{
  return 84992; // TODO(joshblum) store this in TileDB metadata when it's implemented
}

void TileDBBackend::load_array(string mrn)
{
  TileDB_CTX* tiledb_ctx;
  tiledb_ctx_init(tiledb_ctx);
  string group = "";
  string workspace = get_workspace();
  string array_name = mrn_to_array_name(mrn);
  const char* mode = "r";
  int array_id = tiledb_array_open(tiledb_ctx, workspace.c_str(), group.c_str(), array_name.c_str(), mode);
  put_cache(mrn, tiledb_cache_pair(tiledb_ctx, array_id));
}

void TileDBBackend::get_array_data(string mrn, int ch, int start_offset, int end_offset, frowvec& buf)
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

void TileDBBackend::close_array(string mrn)
{
  tiledb_cache_pair pair = get_cache(mrn);
  tiledb_array_close(pair.first, pair.second);
  pop_cache(mrn);
}

// Use this to write binary TileDB file
typedef struct cell
{
  int64_t x;
  int64_t y;
  float sample;
} cell_t;


void TileDBBackend::edf_to_bin(string mrn, string path)
{
  // don't convert if the file already exists.
  struct stat buffer;
  if (stat (path.c_str(), &buffer) == 0) {
    cout << "File : " << path << " already exists." << endl;
    return;
  }

  ofstream file;
  file.open(path, ios::trunc|ios::binary);

  EDFBackend edf_backend;
  edf_backend.load_array(mrn);

  int nchannels = NCHANNELS;
  int nsamples = edf_backend.get_data_len(mrn);
  int start_offset = 0;
  int end_offset = min(nsamples, CHUNK_SIZE);

  // write samples in chunks in a nchannels x CHUNK_SIZE matrix
  fmat chunk_mat = fmat(nchannels, end_offset);
  frowvec chunk_buf; // store samples from each channel here
  int ch;
  cell_t cell;

  for (; end_offset <= nsamples; end_offset = min(end_offset + CHUNK_SIZE, nsamples))
  {
    // for the last chunk
    if (end_offset - start_offset != CHUNK_SIZE)
    {
      chunk_mat.resize(nchannels, end_offset);
    }

    for (int i = 0; i < nchannels; i++)
    {
      chunk_buf = chunk_mat.row(i);
      ch = CHANNEL_ARRAY[i];
      edf_backend.get_array_data(mrn, ch, start_offset, end_offset, chunk_buf);
      chunk_mat.row(i) = chunk_buf;
    }

    // write chunk_mat to file
    for (uword i = 0; i < chunk_mat.n_rows; i++)
    {
      for (uword j = 0; j < chunk_mat.n_cols; j++)
      {
        // x coord, y coord, attribute
        cell.x = i;
        cell.y = start_offset + j;
        cell.sample = chunk_mat(i, j);
        file.write((char*) &cell, CELL_SIZE);
      }
    }

    if (!((end_offset / CHUNK_SIZE) % 10)) // print for even channels every 10 chunks (40MB)
    {
      cout << "Wrote " << end_offset / CHUNK_SIZE << " chunks to file." << endl;
    }

    // ensure we write the last part of the samples
    if (end_offset == nsamples)
    {
      break;
    }
    start_offset = end_offset;
  }
  file.close();
}

void TileDBBackend::edf_to_array(string mrn)
{
  EDFBackend edf_backend;
  edf_backend.load_array(mrn);

  int nchannels = NCHANNELS;
  int nsamples = edf_backend.get_data_len(mrn);
  int fs = edf_backend.get_fs(mrn); // TODO(joshblum) store this in TileDB metadata when it's implemented
  cout << "Writing " << nsamples << " samples with fs=" << fs << "." << endl;
  edf_backend.close_array(mrn);

  string group = "";
  string workspace = get_workspace();
  string array_name = mrn_to_array_name(mrn);
  // csv of array schema
  string array_schema_str = array_name + ",1," + ATTR_NAME + ",2," + COL_NAME + "," + ROW_NAME + ",0," + to_string(nsamples) + ",0," + to_string(nchannels) + ",float32,int64,*,column-major,*,*,*,*";

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

  string path = mrn_to_filename(mrn);
  cout << "Converting to file at: " << path << endl;
  edf_to_bin(mrn, path);
  string format = "bin";
  char delimiter = ',';
  cout << "Loading: " << path << " into TileDB." << endl;
  if (tiledb_array_load(tiledb_ctx, workspace.c_str(), group.c_str(),
                             array_name.c_str(), path.c_str(),
                             format.c_str(), delimiter))
  {
    tiledb_ctx_finalize(tiledb_ctx);
    return;
  }
  cout << "Conversion complete." << endl;

  // Finalize TileDB
  tiledb_ctx_finalize(tiledb_ctx);
}

