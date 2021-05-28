#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

int n = 0;
int t = 0;
float p = 0;
float q = 0;
float b = 0;

/**
 * pthread_sleep takes an integer number of seconds to pause the current thread
 * original by Yingwu Zhu
 * updated by Muhammed Nufail Farooqi
 * updated by Fahrican Kosar
 */
int pthread_sleep(double seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    if (pthread_mutex_init(&mutex, NULL))
    {
        return -1;
    }
    if (pthread_cond_init(&conditionvar, NULL))
    {
        return -1;
    }

    struct timeval tp;
    struct timespec timetoexpire;
    // When to expire is an absolute time, so get the current time and add
    // it to our delay time
    gettimeofday(&tp, NULL);
    long new_nsec = tp.tv_usec * 1000 + (seconds - (long)seconds) * 1e9;
    timetoexpire.tv_sec = tp.tv_sec + (long)seconds + (new_nsec / (long)1e9);
    timetoexpire.tv_nsec = new_nsec % (long)1e9;

    pthread_mutex_lock(&mutex);
    int res = pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);

    //Upon successful completion, a value of zero shall be returned
    return res;
}
/**
 * parses command line arguments from the user
 *  n, q, p, t, b,
 */
void get_params(int argc, char *argv[])
{
    int counter;
    int opt;
    int opterr = 0;
    while ((opt = getopt(argc, argv, "n:p:q:t:b:")) != -1)
    {
        printf("heee %c %c %s\n", opt, optopt, optarg);
        switch (opt)
        {
        case 'n':
            n = atoi(optarg);
            break;
        case 'p':
            p = atof(optarg);
            break;
        case 'q':
            q = atof(optarg);
            break;
        case 't':
            t = atoi(optarg);
            break;
        case 'b':
            b = atof(optarg);
            break;
        case '?':
            printf("Invalid argument option.\n");
            break;
        case ':':
            printf('Option %c reqires an argument.', optopt);
            break;
        }
    }
    printf("User chosen parameters are as follows:\n\tn: %d\tp: %f\tq: %f\n", n, p, q, );
    printf("\tt: %d\tb: %f\n", t, b);
}

// int main(int argc, char **argv)
int main(int argc, char *argv[])
{
    get_params(argc, argv);
}