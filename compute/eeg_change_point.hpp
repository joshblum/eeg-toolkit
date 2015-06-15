#ifndef CHANGE_POINT_H
#define CHANGE_POINT_H
// #define ARMA_NO_DEBUG // enable for no bounds checking
#include <armadillo>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cp_data
{
  arma::rowvec* cp; // name?
  arma::rowvec* yp; // name?
  arma::rowvec* cu; // name?
  arma::rowvec* cl; // name?
  arma::rowvec* mu; // name?
  arma::rowvec* m; // name?
  int total_count;
} cp_data_t;

int get_nt(float duration, int fs);
void get_change_points(arma::mat& spec_mat, int nt, cp_data_t* cp_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // CHANGE_POINT_H
