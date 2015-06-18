#include "eeg_change_point.hpp"

using namespace arma;

void init_cp_data_t(cp_data_t* cp_data, int nt)
{

  frowvec m = zeros<frowvec>(nt);
  m(0) = 1000;
  frowvec mu = zeros<frowvec>(nt);
  mu(0) = 1000;

  frowvec cu = zeros<frowvec>(nt);
  frowvec cl = zeros<frowvec>(nt);

  frowvec cp = zeros<frowvec>(nt);
  frowvec yp = zeros<frowvec>(nt);

  cp_data->cp = cp;
  cp_data->yp = yp;
  cp_data->cu = cu;
  cp_data->cl = cl;
  cp_data->mu = mu;
  cp_data->m = m;
  cp_data->total_count = 0;
}

// TODO(joshblum): make real variable names
void get_change_points(fmat& spec_mat,
    cp_data_t* cp_data)
{
  frowvec s = sum(spec_mat, 0); // sum rows

  int max_val = 5000;
  int nt = s.n_rows;

  int stride = 10;
  float b = 0.95;
  float bb = 0.995;
  int max_amp = 3000;

  // CUMSUM parameters
  int sigma = 100;
  int K = 4 * sigma / 2;
  int h = 5;
  int H = h * sigma;


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
    cp_data->m[j] = fmin(cp_data->m[j - 1] * b + (1 - b) * s_j, max_amp);
    if (ct < 20)
    {
      cp_data->mu[j] = fmin(cp_data->mu[j - 1] * bb + (1 - bb) * cp_data->m[j], max_amp);
    }
    else
    {
      cp_data->mu[j] = cp_data->mu[j - 1];
    }

    cp_data->cu[j] = fmax(0, cp_data->cu[j - 1] + cp_data->m[j] - cp_data->mu[j] - K);
    cp_data->cl[j] = fmax(0, cp_data->cl[j - 1] + cp_data->mu[j] - cp_data->m[j] - K);

    np = (np + 1) * (cp_data->cu[j] > 0);
    nm = (nm + 1) * (cp_data->cl[j] > 0);

    if (cp_data->cu[j] > H || cp_data->cl[j] > H)
    {
      total_count++;
      cp_data->cp[j] = j * stride; // time of change point
      ct = 0;

      if (cp_data->cu[j] > H)
      {
        cp_data->mu[j] = cp_data->mu[j] + K + cp_data->cu[j] / np;
        cp_data->cu[j] = 0.;
        np  = 0;
      }
      if (cp_data->cl[j] > H)
      {
        cp_data->mu[j] = cp_data->mu[j] - K - cp_data->cl[j] / nm;
        cp_data->cl[j] = 0.;
        nm = 0.;
      }
    }
  }
  // cp_data->cp.head(total_count);
  // cp_data->yp.head(total_count);
  cp_data->yp.fill(max_amp);
  cp_data->total_count = total_count;

}
