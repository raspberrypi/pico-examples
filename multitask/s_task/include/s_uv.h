/* Copyright xhawk, MIT license */

#ifndef INC_S_UV_H_
#define INC_S_UV_H_

#include "s_task.h"
#ifdef USE_LIBUV
#include "uv.h"

#ifdef __cplusplus
extern "C" {
#endif

int s_uv_close(__async__, uv_handle_t* handle);
int s_uv_read(__async__, uv_stream_t* handle, void* buf, size_t buf_len, ssize_t* nread);
int s_uv_write_n(__async__, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs);
int s_uv_write(__async__, uv_stream_t* handle, const void* buf, size_t buf_len);

int uv_udp_recv(__async__,
    uv_udp_t* handle,
    void* buf, size_t buf_len,
    ssize_t* nrecv,
    struct sockaddr* addr,
    unsigned int* flags);

int s_uv_udp_send_n(__async__,
    uv_udp_t* handle,
    const uv_buf_t bufs[],
    unsigned int nbufs,
    const struct sockaddr* addr);
int s_uv_udp_send(__async__,
    uv_udp_t* handle,
    const void* buf,
    size_t buf_len,
    const struct sockaddr* addr);

/* use uv_freeaddrinfo() to free the return value; */
struct addrinfo* s_uv_getaddrinfo(__async__,
    uv_loop_t* loop,
    const char* node,
    const char* service,
    const struct addrinfo* hints);

int s_uv_tcp_connect(__async__, uv_tcp_t* handle, const struct sockaddr* addr);

#ifdef __cplusplus
}
#endif
#endif
#endif
