#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include "queue.c"
#include "pthread_sleep_v2.c"

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

struct timeval tv;
struct timezone tz;
struct tm *today;
int zone;

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
// create moderator thread
pthread_t moderator_thread;
//create main thread
pthread_t main_thread;
pthread_t *speaker_threads;

struct SpeakerStruct
{
    int taskid;
    pthread_cond_t cond;
};

void my_print(str)
{
    struct timeval tv;
    struct timezone tz;
    struct tm *today;
    int zone;

    gettimeofday(&tv, &tz);

    today = localtime(&tv.tv_sec);
    printf("It's %d:%0d:%0d.%03d\n",
           today->tm_hour,
           today->tm_min,
           today->tm_sec,
           tv.tv_usec);

    // printf("[%0d:%0d.%03d]\t%s\n",
    //         today->tm_min,
    //         today->tm_sec,
    //         tv.tv_usec,str);
}

void get_time()
{
    gettimeofday(&tv, &tz);

    today = localtime(&tv.tv_sec);
    // char* s = "[";
    // strcat(s, today->tm_min);
    // strcat(s, )
    // return "[%0d:%0d.%03d]\t",
    //     today->tm_min,
    //     today->tm_sec,
    //     tv.tv_usec;
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

    printf("Hello, I am Commentator #%d, glad to be here.\n", taskid);
    while (current_q< q)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&questionAsked, &mutex);

        if ((rand() % 1000) * 0.001 <= p)
        // if (1 == 1)
        {
            float t_speak = rand() % t + (rand() % 1000) * 0.001;
            enqueue(queue, taskid);
            count++;

            get_time();
            printf("[%0d:%0d.%03d]\tCommentator #%d wants to answer, position in queue %d\n", today->tm_min,
                   today->tm_sec,
                   tv.tv_usec, taskid, queue->length - 1);
            pthread_cond_wait(&answer[taskid], &mutex);

            get_time();
            printf("[%0d:%0d.%03d]\t%d: I will be speaking for %.3f seconds.\n", today->tm_min,
                   today->tm_sec,
                   tv.tv_usec, taskid, t_speak);

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
                    get_time();
                    if (t_speak > 0)
                        printf("[%0d:%0d.%03d]\t%d: I stopped %.3f seconds before my end time due to breaking event\n", today->tm_min,
                               today->tm_sec,
                               tv.tv_usec, taskid, t_speak);
                    pthread_mutex_lock(&breaking_mutex);

                    get_time();
                    printf("[%0d:%0d.%03d]\t%d: I am waiting for the breaking event\n", today->tm_min,
                           today->tm_sec,
                           tv.tv_usec, taskid);
                    pthread_cond_wait(&breaking, &breaking_mutex);
                    t_speak = 0;
                    pthread_mutex_unlock(&breaking_mutex);
                }
            }
            pthread_cond_signal(&next);
        }
        else
        {
            count++;
        }
        pthread_mutex_unlock(&mutex);
    }
    // printf("commentator %d  çıktım", taskid);
}

void *Moderator()
{
    while (current_q<= q)
    {
        pthread_mutex_lock(&mutex);

        if (count == n && isEmpty(queue) && current_q < q)
        {
            current_q++;

            get_time();
            printf("[%0d:%0d.%03d]\tModerator asks question %d\n", today->tm_min,
                   today->tm_sec,
                   tv.tv_usec, current_q);
            pthread_cond_broadcast(&questionAsked);
            // pthread_cond_wait(&allDone, &mutex);
            // printf("All commentators done.\n");
            count = 0;
        }
        else if (isEmpty(queue) && current_q >= q && count==n)
        {
            current_q ++;
            printf("End of sessions.\n");
            pthread_mutex_unlock(&mutex);

            // pthread_exit(NULL);
            break;
        }
        else if (isEmpty(queue))
        {
        }
        else
        {
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
    while (current_q<= q)
    {
        pthread_sleep_old(1);
        // pthread_mutex_lock(&breaking_mutex);
        // check breaking events every 1 second
        if ((rand() % 1000) * 0.001 <= b)
        {
            get_time();
            printf("[%0d:%0d.%03d]\tJust in!!\n", today->tm_min,
                   today->tm_sec,
                   tv.tv_usec);
            breaking_event = 1;

            pthread_sleep_old(5);
            get_time();
            printf("[%0d:%0d.%03d]\tWow, at least that's over!\n", today->tm_min,
                   today->tm_sec,
                   tv.tv_usec);
            pthread_cond_signal(&breaking);
        }
        breaking_event = 0;
        // pthread_mutex_unlock(&breaking_mutex);
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
    int *taskids[n];
    speaker_threads = malloc(sizeof(pthread_t) * n);

    // create speaker threads
    for (ind = 0; ind < n; ind++)
    {
        taskids[ind] = (int *)malloc(sizeof(int));
        *taskids[ind] = ind;
        rc = pthread_create(&speaker_threads[ind], NULL, Speaker, (void *)taskids[ind]);

        if (rc)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
    int main_thread_int = pthread_create(&main_thread, NULL, MainThread, NULL);
    int moderator = pthread_create(&moderator_thread, NULL, Moderator, NULL);

    for (ind = 0; ind < n; ind++)
    {
        pthread_join(speaker_threads[ind], NULL);
    }
    pthread_join(moderator_thread, NULL);
    pthread_join(main_thread, NULL);
    pthread_exit(NULL);
}
