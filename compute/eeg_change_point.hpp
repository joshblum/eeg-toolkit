#ifndef CHANGE_POINT_H
#define CHANGE_POINT_H
// #define ARMA_NO_DEBUG // enable for no bounds checking
#include <armadillo>

#ifdef __cplusplus
extern "C" {
#endif

using namespace arma;

typedef struct cp_data
{
  frowvec* cp; // name?
  frowvec* yp; // name?
  frowvec* cu; // name?
  frowvec* cl; // name?
  frowvec* mu; // name?
  frowvec* m; // name?
  int total_count;
} cp_data_t;

void get_change_points(fmat& spec_mat, cp_data_t* cp_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // CHANGE_POINT_H
