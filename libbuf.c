#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "hfile_inc.h"
#include "ringbuf.h"

 #define TAG "libbuf"
/*-----------------------------------------------------------------------------
 *  TypeDef
 *-----------------------------------------------------------------------------*/
struct libBuf_context{
    pthread_mutex_t mutex;
    pthread_cond_t queueFullOrEmptyCV;
    pthread_condattr_t attr;
    ringbuf_t rb;
};

/**********************************************************
 * @brief Reset  libBuf to empty
 *
 * @param size
 *
 * @return
 *********************************************************/
void libBuf_reset(libBuf_context_t libBufCtx)
{
    pthread_mutex_lock(&libBufCtx->mutex);

    ringbuf_reset(libBufCtx->rb);
    pthread_cond_broadcast(&libBufCtx->queueFullOrEmptyCV);

    pthread_mutex_unlock(&libBufCtx->mutex);

}

/**********************************************************
 * @brief Init libBuf
 *
 * @param size
 *
 * @return
 *********************************************************/
libBuf_context_t libBuf_init(uint16_t size) {
    libBuf_context_t libBufCtx = NULL;

    libBufCtx = malloc(sizeof(struct libBuf_context));

    if(libBufCtx != NULL){
	//libBufCtx->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	//libBufCtx->queueFullOrEmptyCV = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	pthread_condattr_init(&libBufCtx->attr);
    	pthread_condattr_setclock(&libBufCtx->attr, CLOCK_MONOTONIC);
  	 pthread_cond_init(&libBufCtx->queueFullOrEmptyCV, &libBufCtx->attr);
	pthread_mutex_init(&libBufCtx->mutex , NULL);
	libBufCtx->rb = ringbuf_new(size);
    }

    return libBufCtx;
}

/*****************
free libBuf_context_t
*****************/
int libBuf_free(libBuf_context_t libBufCtx)
{
	int ret = -1;
	pthread_mutex_lock(&libBufCtx->mutex);
	if(libBufCtx->rb->buf != NULL)
	{
		free(libBufCtx->rb->buf);
		if(libBufCtx->rb != NULL)
		{
			free(libBufCtx->rb);

			if(libBufCtx != NULL)
			{

				pthread_mutex_unlock(&libBufCtx->mutex);
				pthread_condattr_destroy(&libBufCtx->attr);
                pthread_cond_destroy(&libBufCtx->queueFullOrEmptyCV);
                pthread_mutex_destroy(&libBufCtx->mutex);
				free(libBufCtx);
				ret = 0;
			}
			else
			{
				pthread_mutex_unlock(&libBufCtx->mutex);
				ret = -1;
			}
		}
		else
		{

			pthread_mutex_unlock(&libBufCtx->mutex);
			ret = -1;
		}
	}
	else
	{

		pthread_mutex_unlock(&libBufCtx->mutex);
		ret = -1;
	}
	return ret;
}


/**********************************************************
 * @brief Enqueue data  return immediately
 *
 * @param libBufCtx
 * @param data
 * @param len
 *
 * @return
 *********************************************************/

int libBuf_enqueue_try(libBuf_context_t libBufCtx, uint8_t* data, uint16_t len) //不阻塞 ，立刻返回
{
    pthread_mutex_lock(&libBufCtx->mutex);

    if(ringbuf_bytes_free(libBufCtx->rb) < len)
    {
        pthread_mutex_unlock(&libBufCtx->mutex);
        return -1;
    }

    ringbuf_memcpy_into(libBufCtx->rb, data, len);
    pthread_cond_broadcast(&libBufCtx->queueFullOrEmptyCV);
    pthread_mutex_unlock(&libBufCtx->mutex);

    return 0;
}

/**********************************************************
 * @brief Enqueue data  return immediately,force enqueue , pop up  old data ,always success
 *
 * @param libBufCtx
 * @param data
 * @param len
 *
 * @return
 *********************************************************/

