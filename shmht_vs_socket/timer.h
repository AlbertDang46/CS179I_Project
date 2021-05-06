#ifndef __GETTIME_H__
#define __GETTIME_H__

#include <time.h>

void get_monotonic_time(struct timespec* ts) {
    clock_gettime(CLOCK_MONOTONIC, ts);
}

double get_elapsed_time(struct timespec* before, struct timespec* after) {
    double deltat_s  = after->tv_sec - before->tv_sec;
    double deltat_ns = after->tv_nsec - before->tv_nsec;
    return deltat_s + deltat_ns*1e-9;
}

#endif