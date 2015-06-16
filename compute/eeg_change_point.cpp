#include "eeg_change_point.hpp"
#define NUM_SAMPLES 10
using namespace arma;


// TODO(joshblum): make real variable names
void get_change_points(mat& spec_mat,
                       cp_data_t* cp_data)
{
  rowvec s = sum(spec_mat, 0); // sum rows

  int max_val = 5000;
  int stride = 10;
  int nt = s.n_rows;

  float b = 0.95;
  float bb = 0.995;
  int max_amp = 3000;

  // CUMSUM parameters
  int sigma = 100;
  int K = 4 * sigma / 2;
  int h = 5;
  int H = h * sigma;

  rowvec m = zeros<rowvec>(nt);
  m(0) = 1000;
  rowvec mu = zeros<rowvec>(nt);
  mu(0) = 1000;

  rowvec cu = zeros<rowvec>(nt);
  rowvec cl = zeros<rowvec>(nt);

  rowvec cp = rowvec(nt);

  int ct = 0;
  int total_count = 0;

  float np = 0.;
  float nm = 0.;

  for (int j = 1; j < nt; j++)
  {
    ct++;
    double s_j = s[j * stride] > max_val ? max_val : s[j * stride];
    //printf("s_j: %f\n", s_j);

    // signal we are tracking -- short term mean
    m[j] = fmin(m[j - 1] * b + (1 - b) * s_j, max_amp);
    if (ct < 20)
    {
      mu[j] = fmin(mu[j - 1] * bb + (1 - bb) * m[j], max_amp);
    }
    else
    {
      mu[j] = mu[j - 1];
    }

    cu[j] = fmax(0, cu[j - 1] + m[j] - mu[j] - K);
    cl[j] = fmax(0, cl[j - 1] + mu[j] - m[j] - K);

    np = (np + 1) * (cu[j] > 0);
    nm = (nm + 1) * (cl[j] > 0);

    if (cu[j] > H || cl[j] > H)
    {
      total_count++;
      cp[j] = j * stride; // time of change point
      ct = 0;

      if (cu[j] > H)
      {
        mu[j] = mu[j] + K + cu[j] / np;
        cu[j] = 0.;
        np  = 0;
      }
      if (cl[j] > H)
      {
        mu[j] = mu[j] - K - cl[j] / nm;
        cl[j] = 0.;
        nm = 0.;
      }
    }
  }
  cp.head(total_count);
  rowvec yp = rowvec(total_count);
  yp.fill(max_amp);

  cp_data->cp = &cp;
  cp_data->yp = &yp;
  cp_data->cu = &cu;
  cp_data->cl = &cl;
  cp_data->mu = &mu;
  cp_data->m = &m;
  cp_data->total_count = total_count;
}
