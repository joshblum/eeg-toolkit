#include "edf_backend.hpp"

// TODO(joshblum): this needs to be a concurrent structure
static edf_hdr_struct* EDF_HDR_CACHE[EDFLIB_MAXFILES];

// should use a hashmap + linked list implementation if the
// list traversal becomes a bottleneck
void print_hdr_cache()
{
  printf("[\n");
  for (int i = 0; i < EDFLIB_MAXFILES; i++)
  {
    if (EDF_HDR_CACHE[i] == NULL)
    {
      printf("-,");
    }
    else
    {
      printf("%s,\n", EDF_HDR_CACHE[i]->path);
    }
  }
  printf("]\n");
}

edf_hdr_struct* get_hdr_cache(char *filename)
{
  for (int i = 0; i < EDFLIB_MAXFILES; i++)
  {
    if (EDF_HDR_CACHE[i] != NULL)
    {
      if (!(strcmp(filename, EDF_HDR_CACHE[i]->path)))
      {
        edf_hdr_struct* hdr = EDF_HDR_CACHE[i];
        // TODO(joshblum): probably remove this once
        // windowing is fully implemented
        for (int signal = 0; signal < hdr->edfsignals; signal++)
        {
          edfrewind(hdr->handle, signal);
        }
        return hdr;
      }
    }
  }

  return NULL;
}

void set_hdr_cache(edf_hdr_struct* hdr)
{
  for (int i = 0; i < EDFLIB_MAXFILES; i++)
  {
    if (EDF_HDR_CACHE[i] == NULL)
    {
      EDF_HDR_CACHE[i] = hdr;
      break;
    }
  }
}

void pop_hdr_cache(const char* filename)
{
  for (int i = 0; i < EDFLIB_MAXFILES; i++)
  {
    if (EDF_HDR_CACHE[i] != NULL)
    {
      if (!(strcmp(filename, EDF_HDR_CACHE[i]->path)))
      {
        free(EDF_HDR_CACHE[i]);
        EDF_HDR_CACHE[i] = NULL;
      }
    }
  }
}

int get_data_len(edf_hdr_struct* hdr)
{
  // assume all signals have a uniform sample rate
  return hdr->signalparam[0].smp_in_file;
}

void load_edf(edf_hdr_struct* hdr, char* mrn)
{
  char* filename = (char*) malloc(sizeof(char)*100);
  mrn_to_filename(mrn, filename);
  edf_hdr_struct* cached_hdr = get_hdr_cache(filename);
  if (cached_hdr != NULL)
  {
    // get the file from the cache
    hdr = cached_hdr;
    return;
  }
  if (edfopen_file_readonly(filename, hdr, EDFLIB_DO_NOT_READ_ANNOTATIONS))
  {
    switch (hdr->filetype)
    {
      case EDFLIB_MALLOC_ERROR                :
        printf("\nmalloc error\n\n");
        break;
      case EDFLIB_NO_SUCH_FILE_OR_DIRECTORY   :
        printf("\ncannot open file, no such file or directory: %s\n\n", filename);
        break;
      case EDFLIB_FILE_CONTAINS_FORMAT_ERRORS :
        printf("\nthe file is not EDF(+) or BDF(+) compliant\n"
            "(it contains format errors)\n\n");
        break;
      case EDFLIB_MAXFILES_REACHED            :
        printf("\nto many files opened\n\n");
        break;
      case EDFLIB_FILE_READ_ERROR             :
        printf("\na read error occurred\n\n");
        break;
      case EDFLIB_FILE_ALREADY_OPENED         :
        printf("\nfile has already been opened\n\n");
        break;
      default                                 :
        printf("\nunknown error\n\n");
        break;
    }
    exit(1);
  }
  // set the file in the cache
  set_hdr_cache(hdr);
}

void close_edf(char* mrn)
{
  char* filename = (char*) malloc(sizeof(char)*100);
  mrn_to_filename(mrn, filename);
  edf_hdr_struct* hdr = get_hdr_cache(filename);
  if (hdr != NULL)
  {
    edfclose_file(hdr->handle);
    pop_hdr_cache(filename);
  }
}

int read_edf_data(int hdl, int ch, int startOffset, int endOffset, float* buf)
{
  if (edfseek(hdl, ch, (long long) startOffset,  EDFSEEK_SET) == -1)
  {
    printf("\nerror: edfseek()\n");
    return -1;
  }

  int nsamples = endOffset - startOffset;
  int bytes_read = edfread_physical_samples(hdl, ch, nsamples, buf);
  if (bytes_read == -1)
  {
    printf("\nerror: edf_read_physical_samples()\n");
    return -1;
  }
  // clear buffer in case we didn't read as much as we expected to
  for (int i = bytes_read; i < nsamples; i++)
  {
    buf[i] = 0.0;
  }
  return bytes_read;
}

/*
 * Transform a medical record number (mrn) to a filename. This
 * should only be used for temporary testing before a real backend is
 * implemented.
 */

void mrn_to_filename(char* mrn, char* filename) {
  char* basedir;
#ifdef __APPLE__
  basedir = "/Users/joshblum/Dropbox (MIT)";
#elif __linux__
  basedir = "/home/ubuntu";
#endif
  sprintf(filename,
      "%s/MIT-EDFs/MIT-CSAIL-%s.edf",
      basedir,
      mrn);
}
