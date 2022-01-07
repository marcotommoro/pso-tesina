#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>

float time_difference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

void msleep(int n)
{
    if (n == NULL)
    {
        n = rand() % 1000 + 100;
    }

    usleep(n * 1000);
}

int file_exists(char *filename)
{
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}
