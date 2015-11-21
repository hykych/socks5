#include "libthread.h"

INT32 CS_INIT(CS_T *cs)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE_NP);
    return pthread_mutex_init(cs,&attr);
}


INT32 CS_ENTER(CS_T *cs)
{
    return pthread_mutex_lock(cs);
}



INT32 CS_LEAVE(CS_T *cs)
{
    return pthread_mutex_unlock(cs);
}


TID_T THREAD_CREATE(void *(*func)(void*),void* param)
{
    TID_T tid;
    INT32 ret;
    pthread_attr_t attr;
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&tid,&attr,func,param);
    if(ret == 0)
	return tid;
    else return (TID_T)-1;
}
