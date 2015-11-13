#include <sys/time.h>
#include <iostream>
#include <iomanip>
#include <string>

#include "helpers.hpp"

using namespace std;

unsigned long long getticks()
{
  struct timeval t;
  gettimeofday(&t, 0);
  return t.tv_sec * 1000000ULL + t.tv_usec;
}

double ticks_to_seconds(unsigned long long ticks)
{
  return ticks * 1.0e-6;
}

void log_time_diff(string msg, unsigned long long ticks)
{
  double diff_secs = ticks_to_seconds(getticks() - ticks);
  cout << msg << " took " << setprecision(2) << diff_secs << " seconds" << endl;
}

int hours_to_samples(int fs, float time)
{
  return fs * 60 * 60 * time;
}

float samples_to_hours(int fs, int samples)
{
  return samples / (fs * 60.0 * 60.0);
}
