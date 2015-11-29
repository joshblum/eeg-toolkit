#include "backends.hpp"

#include <armadillo>
#include <string>
#include <iostream>
#include <fstream>

#include "../helpers.hpp"
#include "../json11/json11.hpp"

using namespace std;
using namespace arma;
using namespace json11;

string BinaryBackend::mrn_to_array_name(string mrn)
{
  return _mrn_to_array_name(mrn, ".bin");
}

ArrayMetadata BinaryBackend::get_array_metadata(string mrn)
{
  string array_name = mrn_to_array_name(mrn);

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
  ArrayMetadata metadata = ArrayMetadata(fs, nsamples, nrows, ncols);
  metadata.optional_metadata = Json::object
  {
    {"header_offset", (int) (header_len + sizeof(uint32_t))},
  };
  return metadata;
}

void BinaryBackend::create_array(string mrn, ArrayMetadata* metadata)
{
  ofstream file;
  string path = mrn_to_array_name(mrn);
  file.open(path, ios::trunc|ios::binary);

  // Store metadata in header;
  string header = metadata->to_string();
  uint32_t header_len = get_byte_aligned_length(header);
  header.resize(header_len, ' ');
  file.write((char*) &header_len, sizeof(uint32_t));
  file.write(header.c_str(), header_len);
  size_t data_size = metadata->nrows * metadata->ncols * sizeof(float);
  float* tmp = (float*) malloc(data_size);
  file.write((char*) tmp, data_size); // allow us to seek into the file
  free(tmp);
  file.close();
}

void BinaryBackend::open_array(string mrn)
{
  if (!array_exists(mrn)) {
    cout << "Error array " << mrn_to_array_name(mrn) << " does not exist!" << endl;
    exit(1);
  }
  if (in_cache(mrn))
  {
    return;
  }
  ArrayMetadata metadata = get_array_metadata(mrn);
  put_cache(mrn, metadata);
}

void BinaryBackend::read_array(string mrn, int ch, int start_offset, int end_offset, frowvec& buf)
{
  ArrayMetadata metadata = get_cache(mrn);
  uint32_t header_offset = metadata.optional_metadata["header_offset"].int_value();
  int nrows = metadata.nrows;

  ch = CH_REVERSE_IDX[ch];
  size_t row_size = nrows * sizeof(float);
  size_t column_offset = ch * row_size;
  size_t row_offset = start_offset * sizeof(float);
  size_t seek_size = header_offset + column_offset + row_offset;
  size_t read_size = buf.n_cols * sizeof(float);

  string array_name = mrn_to_array_name(mrn);
  ifstream file;
  file.open(array_name, ios::binary);
  file.seekg(seek_size);
  file.read((char*) buf.memptr(), read_size);
  file.close();
}

void BinaryBackend::read_array(string mrn, int start_offset, int end_offset, fmat& buf)
{
  ArrayMetadata metadata = get_cache(mrn);
  uint32_t header_offset = metadata.optional_metadata["header_offset"].int_value();
  int nrows = metadata.nrows;
  int ncols = metadata.ncols;

  string array_name = mrn_to_array_name(mrn);
  ifstream file;
  file.open(array_name, ios::binary);

  size_t read_size = buf.n_cols * sizeof(float);
  size_t row_size = nrows * sizeof(float);
  size_t row_offset = start_offset * sizeof(float);
  size_t column_offset = 0;
  size_t seek_size = header_offset + row_offset;
  frowvec row;
  for (int i = 0; i < ncols; i++)
  {
    file.seekg(seek_size + column_offset);
    row = buf.row(i);
    file.read((char*) row.memptr(), read_size);
    buf.row(i) = row; // needed?
    column_offset += row_size;
  }
  file.close();
}

void BinaryBackend::write_array(string mrn, int ch, int start_offset, int end_offset, fmat& buf)
{
  ArrayMetadata metadata = get_cache(mrn);
  uint32_t header_offset = metadata.optional_metadata["header_offset"].int_value();
  int nrows = metadata.nrows;

  string array_name = mrn_to_array_name(mrn);
  fstream file(array_name, ios::in | ios:: out | ios::binary); // open with ios::in and ios::out flags to write sections of file
  int col;
  if (ch == ALL) {
    col = 0;
  } else {
    col = CH_REVERSE_IDX[ch];
  }
  size_t write_size = buf.n_cols * sizeof(float);
  size_t row_size = nrows * sizeof(float);
  size_t row_offset = start_offset * sizeof(float);
  size_t column_offset = col * row_size;
  size_t seek_size = header_offset + row_offset;
  for (uint i = 0; i < buf.n_rows; i++)
  {
    file.seekp(seek_size + column_offset);
    frowvec row = buf.row(i);
    file.write((char*) row.memptr(), write_size);
    column_offset += row_size;
  }
  file.close();
}

void BinaryBackend::close_array(string mrn)
{
  if (in_cache(mrn)) {
    pop_cache(mrn);
  }
}

