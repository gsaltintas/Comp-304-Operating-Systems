#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include "queue.c"

#define event_duration 5
#define MAX_QUEUE_SIZE 1024

int n = 0;
int t = 0;
float p = 0;
int q = 0;
int current_q = 0;
float b = 0;
struct Queue *queue;
int firstAnswer = 1;
int count;
pthread_cond_t *answer;
int breaking_event = 0; // 0 no event, 1 yes

// Getting the mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t breaking_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t questionAsked =
    PTHREAD_COND_INITIALIZER;
pthread_cond_t questionAnswered =
    PTHREAD_COND_INITIALIZER;
pthread_cond_t allDone = PTHREAD_COND_INITIALIZER;
pthread_cond_t next = PTHREAD_COND_INITIALIZER;
pthread_cond_t breaking = PTHREAD_COND_INITIALIZER;

struct SpeakerStruct
{
    int taskid;
    pthread_cond_t cond;
};

// pthread_cond_t *answer;
/**
 * pthread_sleep takes an integer number of seconds to pause the current thread
 * original by Yingwu Zhu
 * updated by Muhammed Nufail Farooqi
 * updated by Fahrican Kosar
 */
int pthread_sleep_old(double seconds)
{
    pthread_mutex_t sleep_mutex;
    pthread_cond_t conditionvar;
    if (pthread_mutex_init(&sleep_mutex, NULL))
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

    pthread_mutex_lock(&sleep_mutex);
    int res = pthread_cond_timedwait(&conditionvar, &sleep_mutex, &timetoexpire);
    pthread_mutex_unlock(&sleep_mutex);
    pthread_mutex_destroy(&sleep_mutex);
    pthread_cond_destroy(&conditionvar);

    //Upon successful completion, a value of zero shall be returned
    return res;
}

int pthread_sleep(double seconds, pthread_mutex_t *mutex, pthread_cond_t *conditionvar)
{
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
        // printf("heee %c %c %s\n", opt, optopt, optarg);
        switch (opt)
        {
        case 'n':
            n = atoi(optarg);
            break;
        case 'p':
            p = atof(optarg);
            break;
        case 'q':
            q = atoi(optarg);
            break;
        case 't':
            t = atoi(optarg);
            break;
        case 'b':
            b = atof(optarg);
            break;
        case ':':

            break;
        case '?':
            if (opt == 'n' || opt == 'p' || opt == 'q' || opt == 't' || opt == 'b')
                printf('Option %c reqires an argument.', opt);
            else
                printf("Invalid argument option.\n");

            break;
        }
    }
    printf("User chosen parameters are as follows:\n\tn: %d\tp: %f\tq: %d\n", n, p, q);
    printf("\tt: %d\tb: %f\n", t, b);
}

void *Speaker(void *threadid)
{

    int *id_ptr, taskid;
    id_ptr = (int *)threadid;
    taskid = *id_ptr;

    printf("Thread %d: %s\n", taskid, "Hi");
    while (1)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&questionAsked, &mutex);

        printf("%d: Question asked\n", taskid);
        if ((rand() % 1000) * 0.001 <= p)
        // if (1 == 1)
        {
            float t_speak = rand() % t + (rand() % 1000) * 0.001;
            enqueue(queue, taskid);
            count++;
            printf("%d: I wanna answer, %d\n", taskid, queue->length);
            pthread_cond_wait(&answer[taskid], &mutex);

            printf("%d: I am speaking for %.3f seconds.\n", taskid, t_speak);

            while (t_speak > 0)
            {
                float to_sleep;
                if (t_speak > 1)
                    to_sleep = 1;
                else
                    to_sleep = t_speak;
                pthread_sleep_old(to_sleep);
                t_speak -= to_sleep;

                if (breaking_event == 1)
                {
                    if (t_speak > 0)
                        printf("%d: I stopped %.3f seconds before my end time due to breaking event\n", taskid, t_speak);
                    pthread_mutex_lock(&breaking_mutex);
                    pthread_cond_wait(&breaking, &breaking_mutex);
                    t_speak = 0;
                    pthread_mutex_unlock(&breaking_mutex);
                }
            }
            pthread_cond_broadcast(&next);
        }
        else
        {
            count++;
        }
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

void *Moderator()
{
    while (1)
    {
        pthread_mutex_lock(&mutex);

        if (count == n && isEmpty(queue) && current_q < q)
        {
            current_q++;
            printf("Moderator asks question %d\n", current_q);
            pthread_cond_broadcast(&questionAsked);
            // pthread_cond_wait(&allDone, &mutex);
            // printf("All commentators done.\n");
            count = 0;
        }
        else if (isEmpty(queue))
        {
        }
        else
        {
            printf("Moderator in else\n");
            int speaker = dequeue(queue);
            pthread_cond_signal(&answer[speaker]);
            pthread_cond_wait(&next, &mutex);
        }
        // Get the mutex unlocked
        pthread_mutex_unlock(&mutex);
    }
}

void *MainThread()
{
    while (1)
    {
        pthread_sleep_old(1);
        pthread_mutex_lock(&breaking_mutex);
        // check breaking events every 1 second
        if ((rand() % 1000) * 0.001 <= b)
        {
            printf("Generating breaking event\n");
            breaking_event = 1;

            pthread_sleep_old(5);

            printf("Ending breaking event\n");
            pthread_cond_signal(&breaking);
        }
        breaking_event = 0;
        pthread_mutex_unlock(&breaking_mutex);
    }
}

// int main(int argc, char **argv)
int main(int argc, char *argv[])
{
    // Use current time as seed for random generator
    srand(time(0));
    printf("%ld\n", sizeof(queue));
    get_params(argc, argv);
    int rc, ind;

    count = n;

    answer = malloc(sizeof(pthread_cond_t) * n);
    for (ind = 0; ind < n; ind++)
        answer[ind] = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

    // initialize queue
    queue = initialize_queue(MAX_QUEUE_SIZE);
    // create moderator thread
    pthread_t moderator_thread;
    //create main thread
    pthread_t main_thread;

    pthread_t speaker_threads[n];
    int *taskids[n];

    // create speaker threads
    for (ind = 0; ind < n; ind++)
    {
        taskids[ind] = (int *)malloc(sizeof(int));
        *taskids[ind] = ind;
        printf("Creating speaker %d\n", ind);
        rc = pthread_create(&speaker_threads[ind], NULL, Speaker, (void *)taskids[ind]);
        if (rc)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
    int moderator = pthread_create(&moderator_thread, NULL, Moderator, NULL);
    int main_thread_int = pthread_create(&main_thread, NULL, MainThread, NULL);

    for (ind = 0; ind < n; ind++)
    {
        pthread_join(&speaker_threads[ind], NULL);
    }
    pthread_join(moderator_thread, NULL);
    pthread_join(main_thread, NULL);
    pthread_exit(NULL);
}