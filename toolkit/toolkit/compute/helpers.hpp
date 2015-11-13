#ifndef HELPERS_H
#define HELPERS_H

#include <string>
using namespace std;

void log_time_diff(string msg, unsigned long long ticks);
double ticks_to_seconds(unsigned long long ticks);
unsigned long long getticks();
int get_next_pow_2(unsigned int v);
int hours_to_samples(int fs, float time);
float samples_to_hours(int fs, int samples);
#endif // HELPERS_H

