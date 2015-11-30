#ifndef BACKENDS_H
#define BACKENDS_H

#include <unordered_map>
#include <armadillo>
#include <string>
#include <sys/stat.h>

#include "../json11/json11.hpp"
#include "../config.hpp"
#include "../helpers.hpp"
#include "EDFlib/edflib.h"
#include "H5Cpp.h"
#include "TileDB/core/include/capis/tiledb.h"

#define CELL_SIZE (sizeof(int32_t) * 2 + sizeof(float)) // struct gets padded to 24 bytes

using namespace std;
using namespace arma;
using namespace H5;
using namespace json11;

class NotImplementedError: public exception
{
  virtual const char* what() const throw()
  {
    return "Not Implemented!";
  }
};

/*
 * Object to store 2D array metadata
 */
class ArrayMetadata
{
  public:
    int fs;
    int nsamples;
    int nrows;
    int ncols;
    Json optional_metadata;

    ArrayMetadata()
    {
      fs = 0;
      nsamples = 0;
      nrows = 0;
      ncols = 0;
    }

    ArrayMetadata(int fs, int nsamples, int nrows, int ncols)
    {
      this->fs = fs;
      this->nsamples = nsamples;
      this->nrows = nrows;
      this->ncols = ncols;
    }

    Json to_json()
    {
      return Json::object
      {
        {"fs", fs},
        {"nsamples", nsamples},
        {"ncols", ncols},
        {"nrows", nrows}
      };
    }

    string to_string() {
      return to_json().dump();
    }
};

// Use this to write binary file
typedef struct cell
{
  int32_t x;
  int32_t y;
  float sample;
} cell_t;

/*
 * Abstraction for array based storage engine
 */
template <class T>
class AbstractStorageBackend
{
  protected:
    // mrn to array obj pointer
    unordered_map<string, T> data_cache;
    const char* cache_tag = "-cached";

    bool in_cache(string mrn)
    {
      auto iter = data_cache.find(mrn);
      return iter != data_cache.end();
    }

    /*
     * Checks the `data_cache` for a specific mrn.
     * Returns a pointer to an `T` if it is present
     * else `T()`
     */
    T get_cache(string mrn)
    {
      auto iter = data_cache.find(mrn);
      if (iter == data_cache.end())
      {
        open_array(mrn);
        return get_cache(mrn);
      } else {
        return iter->second;
      }
    }

    /*
     * Set the key-value pair `mrn`:`ptr` in the cache
     */
    void put_cache(string mrn, T ptr)
    {
      data_cache[mrn] = ptr;
    }

    /*
     * Delete the value from the cache and free the ptr
     */
    void pop_cache(string mrn)
    {
      data_cache.erase(mrn);
    }

    /*
     * Internal method which determines where
     * a data file lives in the filesystem
     */
    string _mrn_to_array_name(string mrn, string file_ext) {
      return DATADIR + mrn + file_ext;
    }

    virtual string mrn_to_array_name(string mrn) = 0;

    /*
     * Returns a bool if the given array name is a cached array
     */
    bool is_cached_array(string array_name)
    {
      return array_name.find(cache_tag) != string::npos;
    }


  public:
    /*
     * Convert a `mrn` and `ch_name` to the appropiate cached name
     */
    string mrn_to_cached_mrn_name(string mrn, string ch_name)
    {
      return mrn + "-" + ch_name + cache_tag;
    }

    /*
     * Determine if an array exists given the `mrn`
     */
    bool array_exists(string mrn)
    {
      string array_name = mrn_to_array_name(mrn);
      return file_exists(array_name);
    }

    /////// METADATA GETTERS ///////
    int get_fs(string mrn)
    {
      return get_array_metadata(mrn).fs;
    }

    int get_nsamples(string mrn)
    {
      return get_array_metadata(mrn).nsamples;
    }

    int get_nrows(string mrn)
    {
      return get_array_metadata(mrn).nrows;
    }

    int get_ncols(string mrn)
    {
      return get_array_metadata(mrn).ncols;
    }

    virtual ArrayMetadata get_array_metadata(string mrn) = 0;
    virtual void create_array(string mrn, ArrayMetadata* metadata) = 0;
    virtual void open_array(string mrn) = 0;
    virtual void read_array(string mrn, int ch, int start_offset, int end_offset, frowvec& buf) = 0;
    virtual void write_array(string mrn, int ch, int start_offset, int end_offset, fmat& buf) = 0;
    virtual void read_array(string mrn, int start_offset, int end_offset, fmat& buf) = 0;
    virtual void close_array(string mrn) = 0;
};

class EDFBackend: public AbstractStorageBackend<edf_hdr_struct*>
{
  protected:
    string mrn_to_array_name(string mrn);

  public:
    ArrayMetadata get_array_metadata(string mrn);
    void create_array(string mrn, ArrayMetadata* metadata);
    void open_array(string mrn);
    void read_array(string mrn, int ch, int start_offset, int end_offset, frowvec& buf);
    void read_array(string mrn, int start_offset, int end_offset, fmat& buf);
    void write_array(string mrn, int ch, int start_offset, int end_offset, fmat& buf);
    void close_array(string mrn);
};

class BinaryBackend: public AbstractStorageBackend<ArrayMetadata>
{
  protected:
    string mrn_to_array_name(string mrn);

  public:
    ArrayMetadata get_array_metadata(string mrn);
    void create_array(string mrn, ArrayMetadata* metadata);
    void open_array(string mrn);
    void read_array(string mrn, int ch, int start_offset, int end_offset, frowvec& buf);
    void read_array(string mrn, int start_offset, int end_offset, fmat& buf);
    void write_array(string mrn, int ch, int start_offset, int end_offset, fmat& buf);
    void close_array(string mrn);
};

 // TODO(joshblum): why can't we pass by ref?
class HDF5Backend: public AbstractStorageBackend<DataSet>
{
  protected:
    string mrn_to_array_name(string mrn);
    void _read_array(string mrn, hsize_t offset[], hsize_t count[], void* buf);

  public:
    ArrayMetadata get_array_metadata(string mrn);
    void create_array(string mrn, ArrayMetadata* metadata);
    void open_array(string mrn);
    void read_array(string mrn, int ch, int start_offset, int end_offset, frowvec& buf);
    void read_array(string mrn, int start_offset, int end_offset, fmat& buf);
    void write_array(string mrn, int ch, int start_offset, int end_offset, fmat& buf);
    void close_array(string mrn);
};

typedef pair<TileDB_CTX*, int> tiledb_cache_pair;

class TileDBBackend: public AbstractStorageBackend<tiledb_cache_pair>
{
  protected:
    string mrn_to_array_name(string mrn);
    string get_array_name(string mrn);
    string get_group();
    string get_workspace();
    void _open_array(string mrn, const char* mode);
    void _read_array(string mrn, double* range, fmat& buf);

  public:
    ArrayMetadata get_array_metadata(string mrn);
    void create_array(string mrn, ArrayMetadata* metadata);
    void open_array(string mrn);
    void read_array(string mrn, int ch, int start_offset, int end_offset, frowvec& buf);
    void read_array(string mrn, int start_offset, int end_offset, fmat& buf);
    void write_array(string mrn, int ch, int start_offset, int end_offset, fmat& buf);
    void close_array(string mrn);
};

typedef BACKEND StorageBackend;

template <typename T>
void edf_to_array(string mrn, AbstractStorageBackend<T>* backend, size_t desired_size=0);

#endif // BACKENDS_H

