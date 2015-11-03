#ifndef BACKENDS_H
#define BACKENDS_H

#include <unordered_map>
#include <armadillo>
#include <string>
#include "EDFlib/edflib.h"
#include "../config.hpp"

using namespace std;
using namespace arma;

template <class T>
class AbstractStorageBackend
{
  protected:
    // mrn to array obj pointer
    unordered_map<string, T> data_cache;
    virtual T get_cache(string mrn) = 0;
    virtual void put_cache(string mrn, T ptr) = 0;
    virtual void pop_cache(string mrn) = 0;

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
    edf_hdr_struct* get_cache(string mrn);
    void put_cache(string mrn, edf_hdr_struct* ptr);
    void pop_cache(string mrn);

  public:
    int get_fs(string mrn);
    int get_data_len(string mrn);
    void load_array(string mrn);
    void get_array_data(string mrn, int ch, int start_offset, int end_offset, frowvec& buf);
    void close_array(string mrn);

  private:
    string mrn_to_filename(string mrn);
};

typedef BACKEND StorageBackend;

#endif // BACKENDS_H

