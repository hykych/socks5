#ifndef _THREAD_H
#define _THREAD_H


#include <pthread.h>
#include "../inc/def.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#define TID_T    pthread_t


typedef pthread_mutex_t CS_T;

INT32 CS_INIT(CS_T *cs);

INT32 CS_ENTER(CS_T *cs);

INT32 CS_LEAVE(CS_T *cs);

TID_T THREAD_CREATE(void *(*func)(void *), void* param);

#endif
