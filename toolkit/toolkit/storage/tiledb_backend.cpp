#include "backends.hpp"
#include "TileDB/core/include/capis/tiledb.h"

#include<string>
#include<armadillo>

using namespace std;
using namespace arma;

#define DIM_NUM 2
#define RANGE_SIZE 2*(DIM_NUM)
#define WORKSPACE "tiledb_workspace/"
#define CATALOG "tiledb_catalog/"

string TileDBBackend::mrn_to_array_name(string mrn)
{
  mrn = WORKSPACE + mrn;
  return _mrn_to_array_name(mrn, "-tiledb");
}

string TileDBBackend::get_array_name(string mrn)
{
  return get_workspace() +  mrn + "-tiledb";
}


string TileDBBackend::get_workspace()
{
  return DATADIR WORKSPACE;
}

string TileDBBackend::get_catalog()
{
  return DATADIR CATALOG;
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

  /* Prepare the array schema. */
  TileDB_ArraySchema array_schema = {};

  string array_name = get_array_name(mrn);
  array_schema.array_name_ = array_name.c_str();

  /* Set attributes and number of attributes. */
  int attribute_num = 1;
  array_schema.attributes_ = new const char*[attribute_num];
  array_schema.attributes_[0] = "sample";
  array_schema.attribute_num_ = attribute_num;

  /* Set dimensions and number of dimensions. */
  int dim_num = DIM_NUM;
  array_schema.dimensions_ = new const char*[dim_num];
  array_schema.dimensions_[1] = "samples";
  array_schema.dimensions_[0] = "channels";
  array_schema.dim_num_ = dim_num;

  /* The array is dense. */
  array_schema.dense_ = 1;
  array_schema.cell_order_ = "row-major";
  //array_schema.tile_order_ = "column-major";

  /* Set compression. */
  array_schema.compression_ = new const char*[attribute_num + 1];
  array_schema.compression_[0] = "NONE";
  array_schema.compression_[1] = "GZIP";

  /* Set tile extents. */
  array_schema.tile_extents_ = new double[dim_num];
  if (is_cached_array(mrn))
  {
    array_schema.tile_extents_[1] = min(metadata->nrows, WRITE_CHUNK_SIZE);
    array_schema.tile_extents_[0] = metadata->ncols;
  }
  else
  {
    array_schema.tile_extents_[1] = min(metadata->nrows, READ_CHUNK_SIZE);
    array_schema.tile_extents_[0] = 1;
  }

  /* Set domain. */
  array_schema.domain_ = new double[2*dim_num];
  array_schema.domain_[0] = 0;
  array_schema.domain_[3] = metadata->nrows;
  array_schema.domain_[2] = 0;
  array_schema.domain_[1] = metadata->ncols;

  /* Set types: float32 for "sample" and int64 for the coordinates. */
  array_schema.types_ = new const char*[attribute_num + 1];
  array_schema.types_[0] = "float32";
  array_schema.types_[1] = "int32";

  // Initialize TileDB
  TileDB_CTX* tiledb_ctx;
  tiledb_ctx_init(&tiledb_ctx);

  // First we need a workspace and catalog set
  if (tiledb_workspace_create(tiledb_ctx, get_workspace().c_str(), get_catalog().c_str()) == TILEDB_OK)
  {
    cout << "TileDB workspace created." << endl;
  }

  if (array_exists(mrn))
  {
    // Necessary for clean since arrays are not cleared when redefined
    if (tiledb_array_delete(tiledb_ctx, array_name.c_str()) == TILEDB_ERR)
    {
      cout << "TileDB delete error." << endl;
      exit(-1);
    }
  }

  /* Create the array. */
  if (tiledb_array_create(tiledb_ctx, &array_schema) == TILEDB_ERR)
  {
    cout << "TileDB define error." << endl;
    exit(-1);
  }

  /* Clean up. */
  delete [] array_schema.compression_;
  delete [] array_schema.attributes_;
  delete [] array_schema.dimensions_;
  delete [] array_schema.tile_extents_;
  delete [] array_schema.domain_;
  delete [] array_schema.types_;

  tiledb_ctx_finalize(tiledb_ctx);
}

void TileDBBackend::open_array(string mrn)
{
  _open_array(mrn, TILEDB_ARRAY_MODE_READ);
}

void TileDBBackend::_open_array(string mrn, int mode)
{
  if (in_cache(mrn))
  {
    return;
  }
  TileDB_CTX* tiledb_ctx;
  tiledb_ctx_init(&tiledb_ctx);
  string array_name = get_array_name(mrn);
  int array_id = tiledb_array_open(tiledb_ctx, array_name.c_str(), mode);
  if (array_id == TILEDB_ERR)
  {
    cout << "TileDB open error." << endl;
    exit(-1);
  }

  put_cache(mrn, tiledb_cache_pair(tiledb_ctx, array_id));
}

void TileDBBackend::read_array(string mrn, int ch, int start_offset, int end_offset, frowvec& buf)
{
  double* range = new double[RANGE_SIZE];
  range[0] = CH_REVERSE_IDX[ch];
  range[1] = CH_REVERSE_IDX[ch];
  range[2] = start_offset;
  range[3] = start_offset + buf.n_cols;
  _read_array(mrn, range, buf);
}

void TileDBBackend::read_array(string mrn, int start_offset, int end_offset, fmat& buf)
{
  double* range = new double[RANGE_SIZE];
  range[0] = start_offset;
  range[1] = start_offset + buf.n_rows;
  range[2] = 0;
  range[3] = buf.n_cols;
  _read_array(mrn, range, buf);
}

void TileDBBackend::_read_array(string mrn, double* range, fmat& buf)
{
  tiledb_cache_pair cache_pair = get_cache(mrn);
  TileDB_CTX* tiledb_ctx = cache_pair.first;
  int array_id = cache_pair.second;
  int dim_num = 0;
  const char** dims = new const char*[dim_num];
  dims[0] = "__hide";

  int attribute_num = 0;
  const char** attributes = NULL;
  size_t buf_size = sizeof(float) * buf.n_elem;
  tiledb_array_read(tiledb_ctx, array_id, range,
      dims, dim_num,
      attributes, attribute_num,
      buf.memptr(), &buf_size);
}

void TileDBBackend::write_array(string mrn, int ch, int start_offset, int end_offset, fmat& buf)
{
  _open_array(mrn, TILEDB_ARRAY_MODE_WRITE);
  tiledb_cache_pair pair = get_cache(mrn);
  TileDB_CTX* tiledb_ctx = pair.first;
  int array_id = pair.second;
  if (ch == ALL) {
    buf = buf.t();
  }
  for (uword i = 0; i < buf.n_rows; i++)
  {
    for (uword j = 0; j < buf.n_cols; j++)
    {
      tiledb_array_write_dense(tiledb_ctx, array_id, &buf(i,j));
    }
  }
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

