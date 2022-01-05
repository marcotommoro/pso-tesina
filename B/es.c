#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include "../utils/utils.h"

#define N_THREAD 100
#define N_IMMIGRANTS 50
#define DELAY_OPERATIONS_MSEC 500

typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t priv_enter_immigrant, priv_leave_immigrant, priv_swear_immigrant, priv_get_certificate_immigrant, priv_judge_enter, priv_judge_check, priv_spectator;
    int immigrants_in, immigrants_checked_in, blocked_immigrants, to_leave_immigrants, blocked_judge, spectators_in, is_judge_in, total_certs;
} Master;

Master m;

void __init__()
{
    pthread_mutexattr_t m_attr;
    pthread_condattr_t c_attr;

    pthread_mutexattr_init(&m_attr);
    pthread_condattr_init(&c_attr);

    pthread_mutex_init(&m.mutex, &m_attr);

    pthread_cond_init(&m.priv_enter_immigrant, &c_attr);
    pthread_cond_init(&m.priv_leave_immigrant, &c_attr);
    pthread_cond_init(&m.priv_swear_immigrant, &c_attr);
    pthread_cond_init(&m.priv_get_certificate_immigrant, &c_attr);
    pthread_cond_init(&m.priv_judge_enter, &c_attr);
    pthread_cond_init(&m.priv_judge_check, &c_attr);
    pthread_cond_init(&m.priv_spectator, &c_attr);

    m.immigrants_checked_in = m.immigrants_in = m.blocked_immigrants = m.blocked_judge = m.spectators_in = m.is_judge_in = m.to_leave_immigrants = m.total_certs = 0;
}

// Block immigrant semaphore
void enter_immigrant(int ID)
{
    pthread_mutex_lock(&m.mutex);

    // Immigrant cannot enter until judge is in
    while (m.is_judge_in)
    {
        m.blocked_immigrants++;
        pthread_cond_wait(&m.priv_enter_immigrant, &m.mutex);
        m.blocked_immigrants--;
    }

    printf("%d) Enter Immigrant...\n", ID);

    m.immigrants_in++;

    pthread_mutex_unlock(&m.mutex);
}

// Release immigrant semaphore
void leave_immigrant(int ID)
{
    pthread_mutex_lock(&m.mutex);

    m.immigrants_in--;
    m.immigrants_checked_in--;

    // `to_leave_migrants` are them that have been completed all steps
    // but didn't get the confirmation by de judge,
    // so they have to leave and enter again the building.
    //
    // It's a significant variable, because judge cannot re-enter
    // until all immigrants have left the building.
    if (m.to_leave_immigrants)
    {
        // The last, free the blocked judge
        // (if is blocked....)
        if (m.to_leave_immigrants == 1)
            pthread_cond_signal(&m.priv_judge_enter);

        m.to_leave_immigrants--;
    }

    printf("%d) Leave Immigrant... immigrants_in: %d to_leave: %d\n", ID, m.immigrants_in, m.to_leave_immigrants);

    // If others immigrants are blocked, free them
    if (m.blocked_immigrants)
        pthread_cond_signal(&m.priv_enter_immigrant);

    pthread_mutex_unlock(&m.mutex);
}

// Block spectator semaphore
void enter_spectator()
{
    pthread_mutex_lock(&m.mutex);

    // Spectator cannot enter until judge is in
    while (m.is_judge_in)
    {
        pthread_cond_wait(&m.priv_spectator, &m.mutex);
    }

    m.spectators_in++;

    pthread_mutex_unlock(&m.mutex);
}

// Release spectator semaphore
void leave_spectator()
{
    pthread_mutex_lock(&m.mutex);

    m.spectators_in--;

    pthread_mutex_unlock(&m.mutex);
}

// Block judge semaphore
void enter_judge()
{
    pthread_mutex_lock(&m.mutex);

    while (m.to_leave_immigrants)
    {
        pthread_cond_wait(&m.priv_judge_enter, &m.mutex);
    }

    printf("ENTER JUDGE!\n");
    m.is_judge_in = 1;

    pthread_cond_broadcast(&m.priv_swear_immigrant);

    pthread_mutex_unlock(&m.mutex);
}

// Release judge semaphore
void leave_judge()
{
    pthread_mutex_lock(&m.mutex);

    m.is_judge_in = 0;

    printf("LEAVE JUDGE!\n");

    pthread_cond_broadcast(&m.priv_get_certificate_immigrant);
    pthread_cond_broadcast(&m.priv_enter_immigrant);
    pthread_cond_broadcast(&m.priv_spectator);

    m.to_leave_immigrants = m.immigrants_in;

    pthread_mutex_unlock(&m.mutex);
}

