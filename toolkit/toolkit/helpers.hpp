#ifndef HELPERS_H
#define HELPERS_H

#include <sys/time.h>
#include <sys/stat.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <armadillo>

#include "helpers.hpp"

using namespace std;
using namespace arma;

static inline unsigned long long getticks()
{
  struct timeval t;
  gettimeofday(&t, 0);
  return t.tv_sec * 1000000ULL + t.tv_usec;
}

static inline double ticks_to_seconds(unsigned long long ticks)
{
  return ticks * 1.0e-6;
}

static inline void log_time_diff(string msg, unsigned long long ticks)
{
  double diff_secs = ticks_to_seconds(getticks() - ticks);
  cout << msg << " took " << setprecision(2) << diff_secs << " seconds" << endl;
}

static inline int hours_to_samples(int fs, float time)
{
  return fs * 60 * 60 * time;
}

static inline float samples_to_hours(int fs, int samples)
{
  return samples / (fs * 60.0 * 60.0);
}

static inline uint32_t get_byte_aligned_length(string header)
{
  return header.size() + (8 - ((header.size() + 4) % 8));
}

static inline bool file_exists(string path)
{
  struct stat buffer;
  return stat(path.c_str(), &buffer) == 0;
}

static inline void downsample(fmat& buf, uint extent)
{
  if (extent > 1) // don't downsample for 0 or 1
  {
    fmat new_buf = fmat(buf.n_rows, buf.n_cols/extent);
    for (uword i = 0; i < new_buf.n_cols; i++)
    {
      new_buf.col(i) = buf.col(i * extent);
    }
    buf = new_buf;
  }
}

static inline void cap_max_width(fmat& buf, int max_width)
{
  uint extent = ceil(buf.n_cols / (float) max_width);
  downsample(buf, extent);
}

#endif // HELPERS_H

