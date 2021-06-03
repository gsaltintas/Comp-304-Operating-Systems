#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

// time structs
struct timeval tv;
struct timeval start_tv;
struct timezone tz;
struct tm *start_time;
struct tm *log_time;

/**
 * update log_time variable
 */
void get_time()
{
    gettimeofday(&tv, &tz);
    struct tm *now;

    now = localtime(&tv.tv_sec);
    struct timeval diff = {tv.tv_sec - start_tv.tv_sec, tv.tv_usec - start_tv.tv_usec};
    log_time = localtime(&diff.tv_sec);
}

// parameters
int n = 0;
int t = 0;
float p = 0;
int q = 0;
float b = 0;
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