int libBuf_enqueue_force(libBuf_context_t libBufCtx, uint8_t* data, uint16_t len) //不阻塞 ，立刻返回
{
    pthread_mutex_lock(&libBufCtx->mutex);

    ringbuf_memcpy_into(libBufCtx->rb, data, len);
    pthread_cond_broadcast(&libBufCtx->queueFullOrEmptyCV);
    pthread_mutex_unlock(&libBufCtx->mutex);

    return 0;
}

/**********************************************************
 * @brief Enqueue data   timeout
 *
 * @param libBufCtx
 * @param data
 * @param len
 *
 * @return
 *********************************************************/
int libBuf_enqueue_timeout(libBuf_context_t libBufCtx, uint8_t* data, uint16_t len , uint16_t timeout_ms)
{
    pthread_mutex_lock(&libBufCtx->mutex);

    //如果总大小根本不够，就不要等了，否则会死锁
    if(ringbuf_capacity(libBufCtx->rb) < len)
    {
        pthread_mutex_unlock(&libBufCtx->mutex);
        LOGE(TAG,"libBuf_enqueue, too big!!!\n");
        return -2;
    }

    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec = ts.tv_sec + (timeout_ms / 1000L);
    ts.tv_nsec = ts.tv_nsec + (timeout_ms % 1000L) * 1000000L;
    ts.tv_sec += ts.tv_nsec / 1000000000L;
    ts.tv_nsec %= 1000000000L;

    while(ringbuf_bytes_free(libBufCtx->rb) < len)
    {
	//pthread_cond_wait(&libBufCtx->queueFullOrEmptyCV, &libBufCtx->mutex);

	    int rc = pthread_cond_timedwait(&libBufCtx->queueFullOrEmptyCV, &libBufCtx->mutex, &ts);
		if (rc == ETIMEDOUT)
		{
			pthread_mutex_unlock(&libBufCtx->mutex);
			LOGE(TAG,"libBuf_enqueue, full\n");
			return -1;
		}
    }
    ringbuf_memcpy_into(libBufCtx->rb, data, len);

    pthread_cond_broadcast(&libBufCtx->queueFullOrEmptyCV);

    pthread_mutex_unlock(&libBufCtx->mutex);

    return 0;
}


/**********************************************************
 * @brief Enqueue data  119 second timeout
 *
 * @param libBufCtx
 * @param data
 * @param len
 *
 * @return
 *********************************************************/
int libBuf_enqueue(libBuf_context_t libBufCtx, uint8_t* data, uint16_t len) {

    pthread_mutex_lock(&libBufCtx->mutex);

    //如果总大小根本不够，就不要等了，否则会死锁
    if(ringbuf_capacity(libBufCtx->rb) < len)
    {
        pthread_mutex_unlock(&libBufCtx->mutex);
        LOGE(TAG,"libBuf_enqueue, too big!!!\n");
        return -2;
    }

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec = ts.tv_sec + 119;

    while(ringbuf_bytes_free(libBufCtx->rb) < len)
    {
	//pthread_cond_wait(&libBufCtx->queueFullOrEmptyCV, &libBufCtx->mutex);

        int rc = pthread_cond_timedwait(&libBufCtx->queueFullOrEmptyCV, &libBufCtx->mutex, &ts);
		if (rc == ETIMEDOUT)
		{
			pthread_mutex_unlock(&libBufCtx->mutex);
			LOGE(TAG,"libBuf_enqueue, full\n");
			return -1;
		}
    }
    ringbuf_memcpy_into(libBufCtx->rb, data, len);

    pthread_cond_broadcast(&libBufCtx->queueFullOrEmptyCV);

    pthread_mutex_unlock(&libBufCtx->mutex);

    return 0;
}

/**********************************************************
 * @brief Dequeue data
 *
 * @param libBufCtx
 * @param data
 * @param len
 *
 * @return
 *********************************************************/