// Immigrant check-in
void check_in(int ID)
{
    pthread_mutex_lock(&m.mutex);
    printf("%d) Init check in...\n", ID);

    msleep(NULL);

    printf("%d) Finit check in...\n", ID);

    m.immigrants_checked_in++;

    // If the judge is in the building,
    // and all immigrants have done the check-in
    // the judge can start his job,
    // otherwise no.
    if (m.immigrants_checked_in == m.immigrants_in && m.is_judge_in)
        pthread_cond_signal(&m.priv_judge_check);

    pthread_mutex_unlock(&m.mutex);
}

// Immigrant sit down
void sit_down(int ID)
{
    printf("%d) Init sit down...\n", ID);

    msleep(NULL);

    printf("%d) Finit sit down...\n", ID);
}

// Immigrant swear
void swear(int ID)
{
    pthread_mutex_lock(&m.mutex);

    while (!m.is_judge_in)
    {
        printf("waiting for judge enter...\n");
        pthread_cond_wait(&m.priv_swear_immigrant, &m.mutex);
    }

    printf("%d) Init swear...\n", ID);
    msleep(NULL);
    printf("%d) Finit swear...\n", ID);

    pthread_mutex_unlock(&m.mutex);
}

void confirm()
{
    pthread_mutex_lock(&m.mutex);
    printf("Confirm...\n");

    while (m.immigrants_in != m.immigrants_checked_in)
    {
        pthread_cond_wait(&m.priv_judge_check, &m.mutex);
    }

    pthread_cond_signal(&m.priv_get_certificate_immigrant);

    pthread_mutex_unlock(&m.mutex);
}

int get_certificate(int ID)
{
    pthread_mutex_lock(&m.mutex);

    pthread_cond_wait(&m.priv_get_certificate_immigrant, &m.mutex);

    if (m.is_judge_in)
    {
        printf("%d) Got CERTIFICATE!\n", ID);
        m.total_certs++;
        pthread_mutex_unlock(&m.mutex);
        return 1;
    }
    else
    {
        pthread_mutex_unlock(&m.mutex);
        return 0;
    }
}

void spectate(int ID)
{
    printf("%d) Spectating...\n", ID);
    msleep(NULL);
}

void *immigrant(void *arg)
{
    int ID = (int *)arg;
    int cert_ok = 0;

    while (!cert_ok)
    {
        msleep(NULL);
        enter_immigrant(ID);

        check_in(ID);
        sit_down(ID);
        swear(ID);
        cert_ok = get_certificate(ID);

        leave_immigrant(ID);
    }
    pthread_exit(NULL);
}

void *judge()
{

    int i;

    while (m.total_certs != N_IMMIGRANTS)
    {
        msleep(2000);
        enter_judge();
        msleep(2000);

        for (i = 0; i < m.immigrants_in; i++)
        {
            confirm();

            if (rand() % 10 == 5)
                break;
        }
        msleep(2000);

        leave_judge();
    }
    pthread_exit(NULL);
}

void *spectator(void *arg)
{
    int ID = (int *)arg;

    enter_spectator();
    spectate(ID);
    leave_spectator();
    pthread_exit(NULL);
}

int main()
{

    struct timeval t0;
    struct timeval t1;
    float elapsed;
    int i;

    time_t start = time(0);

    gettimeofday(&t0, 0);

    printf("Run app with %d Threads\n\n", N_THREAD);

    srand(time(NULL));

    pthread_t p[N_THREAD];

    char c;

    __init__();

    pthread_attr_t a;
    pthread_attr_init(&a);

    pthread_create(&p[0], &a, judge, NULL);

    for (i = 1; i < N_IMMIGRANTS + 1; i++)
        pthread_create(&p[i], &a, immigrant, (void *)i);

    for (i = N_IMMIGRANTS + 1; i < N_THREAD + 1; i++)
        pthread_create(&p[i], &a, spectator, (void *)i);

    for (i = 0; i < N_THREAD; i++)
        pthread_join(p[i], NULL);

    gettimeofday(&t1, 0);
    elapsed = time_difference_msec(t0, t1);

    printf("TIME: %fms\n", elapsed);
    printf("TOTAL CERTIFICATES: %d\n", m.total_certs);

    return 0;
}