#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include "queue.c"
#include "pthread_sleep_v2.c"
#include "utils.c"

#define event_duration 5
#define MAX_QUEUE_SIZE 1024

struct Queue *queue;
int current_q = 0;
int count;
int breaking_event = 0; // 0 no event, 1 yes

// Getting the mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t breaking_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t questionAsked = PTHREAD_COND_INITIALIZER;
pthread_cond_t *answer;
pthread_cond_t next = PTHREAD_COND_INITIALIZER;
pthread_cond_t breaking = PTHREAD_COND_INITIALIZER;

//  define threads
pthread_t moderator_thread;
pthread_t main_thread;
pthread_t *commentator_threads;

/**
 * Commentator thread, decides if the commentator will speak,
 */
void *Commentator(void *threadid)
{

    int *id_ptr, taskid;
    id_ptr = (int *)threadid;
    taskid = *id_ptr;

    printf("Hello, I am Commentator #%d, glad to be here.\n", taskid);
    // commentator continues until all questions are asked
    while (current_q < q)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&questionAsked, &mutex);

        // speak with probability p
        if ((rand() % 1000) * 0.001 <= p)
        {
            float t_speak = rand() % t + (rand() % 1000) * 0.001;
            enqueue(queue, taskid);
            count++;

            get_time();
            printf("[%0d:%0d.%03d]\tCommentator #%d wants to answer, position in queue %d\n", log_time->tm_min,
                   log_time->tm_sec,
                   tv.tv_usec, taskid, queue->length - 1);
            pthread_cond_wait(&answer[taskid], &mutex);

            get_time();
            printf("[%0d:%0d.%03d]\tCommentator #%d: I will be speaking for %.3f seconds.\n", log_time->tm_min,
                   log_time->tm_sec,
                   tv.tv_usec, taskid, t_speak);

            // as long as commentator has sth to say
            while (t_speak > 0)
            {
                float to_sleep;
                if (t_speak > 1)
                    to_sleep = 1;
                else
                    to_sleep = t_speak;
                pthread_sleep(to_sleep);
                t_speak -= to_sleep;

                if (breaking_event == 1)
                {
                    get_time();
                    if (t_speak > 0)
                        printf("[%0d:%0d.%03d]\tCommentator #%d: I stopped %.3f seconds before my end time due to breaking event\n", log_time->tm_min,
                               log_time->tm_sec,
                               tv.tv_usec, taskid, t_speak);
                    pthread_mutex_lock(&breaking_mutex);

                    // wait for the breaking event to end
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
}

/**
 * Defines moderator thread, runs as long as there are questions left and manages queue
 */
void *Moderator()
{
    while (current_q <= q)
    {
        pthread_mutex_lock(&mutex);

        if (count == n && isEmpty(queue) && current_q < q)
        {
            current_q++;

            get_time();
            printf("[%0d:%0d.%03d]\tModerator asks question %d\n", log_time->tm_min,
                   log_time->tm_sec,
                   tv.tv_usec, current_q);
            pthread_cond_broadcast(&questionAsked);
            count = 0;
        }
        else if (isEmpty(queue) && current_q >= q && count == n)
        {
            current_q++;
            pthread_mutex_unlock(&mutex);
            break;
        }
        else if (isEmpty(queue))
        {
        }
        else
        {
            int commentator = dequeue(queue);
            pthread_cond_signal(&answer[commentator]);
            pthread_cond_wait(&next, &mutex);
        }
        // Get the mutex unlocked
        pthread_mutex_unlock(&mutex);
    }
}

/**
 * MainThread controls breaking events every second.
 */
void *MainThread()
{
    while (current_q <= q)
    {
        pthread_sleep(1);

        // check breaking events every 1 second
        if ((rand() % 1000) * 0.001 <= b)
        {
            get_time();
            printf("[%0d:%0d.%03d]\tJust in!!\n", log_time->tm_min,
                   log_time->tm_sec,
                   tv.tv_usec);
            breaking_event = 1;

            pthread_sleep(5);
            get_time();
            printf("[%0d:%0d.%03d]\tWow, at least that's over!\n", log_time->tm_min,
                   log_time->tm_sec,
                   tv.tv_usec);
            pthread_cond_signal(&breaking);
        }
        breaking_event = 0;
    }
}

int main(int argc, char *argv[])
{
    gettimeofday(&start_tv, &tz);
    start_time = localtime(&start_tv.tv_sec);
    // Use current time as seed for random generator
    srand(time(0));
    get_params(argc, argv);
    int rc, ind;

    count = n;

    answer = malloc(sizeof(pthread_cond_t) * n);
    for (ind = 0; ind < n; ind++)
        answer[ind] = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

    // initialize queue
    queue = initialize_queue(MAX_QUEUE_SIZE);
    int *taskids[n];
    commentator_threads = malloc(sizeof(pthread_t) * n);

    // create commentator threads
    for (ind = 0; ind < n; ind++)
    {
        taskids[ind] = (int *)malloc(sizeof(int));
        *taskids[ind] = ind;
        rc = pthread_create(&commentator_threads[ind], NULL, Commentator, (void *)taskids[ind]);

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
        pthread_join(commentator_threads[ind], NULL);
    }
    pthread_join(moderator_thread, NULL);
    pthread_join(main_thread, NULL);
    pthread_exit(NULL);
}
