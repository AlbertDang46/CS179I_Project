#ifndef __GETTIME_H__
#define __GETTIME_H__

#include <time.h>

void get_monotonic_time(struct timespec* ts);
long get_time_nano(struct timespec* ts);
double get_elapsed_time_sec(struct timespec* before, struct timespec* after);
long get_elapsed_time_nano(struct timespec* before, struct timespec* after);

void get_monotonic_time(struct timespec* ts) {
    clock_gettime(CLOCK_MONOTONIC, ts);
}

long get_time_nano(struct timespec* ts) {
    return (long)ts->tv_sec * 1e9 + ts->tv_nsec;
}

double get_elapsed_time_sec(struct timespec* before, struct timespec* after) {
    double deltat_s  = after->tv_sec - before->tv_sec;
    double deltat_ns = after->tv_nsec - before->tv_nsec;
    return deltat_s + deltat_ns*1e-9;
}

long get_elapsed_time_nano(struct timespec* before, struct timespec* after) {
    return get_time_nano(after) - get_time_nano(before);
}

#endif