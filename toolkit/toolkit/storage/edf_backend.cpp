#include "backends.hpp"
#include <exception>

#include <armadillo>
#include <string>

using namespace std;
using namespace arma;

string EDFBackend::mrn_to_array_name(string mrn)
{
  return _mrn_to_array_name(mrn, ".edf");
}

ArrayMetadata EDFBackend::get_array_metadata(string mrn)
{
    if (is_cached_array(mrn))
    {
      throw NotImplementedError();
    }
    edf_hdr_struct* hdr = get_cache(mrn);
    int fs = ((double)hdr->signalparam[0].smp_in_datarecord / (double)hdr->datarecord_duration) * EDFLIB_TIME_DIMENSION;
    int nsamples = hdr->signalparam[0].smp_in_file;
    int nrows = nsamples;
    int ncols = NCHANNELS;
  return ArrayMetadata(fs, nsamples, nrows, ncols);
}

void EDFBackend::create_array(string mrn, ArrayMetadata* metadata)
{
  throw NotImplementedError();
}

void EDFBackend::open_array(string mrn)
{
    if (is_cached_array(mrn))
    {
      throw NotImplementedError();
    }

    if (!in_cache(mrn)) {
        edf_hdr_struct* hdr = (edf_hdr_struct*) malloc(sizeof(edf_hdr_struct));
        string filename = mrn_to_array_name(mrn);
        if (edfopen_file_readonly(filename.c_str(), hdr, EDFLIB_DO_NOT_READ_ANNOTATIONS))
        {
            switch (hdr->filetype)
            {
            case EDFLIB_MALLOC_ERROR:
                cout << "malloc error" << endl;
                break;
            case EDFLIB_NO_SUCH_FILE_OR_DIRECTORY:
                cout << "cannot open file, no such file or directory:" << filename << endl;
                break;
            case EDFLIB_FILE_CONTAINS_FORMAT_ERRORS:
                cout << "the file is not EDF(+) or BDF(+) compliant" << endl;
                break;
            case EDFLIB_MAXFILES_REACHED:
                cout << "to many files opened" << endl;
                break;
            case EDFLIB_FILE_READ_ERROR:
                cout << "a read error occurred" << endl;
                break;
            case EDFLIB_FILE_ALREADY_OPENED:
                cout << "file has already been opened" << endl;
                break;
            default:
                cout << "unknown error" << endl;
                break;
            }
        }
        put_cache(mrn, hdr);
    }
}

void EDFBackend::read_array(string mrn, int ch, int start_offset, int end_offset, frowvec& buf)

{
    if (is_cached_array(mrn))
    {
      throw NotImplementedError();
    }
    edf_hdr_struct* hdr = get_cache(mrn);
    int hdl = hdr->handle;
    if (edfseek(hdl, ch, (long long) start_offset,  EDFSEEK_SET) == -1)
    {
        cout << "error: edfseek()" << endl;
    }

    int nsamples = end_offset - start_offset;
    int bytes_read = edfread_physical_samples(hdl, ch, nsamples, buf.memptr());

    if (bytes_read == -1)
    {
        cout << "error: edf_read_physical_samples()" << endl;
    }

    // clear buffer in case we didn't read as much as we expected to
    for (int i = bytes_read; i < nsamples; i++)
    {
        buf(i) = 0.0;
    }
}

void EDFBackend::read_array(string mrn, int start_offset, int end_offset, fmat& buf)
{
  throw NotImplementedError();
}

void EDFBackend::write_array(string mrn, int ch, int start_offset, int end_offset, fmat& buf)
{
  throw NotImplementedError();
}

void EDFBackend::close_array(string mrn)
{
    if (is_cached_array(mrn))
    {
      throw NotImplementedError();
    }

    if (in_cache(mrn))
    {
        edf_hdr_struct* hdr = get_cache(mrn);
        edfclose_file(hdr->handle);
        free(hdr);
        pop_cache(mrn);
    }
}

