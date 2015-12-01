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
#define TILEDB_READ_MODE "r"
#define TILEDB_WRITE_MODE "w"
#define TILEDB_APPEND_MODE "a"

string TileDBBackend::mrn_to_array_name(string mrn)
{
  return _mrn_to_array_name(mrn, "-tiledb");
}

string TileDBBackend::get_array_name(string mrn)
{
  return mrn + "-tiledb";
}


string TileDBBackend::get_workspace()
{
  return DATADIR;
}

string TileDBBackend::get_group()
{
  return "";
}

ArrayMetadata TileDBBackend::get_array_metadata(string mrn)
{
  string array_name = mrn_to_array_name(mrn) + "-metadata";

  ifstream file;
  file.open(array_name, ios::binary);

  uint32_t header_len;
  file.read((char*) &header_len, sizeof(uint32_t));

  string header = "";
  header.resize(header_len, ' ');
  file.read((char*) header.c_str(), header_len);
  file.close();

  string err;
  Json json = Json::parse(header, err);
  int fs = json["fs"].int_value();
  int nsamples = json["nsamples"].int_value();
  int nrows = json["nrows"].int_value();
  int ncols = json["ncols"].int_value();
  return ArrayMetadata(fs, nsamples, nrows, ncols);
}

void TileDBBackend::write_metadata(string mrn, ArrayMetadata* metadata)
{
  ofstream file;
  string path = mrn_to_array_name(mrn) + "-metadata";
  file.open(path, ios::trunc|ios::binary);

  // Store metadata in header;
  string header = metadata->to_string();
  uint32_t header_len = get_byte_aligned_length(header);
  header.resize(header_len, ' ');
  file.write((char*) &header_len, sizeof(uint32_t));
  file.write(header.c_str(), header_len);
  file.close();
}

void TileDBBackend::create_array(string mrn, ArrayMetadata* metadata)
{
  // tmp fix until TileDB metadata is implemented
  write_metadata(mrn, metadata);
  string group = get_group();
  string workspace = get_workspace();
  string array_name = get_array_name(mrn);
  // csv of array schema
  string array_schema_str = array_name + ",1," + ATTR_NAME + ",2," + COL_NAME + "," + ROW_NAME + ",0," + to_string(metadata->ncols) + ",0," + to_string(metadata->nrows) + ",float32,int32,*,column-major,*,*,*,*";

  // Initialize TileDB
  TileDB_CTX* tiledb_ctx;
  tiledb_ctx_init(tiledb_ctx);

  // Store the array schema
  tiledb_array_define(tiledb_ctx, workspace.c_str(), group.c_str(), array_schema_str.c_str());
  tiledb_ctx_finalize(tiledb_ctx);
}

void TileDBBackend::open_array(string mrn)
{
  _open_array(mrn, TILEDB_READ_MODE);
}

void TileDBBackend::_open_array(string mrn, const char* mode)
{
  if (in_cache(mrn))
  {
    return;
  }
  TileDB_CTX* tiledb_ctx;
  tiledb_ctx_init(tiledb_ctx);
  string group = get_group();
  string workspace = get_workspace();
  string array_name = get_array_name(mrn);
  int array_id = tiledb_array_open(tiledb_ctx, workspace.c_str(), group.c_str(), array_name.c_str(), mode);
  put_cache(mrn, tiledb_cache_pair(tiledb_ctx, array_id));
}

void TileDBBackend::read_array(string mrn, int ch, int start_offset, int end_offset, frowvec& buf)
{
  double* range = new double[RANGE_SIZE];
  range[0] = CH_REVERSE_IDX[ch];
  range[1] = CH_REVERSE_IDX[ch];
  range[2] = start_offset;
  range[3] = end_offset;
  _read_array(mrn, range, buf);
}

void TileDBBackend::read_array(string mrn, int start_offset, int end_offset, fmat& buf)
{
  double* range = new double[RANGE_SIZE];
  range[0] = 0;
  range[1] = get_ncols(mrn);
  range[2] = start_offset;
  range[3] = end_offset;
  _read_array(mrn, range, buf);
}

void TileDBBackend::_read_array(string mrn, double* range, fmat& buf)
{
  tiledb_cache_pair cache_pair = get_cache(mrn);
  TileDB_CTX* tiledb_ctx = cache_pair.first;
  int array_id = cache_pair.second;
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

void TileDBBackend::write_array(string mrn, int ch, int start_offset, int end_offset, fmat& buf)
{
  if (in_cache(mrn))
  {
    close_array(mrn); // we need to reopen in append mode
  }
  _open_array(mrn, TILEDB_APPEND_MODE);
  tiledb_cache_pair pair = get_cache(mrn);
  TileDB_CTX* tiledb_ctx = pair.first;
  int array_id = pair.second;
  cell_t cell;
  if (ch == ALL) {
    buf = buf.t();
  }
  for (uword i = 0; i < buf.n_rows; i++)
  {
    for (uword j = 0; j < buf.n_cols; j++)
    {
      // x (col) coord, y (row) coord, attribute
      cell.x = start_offset + j;
      cell.y = ch == ALL ? i : CH_REVERSE_IDX[ch];
      cell.sample = buf(i, j);
      tiledb_cell_write_sorted(tiledb_ctx, array_id, &cell);
    }
  }

  // Finalize TileDB
  close_array(mrn);
}

void TileDBBackend::close_array(string mrn)
{
  if (in_cache(mrn))
  {
    tiledb_cache_pair pair = get_cache(mrn);
    tiledb_array_close(pair.first, pair.second);
    tiledb_ctx_finalize(pair.first);
    pop_cache(mrn);
  }
}

