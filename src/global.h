#ifndef AGR_CONFIG_H
#define AGR_CONFIG_H

// Enable O_NOATIME
#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>

#define MAX(a,b) (a>b ? a : b)

//#define CASE_INSENSITIVE_SEARCH

inline static print_duration (struct timeval * tv1,struct timeval * tv2)
{
    size_t seconds = tv2->tv_sec - tv1->tv_sec;
    long mseconds = tv2->tv_usec - tv1->tv_usec;

    if (tv2->tv_sec > tv1->tv_sec) {
        if (tv1->tv_usec > tv2->tv_usec) {
            seconds--;
        }
        mseconds += 1000000;
    }

    printf ("%ld.%.6ld", seconds, mseconds);
}

inline static int my_tolower (char ch)
{
    return tolower (ch);
}

#define BAILOUT_ERRNO(errno_val,msg,rest...)    \
    printf ("ERROR in %s:%i: %s (%i): " msg "\n", __FILE__,__LINE__, strerror(errno_val), errno_val, ##rest);\
    exit (-1);

#define BAILOUT(msg,rest...)    \
    BAILOUT_ERRNO(errno,msg,##rest);

#endif
