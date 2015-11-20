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

void BinaryBackend::edf_to_array(string mrn)
{
  EDFBackend edf_backend;
  edf_backend.open_array(mrn);

  int nchannels = NCHANNELS;
  int nsamples = edf_backend.get_nsamples(mrn);
  int fs = edf_backend.get_fs(mrn);

  cout << "Converting mrn: " << mrn << " with " << nsamples << " samples and fs=" << fs <<endl;
  ArrayMetadata metadata = ArrayMetadata(fs, nsamples, nsamples, nchannels);
  create_array(mrn, &metadata);
  open_array(mrn);

  int ch, start_offset, end_offset;
  for (int i = 0; i < nchannels; i++)
  {
    ch = CHANNEL_ARRAY[i];
    start_offset = 0;
    end_offset = min(nsamples, CHUNK_SIZE);
    frowvec chunk_buf = frowvec(end_offset);

    // read chunks from each signal and write them
    for (; end_offset <= nsamples; end_offset = min(end_offset + CHUNK_SIZE, nsamples))
    {
      if (end_offset - start_offset != CHUNK_SIZE) {
        chunk_buf.resize(end_offset - start_offset);
      }
      edf_backend.read_array(mrn, ch, start_offset, end_offset, chunk_buf);
      write_array(mrn, ch, start_offset, end_offset, chunk_buf);

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

  cout << "Write complete" << endl;
}

