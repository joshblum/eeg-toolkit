#ifndef BACKENDS_H
#define BACKENDS_H

#include <unordered_map>
#include <armadillo>
#include <string>
#include "../config.hpp"
#include "EDFlib/edflib.h"
#include "H5Cpp.h"
#include "TileDB/core/include/capis/tiledb.h"

#define CHUNK_SIZE 4000000 // 4 * 1 000 000 = 4MB chunks
#define CELL_SIZE (sizeof(int32_t) * 2 + sizeof(float)) // struct gets padded to 24 bytes

using namespace std;
using namespace arma;
using namespace H5;

class NotImplementedError: public exception
{
  virtual const char* what() const throw()
  {
    return "Not Implemented!";
  }
};

// Use this to write binary TileDB file
typedef struct cell
{
  int32_t x;
  int32_t y;
  float sample;
} cell_t;

template <class T>
class AbstractStorageBackend
{
  protected:
    // mrn to array obj pointer
    unordered_map<string, T> data_cache;
    char* cache_tag = "-cached";

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
        return T();
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

    string _mrn_to_array_name(string mrn, string ext) {
      return DATADIR + mrn + "." + ext;
    }

    virtual string mrn_to_array_name(string mrn) = 0;

    bool is_cached_array(string array_name)
    {
      return array_name.find(cache_tag) != string::npos;
    }

  public:
    string mrn_to_cached_array_name(string mrn)
    {
      return mrn + cache_tag;
    }

    virtual int get_fs(string mrn) = 0;
    virtual int get_array_len(string mrn) = 0;
    virtual void create_array(string mrn, int nrows, int ncols) = 0;
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
    int get_fs(string mrn);
    int get_array_len(string mrn);
    void create_array(string mrn, int nrows, int ncols);
    void open_array(string mrn);
    void read_array(string mrn, int ch, int start_offset, int end_offset, frowvec& buf);
    void read_array(string mrn, int start_offset, int end_offset, fmat& buf);
    void write_array(string mrn, int ch, int start_offset, int end_offset, fmat& buf);
    void close_array(string mrn);
    void edf_to_array(string filename);
};

 // TODO(joshblum): why can't we pass by ref?
class HDF5Backend: public AbstractStorageBackend<DataSet>
{
  protected:
    string mrn_to_array_name(string mrn);
    int get_array_metadata(string mrn, int metadata_idx);
    void _read_array(string mrn, hsize_t offset[], hsize_t count[], void* buf);

  public:
    int get_fs(string mrn);
    int get_array_len(string mrn);
    void create_array(string mrn, int nrows, int ncols);
    void open_array(string mrn);
    void read_array(string mrn, int ch, int start_offset, int end_offset, frowvec& buf);
    void read_array(string mrn, int start_offset, int end_offset, fmat& buf);
    void write_array(string mrn, int ch, int start_offset, int end_offset, fmat& buf);
    void close_array(string mrn);
    void edf_to_array(string filename);
};

typedef pair<TileDB_CTX*, int> tiledb_cache_pair;

class TileDBBackend: public AbstractStorageBackend<tiledb_cache_pair>
{
  protected:
    string mrn_to_array_name(string mrn);
    string mrn_to_filename(string mrn, string format);
    string get_workspace();

  public:
    int get_fs(string mrn);
    int get_array_len(string mrn);
    void create_array(string mrn, int nrows, int ncols);
    void open_array(string mrn);
    void read_array(string mrn, int ch, int start_offset, int end_offset, frowvec& buf);
    void read_array(string mrn, int start_offset, int end_offset, fmat& buf);
    void write_array(string mrn, int ch, int start_offset, int end_offset, fmat& buf);
    void close_array(string mrn);
    void edf_to_array(string filename);
};

typedef BACKEND StorageBackend;

#endif // BACKENDS_H

