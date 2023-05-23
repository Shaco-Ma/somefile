/*
 * =====================================================================================
 *
 *       Filename:  libbuf.h
 *
 *    Description:  Buffer Manager
 *
 *        Version:  1.0
 *        Created:
 *
 * =====================================================================================
 */
#ifndef _BUFFER_MANAGER_H_
#define _BUFFER_MANAGER_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct libBuf_context *libBuf_context_t;

libBuf_context_t libBuf_init(uint16_t size) ;
int libBuf_enqueue(libBuf_context_t libBufCtx, uint8_t* data, uint16_t len) ;
//阻塞一段时间返回 毫秒
int libBuf_enqueue_timeout(libBuf_context_t libBufCtx, uint8_t* data, uint16_t len , uint16_t timeout_ms) ;
int libBuf_enqueue_try(libBuf_context_t libBufCtx, uint8_t* data, uint16_t len); //不阻塞 ，立刻返回
int libBuf_enqueue_force(libBuf_context_t libBufCtx, uint8_t* data, uint16_t len); //不阻塞 ，立刻返回
int libBuf_dequeue(libBuf_context_t libBufCtx, uint8_t* data, uint16_t len) ;
//阻塞一段时间返回 毫秒
int libBuf_dequeue_timeout(libBuf_context_t libBufCtx, uint8_t* data, uint16_t len , uint16_t timeout_ms) ;
int libBuf_dequeue_try(libBuf_context_t libBufCtx, uint8_t* data, uint16_t len) ; //不阻塞 ，立刻返回
int libBuf_getUsedSize(libBuf_context_t libBufCtx) ; //不阻塞 ，立刻返回
int libBuf_getFreeSize(libBuf_context_t libBufCtx) ; //不阻塞 ，立刻返回

//清空buffer
void libBuf_reset(libBuf_context_t libBufCtx) ;
int libBuf_free(libBuf_context_t libBufCtx);


#ifdef __cplusplus
};
#endif
#endif
