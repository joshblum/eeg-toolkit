#ifndef CHANGE_POINT_H
#define CHANGE_POINT_H
// #define ARMA_NO_DEBUG // enable for no bounds checking
#include <armadillo>

// necessary for the shared lib for python acess
#ifdef __cplusplus
extern "C" {
#endif

using namespace arma;

typedef struct cp_data
{
  frowvec cp; // name?
  frowvec yp; // name?
  frowvec cu; // name?
  frowvec cl; // name?
  frowvec mu; // name?
  frowvec m; // name?
  int total_count;
} cp_data_t;

void init_cp_data_t(cp_data_t* cp_data, int nt);
void get_change_points(fmat& spec_mat, cp_data_t* cp_data);
void get_change_points_as_arr(float* spec_arr, int n_rows, int n_cols, cp_data_t* cp_data);
void example_change_points_as_arr(float* spec_arr, int n_rows, int n_cols);
void example_change_points(fmat& spec_mat);
void print_cp_data_t(cp_data_t* cp_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // CHANGE_POINT_H
