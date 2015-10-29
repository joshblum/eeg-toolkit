#ifndef HELPERS_H
#define HELPERS_H

using namespace std;

void log_time_diff(std::string msg, unsigned long long ticks);
double ticks_to_seconds(unsigned long long ticks);
unsigned long long getticks();
int get_next_pow_2(unsigned int v);
int hours_to_nsamples(int fs, float time);
#endif // HELPERS_H