int libBuf_dequeue_timeout(libBuf_context_t libBufCtx, uint8_t* data, uint16_t len , uint16_t timeout_ms)
{
    pthread_mutex_lock(&libBufCtx->mutex);

    //如果总大小根本不够，就不要等了，否则会死锁
    if(ringbuf_capacity(libBufCtx->rb) < len)
    {
        pthread_mutex_unlock(&libBufCtx->mutex);
        LOGE(TAG,"libBuf_dequeue, too big!!!\n");
        return -2;
    }

    //    ringbuf_is_empty(rb1);
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec = ts.tv_sec + (timeout_ms / 1000L);
    ts.tv_nsec = ts.tv_nsec + (timeout_ms % 1000L) * 1000000L;
    ts.tv_sec += ts.tv_nsec / 1000000000L;
    ts.tv_nsec %= 1000000000L;

    while(ringbuf_bytes_used(libBufCtx->rb) < len)
    {
	 //pthread_cond_wait(&libBufCtx->queueFullOrEmptyCV, &libBufCtx->mutex);
	    int rc = pthread_cond_timedwait(&libBufCtx->queueFullOrEmptyCV, &libBufCtx->mutex, &ts);
		if (rc == ETIMEDOUT)
		{
			pthread_mutex_unlock(&libBufCtx->mutex);
			LOGE(TAG,"libBuf_dequeue, not enough size\n");
			return -1;
		}
    }

    ringbuf_memcpy_from(data, libBufCtx->rb, len);

    pthread_cond_broadcast(&libBufCtx->queueFullOrEmptyCV);

    pthread_mutex_unlock(&libBufCtx->mutex);

    return 0;
}


/**********************************************************
 * @brief Dequeue data
 *
 * @param libBufCtx
 * @param data
 * @param len
 *
 * @return
 *********************************************************/
int libBuf_dequeue(libBuf_context_t libBufCtx, uint8_t* data, uint16_t len)
{
    pthread_mutex_lock(&libBufCtx->mutex);

    //如果总大小根本不够，就不要等了，否则会死锁
    if(ringbuf_capacity(libBufCtx->rb) < len)
    {
        pthread_mutex_unlock(&libBufCtx->mutex);
        LOGE(TAG,"libBuf_dequeue, too big!!!\n");
        return -2;
    }

    //    ringbuf_is_empty(rb1);
    while(ringbuf_bytes_used(libBufCtx->rb) < len)
    {
	pthread_cond_wait(&libBufCtx->queueFullOrEmptyCV, &libBufCtx->mutex);
    }
    ringbuf_memcpy_from(data, libBufCtx->rb, len);

    pthread_cond_broadcast(&libBufCtx->queueFullOrEmptyCV);

    pthread_mutex_unlock(&libBufCtx->mutex);


    return 0;
}


/**********************************************************
 * @brief Dequeue data , return immediately
 *
 * @param libBufCtx
 * @param data
 * @param len
 *
 * @return
 *********************************************************/

int libBuf_dequeue_try(libBuf_context_t libBufCtx, uint8_t* data, uint16_t len)  //不阻塞 ，立刻返回
{
    pthread_mutex_lock(&libBufCtx->mutex);

    //    ringbuf_is_empty(rb1);
    if(ringbuf_bytes_used(libBufCtx->rb) < len)
    {
        pthread_mutex_unlock(&libBufCtx->mutex);
        return -1;
    }

    ringbuf_memcpy_from(data, libBufCtx->rb, len);

    pthread_cond_broadcast(&libBufCtx->queueFullOrEmptyCV);

    pthread_mutex_unlock(&libBufCtx->mutex);

    return 0;
}

/**********************************************************
 * @brief return used size
 *
 * @param libBufCtx
 *
 * @return
 *********************************************************/
int libBuf_getUsedSize(libBuf_context_t libBufCtx)  //不阻塞 ，立刻返回
{
    int used = 0;
    pthread_mutex_lock(&libBufCtx->mutex);
    used = ringbuf_bytes_used(libBufCtx->rb);
    pthread_mutex_unlock(&libBufCtx->mutex);
    return used;
}

/**********************************************************
 * @brief return free size
 *
 * @param libBufCtx
 *
 * @return
 *********************************************************/
int libBuf_getFreeSize(libBuf_context_t libBufCtx)  //不阻塞 ，立刻返回
{
    int free = 0;
    pthread_mutex_lock(&libBufCtx->mutex);
    free = ringbuf_bytes_free(libBufCtx->rb);
    pthread_mutex_unlock(&libBufCtx->mutex);
    return free;
}
