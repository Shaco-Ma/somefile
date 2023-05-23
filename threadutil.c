/*
 * @Descripttion: 
 * @version: 
 */
#include <errno.h>
#include "threadutil.h"
#include "log.h"

#define   TAG       "threadUtil"

//  初始化线程条件变量,pCheckCondition 表示条件函数，
int ThreadUtil_initCondition(ThreadUtilCondition_t * pCondition , u8  (*pCheckCondition)())
{
    pthread_condattr_t cattr;
    pthread_condattr_init(&cattr);
    pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);
    int ret = pthread_cond_init(&pCondition->condition, &cattr);
    pthread_condattr_destroy(&cattr);

    if(ret != 0)
    {
        LOGW(TAG ,"pthread_cond_init failed:%d\n",ret);
        return ret;
    }
    ret = pthread_mutex_init(&(pCondition->lock) , NULL);
    if(ret != 0)
    {
        LOGW(TAG ,"pthread_mutex_init failed:%d\n",ret);
        return ret;
    }

    pCondition->conditionVal = 0;
    pCondition->pCheckCondition = pCheckCondition;

    return ret;
}

#define     THREADUTIL_SLEEP_TIME           50

//  睡眠，等待条件变量成立，否则等待到超时时间 timeout（ms）
//  若条件变量为真，返回非0, 否则返回0
int ThreadUtil_waitCondition(ThreadUtilCondition_t * pCondition, int timeout)
{
    u8 condition = 0;

    pthread_mutex_lock(&pCondition->lock);

    if(pCondition->conditionVal)
    {
        //condition = 1;
    }
    else if(timeout > 0)
    {
        struct timespec ts;
        int ret = 0;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        ts.tv_sec = ts.tv_sec + (timeout / 1000L);
        ts.tv_nsec = ts.tv_nsec + (timeout % 1000L) * 1000000L;
        ts.tv_sec += ts.tv_nsec / 1000000000L;
        ts.tv_nsec %= 1000000000L;

        while(!pCondition->conditionVal)
        {
            ret = pthread_cond_timedwait(&pCondition->condition, &pCondition->lock, &ts);
            if (ret == ETIMEDOUT)
            {
                break;
            }
        }
    }

    condition = pCondition->conditionVal;

    pthread_mutex_unlock(&pCondition->lock);

    return  condition;
}

void ThreadUtil_startCheckCondition(ThreadUtilCondition_t * pCondition)
{
}

void ThreadUtil_endCheckCondition(ThreadUtilCondition_t * pCondition)
{
    pthread_mutex_lock(&pCondition->lock);

    //如果条件满足，广播一下
    u8 condition = pCondition->pCheckCondition();
    //LOGV(TAG, "condition=%u\n",condition);

    pCondition->conditionVal = condition;

    if(pCondition->conditionVal)
    {
        pthread_cond_broadcast(&pCondition->condition);
    }

    pthread_mutex_unlock(&pCondition->lock);
}

void ThreadUtil_DeinitCheckCondition(ThreadUtilCondition_t * pCondition)
{
	if(pCondition != NULL)
	{
		pthread_mutex_destroy(&pCondition->lock);
		pthread_cond_destroy(&pCondition->condition);
	}
}


