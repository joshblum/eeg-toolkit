#ifndef HELPERS_H
#define HELPERS_H

using namespace std;

void log_time_diff(std::string msg, unsigned long long ticks);
double ticks_to_seconds(unsigned long long ticks);
unsigned long long getticks();

#endif // HELPERS_H
