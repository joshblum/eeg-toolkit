#ifndef HELPERS_H
#define HELPERS_H

#include <sys/time.h>
#include <sys/stat.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <armadillo>


using namespace std;
using namespace arma;

/*
 * Get a timestamp
 */
static inline unsigned long long getticks()
{
  struct timeval t;
  gettimeofday(&t, 0);
  return t.tv_sec * 1000000ULL + t.tv_usec;
}

/*
 * Convert the ticks timestamp to seconds
 */
static inline double ticks_to_seconds(unsigned long long ticks)
{
  return ticks * 1.0e-6;
}

/*
 * Print the time diff in seconds
 */
static inline void log_time_diff(string msg, unsigned long long ticks)
{
  double diff_secs = ticks_to_seconds(getticks() - ticks);
  cout << msg << " took " << setprecision(2) << diff_secs << " seconds" << endl;
}

/*
 * Return the next power of 2 greater than `v`
 */
static inline int get_next_pow_2(unsigned int v)
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

/*
 * Given the sampling rate `fs` return the number of
 * samples for the given `time` range.
 */
static inline int hours_to_samples(int fs, float time)
{
  return fs * 60 * 60 * time;
}

/*
 * Return the number of hours for the number of `samples`
 * given and the given sampling rate `fs`
 */
static inline float samples_to_hours(int fs, int samples)
{
  return samples / (fs * 60.0 * 60.0);
}

/*
 * Return the length of the string aligned to 8 bytes
 * with an extra 4 bytes to store the length in a uint32_t
 */
static inline uint32_t get_byte_aligned_length(string str)
{
  return str.size() + (8 - ((str.size() + 4) % 8));
}

/*
 * Determine if a file exists at the given `path`
 */
static inline bool file_exists(string path)
{
  struct stat buffer;
  return stat(path.c_str(), &buffer) == 0;
}

/*
 * Remove every `extent` column from the given matrix.
 * If `extent` is `0` or `1` the matrix is unchanged
 */
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

/*
 * Given the maximum width `max_width`, downsample
 * the given matrix to not exceed this width.
 */
static inline void cap_max_width(fmat& buf, int max_width)
{
  uint extent = ceil(buf.n_cols / (float) max_width);
  downsample(buf, extent);
}

#endif // HELPERS_H

