#ifndef BACKENDS_H
#define BACKENDS_H

#include <unordered_map>
#include <armadillo>
#include <string>
#include "H5Cpp.h"
#include "../config.hpp"
#include "EDFlib/edflib.h"

using namespace std;
using namespace arma;
using namespace H5;

template <class T>
class AbstractStorageBackend
{
  protected:
    // mrn to array obj pointer
    unordered_map<string, T> data_cache;
    /*
     * Checks the `data_cache` for a specific mrn.
     * Returns a pointer to an `T` if it is present
     * else `NULL`
     */
    T get_cache(string mrn)
    {
      auto iter = data_cache.find(mrn);
      if (iter == data_cache.end())
      {
        return NULL;
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

  public:
    virtual int get_fs(string mrn) = 0;
    virtual int get_data_len(string mrn) = 0;
    virtual void load_array(string mrn) = 0;
    virtual void get_array_data(string mrn, int ch, int start_offset, int end_offset, frowvec& buf) = 0;
    virtual void close_array(string mrn) = 0;
};

class EDFBackend: public AbstractStorageBackend<edf_hdr_struct*>
{
  protected:
    string mrn_to_array_name(string mrn);

  public:
    int get_fs(string mrn);
    int get_data_len(string mrn);
    void load_array(string mrn);
    void get_array_data(string mrn, int ch, int start_offset, int end_offset, frowvec& buf);
    void close_array(string mrn);
    void edf_to_array(string filename);
};

 // TODO(joshblum): why can't we pass by ref?
class HDF5Backend: public AbstractStorageBackend<DataSet>
{
  protected:
    string mrn_to_array_name(string mrn);
    int get_array_metadata(string mrn, int metadata_idx);

  public:
    int get_fs(string mrn);
    int get_data_len(string mrn);
    void load_array(string mrn);
    void get_array_data(string mrn, int ch, int start_offset, int end_offset, frowvec& buf);
    void close_array(string mrn);
    void edf_to_array(string filename);
};

typedef BACKEND StorageBackend;

#endif // BACKENDS_H

