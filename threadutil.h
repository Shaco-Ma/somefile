#ifndef _THREAD_UTIL_H_
#define _THREAD_UTIL_H_

#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>
#include "rxdType.h"


typedef struct
{
    pthread_cond_t      condition;  
    pthread_mutex_t     lock;
    u8                  conditionVal;         //取自pCheckCondition 返回某个时刻的状态 
    u8                (*pCheckCondition)();   //函数指针，条件函数，若为真，条件变量触发。
} ThreadUtilCondition_t;

__BEGIN_DECLS
//  初始化线程条件变量,pCheckCondition 表示条件函数，
//  返回0 成功。
int ThreadUtil_initCondition(ThreadUtilCondition_t * pCondition, u8  (*pCheckCondition)());
//  睡眠，等待条件变量成立，否则等待到超时时间 timeout（ms）
//  若条件变量为真，返回非0, 否则返回0
int ThreadUtil_waitCondition(ThreadUtilCondition_t * pCondition, int timeout);

//注意，以下函数需要成对使用！在改变条件之前，调用start，改变条件后，调用end
void ThreadUtil_startCheckCondition(ThreadUtilCondition_t * pCondition);
void ThreadUtil_endCheckCondition(ThreadUtilCondition_t * pCondition);

void ThreadUtil_DeinitCheckCondition(ThreadUtilCondition_t * pCondition);

__END_DECLS

#endif
