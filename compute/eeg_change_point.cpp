#include "eeg_change_point.hpp"

using namespace arma;

static inline int round_up(int num_to_round, int multiple)
{
  return ((num_to_round + multiple - 1) / multiple) * multiple;
}

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

void get_change_points_as_arr(float* spec_arr, int n_rows, int n_cols, cp_data_t* cp_data)
{
  fmat spec_mat = fmat(spec_arr, n_rows, n_cols);
  get_change_points(spec_mat, cp_data);
}

// TODO(joshblum): make real variable names
void get_change_points(fmat& spec_mat,
                       cp_data_t* cp_data)
{
  frowvec s = sum(spec_mat, 0); // sum rows
  int stride = 10;
  int nt = round_up(s.n_cols / stride, 10);
  init_cp_data_t(cp_data, nt);

  int max_val = 5000;

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
      cp_data->cp[total_count] = j * stride; // time of change point
      total_count++;
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
  // TODO(joshblum): ensure we are clipping the array properly here
  // TODO(joshblum): head method missing on ubuntu package?
  // cp_data->cp.head(total_count);
  // cp_data->yp.head(total_count);
  cp_data->yp.fill(max_amp);
  cp_data->total_count = total_count;

}

void print_frowvec(char* name, frowvec vector, int num_samples)
{
  printf("%s: [ ", name);
  for (int i = 0; i < num_samples; i++)
  {
    printf("%.2f, ", vector[i]);
  }
  printf("]\n");
}

void example_change_points_as_arr(float* spec_arr, int n_rows, int n_cols)
{
  cp_data_t cp_data;
  get_change_points_as_arr(spec_arr, n_rows, n_cols, &cp_data);
  print_cp_data_t(&cp_data);
}

void example_change_points(fmat& spec_mat)
{
  cp_data_t cp_data;
  get_change_points(spec_mat, &cp_data);
  print_cp_data_t(&cp_data);
}

void print_cp_data_t(cp_data_t* cp_data)
{
  printf("Total change points found: %d\n", cp_data->total_count);
  print_frowvec("cp", cp_data->cp, cp_data->total_count);
  print_frowvec("yp", cp_data->yp, cp_data->total_count);
  print_frowvec("cu", cp_data->cu, cp_data->total_count);
  print_frowvec("cl", cp_data->cl, cp_data->total_count);
  print_frowvec("mu", cp_data->mu, cp_data->total_count);
  print_frowvec("m", cp_data->m, cp_data->total_count);
}

