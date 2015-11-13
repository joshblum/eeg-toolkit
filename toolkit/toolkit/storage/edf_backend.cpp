#include "backends.hpp"

#include <armadillo>
#include <string>

using namespace std;
using namespace arma;

/*
 * Transform a medical record number (mrn) to a filename. This
 * should only be used for temporary testing before a real backend is
 * implemented.
 */
string EDFBackend::mrn_to_array_name(string mrn)
{
    return AbstractStorageBackend::_mrn_to_array_name(mrn, "edf");
}

int EDFBackend::get_fs(string mrn)
{
    edf_hdr_struct* hdr = get_cache(mrn);
    return ((double)hdr->signalparam[0].smp_in_datarecord / (double)hdr->datarecord_duration) * EDFLIB_TIME_DIMENSION;
}

int EDFBackend::get_array_len(string mrn)
{
    edf_hdr_struct* hdr = get_cache(mrn);
    // assume all signals have a uniform sample rate
    return hdr->signalparam[0].smp_in_file;
}

void EDFBackend::open_array(string mrn)
{
    if (in_cache(mrn)) {
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

void EDFBackend::read_array(string mrn, int ch, int startOffset, int endOffset, frowvec& buf)

{
    edf_hdr_struct* hdr = get_cache(mrn);
    int hdl = hdr->handle;
    if (edfseek(hdl, ch, (long long) startOffset,  EDFSEEK_SET) == -1)
    {
        cout << "error: edfseek()" << endl;
    }

    int nsamples = endOffset - startOffset;
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

void EDFBackend::close_array(string mrn)
{
    if (in_cache(mrn))
    {
        edf_hdr_struct* hdr = get_cache(mrn);
        edfclose_file(hdr->handle);
        free(hdr);
        pop_cache(mrn);
    }
}

