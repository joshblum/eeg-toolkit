#ifndef EDF_BACKEND_H
#define EDF_BACKEND_H

#include "EDFlib/edflib.h"

// necessary for the shared lib for python acess
#ifdef __cplusplus
extern "C" {
#endif

void print_hdr_cache();
edf_hdr_struct* get_hdr_cache(const char *filename);
void set_hdr_cache(edf_hdr_struct* hdr);
void pop_hdr_cache(const char* filename);
void load_edf(edf_hdr_struct* hdr, char* mrn);
int read_edf_data(int hdl, int ch, int n, float* buf);
void close_edf(char* filename);
void mrn_to_filename(char* mrn, char* filename);
int get_data_len(edf_hdr_struct* hdr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // EDF_BACKEND_H
