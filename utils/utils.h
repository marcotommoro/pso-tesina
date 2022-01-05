#include <stdio.h>
#include <sys/time.h>

float time_difference_msec(struct timeval t0, struct timeval t1);
void msleep(int n);