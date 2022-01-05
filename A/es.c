#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../utils/utils.h"

#define N_THREAD 20
#define FILENAME "file.txt"
#define WAIT 0
#define PROCESS 1

typedef struct
{
    pthread_mutex_t read_mutex, sum_mutex, odd_mutex, min_mutex, max_mutex;
    FILE *fp;
    int sum, min, n_odd, max;
    int is_end;
} Master;

Master m;

void __init__()
{
    pthread_mutexattr_t m_attr;
    pthread_mutexattr_init(&m_attr);
    pthread_mutex_init(&m.read_mutex, &m_attr);
    pthread_mutex_init(&m.sum_mutex, &m_attr);
    pthread_mutex_init(&m.odd_mutex, &m_attr);
    pthread_mutex_init(&m.min_mutex, &m_attr);
    pthread_mutex_init(&m.max_mutex, &m_attr);

    m.sum = m.min = m.n_odd = m.max = m.is_end = 0;

    m.fp = fopen(FILENAME, "r");

    if (!m.fp)
    {
        printf("ERROR: could not open %s\n", FILENAME);
        exit(EXIT_FAILURE);
    }
}

int read_char(int arr[])
{
    // Optimization
    if (m.is_end)
        return -1;

    char action;
    int num;

    pthread_mutex_lock(&m.read_mutex);
    int res = fscanf(m.fp, "%c   %d\n", &action, &num);
    pthread_mutex_unlock(&m.read_mutex);

    if (res != 2)
    {
        m.is_end = 1;
        return -1;
    }

    arr[1] = num;

    if (action == 'p')
    {
        arr[0] = PROCESS;
    }
    else if (action == 'w')
    {
        arr[0] = WAIT;
    }
    else
    {
        printf("ERROR: Unrecognized action: '%c'\n", action);
        exit(EXIT_FAILURE);
    }
    return 0;
}

void sum(int n)
{
    pthread_mutex_lock(&m.sum_mutex);
    m.sum += n;
    pthread_mutex_unlock(&m.sum_mutex);
}

void min(int n)
{
    pthread_mutex_lock(&m.min_mutex);
    if (n < m.min)
    {
        m.min = n;
    }
    pthread_mutex_unlock(&m.min_mutex);
}

void odd(int n)
{
    if ((n & 1) == 1)
    {
        pthread_mutex_lock(&m.odd_mutex);
        m.n_odd++;
        pthread_mutex_unlock(&m.odd_mutex);
    }
}

void max(int n)
{
    pthread_mutex_lock(&m.max_mutex);
    if (n > m.max)
    {
        m.max = n;
    }
    pthread_mutex_unlock(&m.max_mutex);
}

void *thread(void *arg)
{
    int arr[2];

    while (read_char(arr) != -1)
    {
        int n = arr[1];
        if (arr[0] == PROCESS)
        {
            sum(n);
            odd(n);
            min(n);
            max(n);
        }
        else
        {
            // Seconds / 10.
            // For test-only purpose.
            msleep(n * 100);
        }
    }

    pthread_exit(NULL);
}

int main()
{
    printf("Run app with %d Threads\n\n", N_THREAD);

    struct timeval t0;
    struct timeval t1;
    float elapsed;
    pthread_t p[N_THREAD];

    time_t start = time(0);

    gettimeofday(&t0, 0);

    __init__();

    pthread_attr_t a;
    pthread_attr_init(&a);

    for (int i = 0; i < N_THREAD; i++)
        pthread_create(&p[i], &a, thread, NULL);

    for (int i = 0; i < N_THREAD; i++)
        pthread_join(p[i], NULL);

    gettimeofday(&t1, 0);
    elapsed = time_difference_msec(t0, t1);

    printf("TIME: %fms\nSUM: %d\n#ODD: %d\nMIN: %d\nMAX: %d\n", elapsed, m.sum, m.n_odd, m.min, m.max);
    return 0;
}
