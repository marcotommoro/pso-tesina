#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#include "../utils/utils.h"

#define WAIT 0
#define PROCESS 1

typedef struct Node
{
    int num;
    char action;
    struct Node *next;
};

typedef struct
{
    pthread_mutex_t read_mutex, sum_mutex, odd_mutex, min_mutex, max_mutex;
    FILE *fp;
    int sum, min, n_odd, max;
    int is_end;
    struct Node *linked_list;
} Master;

Master m;

void init_linked_list()
{
    struct Node *head;
    struct Node *last;
    last = head = NULL;

    int res;
    char action;
    int num;

    while (fscanf(m.fp, "%c   %d\n", &action, &num) == 2)
    {
        if (head == NULL)
        {
            head = last = (struct Node *)malloc(sizeof(struct Node));
        }
        else
        {
            last->next = (struct Node *)malloc(sizeof(struct Node));
            last = last->next;
        }

        last->action = action;
        last->num = num;
        last->next = NULL;
    }

    m.linked_list = head;
}

void __init__(char *filename)
{
    pthread_mutexattr_t m_attr;
    pthread_mutexattr_init(&m_attr);
    pthread_mutex_init(&m.read_mutex, &m_attr);
    pthread_mutex_init(&m.sum_mutex, &m_attr);
    pthread_mutex_init(&m.odd_mutex, &m_attr);
    pthread_mutex_init(&m.min_mutex, &m_attr);
    pthread_mutex_init(&m.max_mutex, &m_attr);

    m.sum = m.n_odd = m.is_end = 0;
    m.min = INT_MAX;
    m.max = INT_MIN;

    m.fp = fopen(filename, "r");

    if (!m.fp)
    {
        printf("ERROR: could not open %s\n", filename);
        exit(EXIT_FAILURE);
    }
    init_linked_list();
}

int get_next_element(int arr[])
{
    // Optimization, not blocking
    if (m.is_end)
    {
        return -1;
    }

    pthread_mutex_lock(&m.read_mutex);

    // Exit if list is ended
    if (m.linked_list == NULL)
    {
        m.is_end = 1;
        pthread_mutex_unlock(&m.read_mutex);

        return -1;
    }

    arr[1] = m.linked_list->num;

    if (m.linked_list->action == 'p')
    {
        arr[0] = PROCESS;
    }
    else if (m.linked_list->action == 'w')
    {
        arr[0] = WAIT;
    }
    else
    {
        printf("ERROR: Unrecognized action: '%c'\n", m.linked_list->action);
        exit(EXIT_FAILURE);
    }

    // set next element in the list
    m.linked_list = m.linked_list->next;

    pthread_mutex_unlock(&m.read_mutex);

    return 0;
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

    while (get_next_element(arr) != -1)
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
}

int main(int argc, char *argv[])
{
    struct timeval t0;
    struct timeval t1;
    float elapsed;

    if (argc != 3)
    {
        printf("2 parameters reqired!\n");
        printf("`./a N_THREAD FILE`\n");
        printf("ex: `./a 10 file.txt`\n");
        exit(NULL);
    }

    const int N_THREAD = atoi(argv[1]) - 1;

    if (N_THREAD < 0)
    {
        printf("N_THREAD must be at least 1.\n");
        exit(NULL);
    }

    if (!file_exists(argv[2]))
        exit(NULL);

    printf("Run app with 1 Master and %d threads workers\n\n", N_THREAD);

    // Take time now - start
    gettimeofday(&t0, 0);

    pthread_t p[N_THREAD];
    pthread_attr_t a;
    pthread_attr_init(&a);

    __init__(argv[2]);

    for (int i = 0; i < N_THREAD; i++)
        pthread_create(&p[i], &a, thread, NULL);

    // Execute master thread
    thread(NULL);

    for (int i = 0; i < N_THREAD; i++)
        pthread_join(p[i], NULL);

    // Take time now - end
    gettimeofday(&t1, 0);
    elapsed = time_difference_msec(t0, t1);

    printf("TIME: %fms\nSUM: %d\n#ODD: %d\nMIN: %d\nMAX: %d\n", elapsed, m.sum, m.n_odd, m.min, m.max);
    return 0;
}
