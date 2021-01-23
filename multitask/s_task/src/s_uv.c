#include "s_task.h"
#include "s_uv.h"

#ifdef USE_LIBUV

typedef struct {
    s_event_t event;
    bool trigged;
} s_uv_close_arg_t;

static void s_uv_close_cb(uv_handle_t* handle, void *arg) {
    s_uv_close_arg_t* arg_ = (s_uv_close_arg_t*)arg;
    arg_->trigged = true;
    s_event_set(&arg_->event);
}

int s_uv_close(__async__, uv_handle_t* handle) {
    s_uv_close_arg_t arg;
    arg.trigged = false;
    s_event_init(&arg.event);

    uv_close(handle, s_uv_close_cb, &arg);
    while (!arg.trigged) {
        int ret = s_event_wait(__await__, &arg.event);
        if (ret != 0)
            return ret;
    }
    return 0;
}


typedef struct {
    s_event_t event;
    void* buf;
    size_t buf_len;
    ssize_t nread;
    bool trigged;
} s_uv_read_start_arg_t;

static void s_uv_read_start_alloc_cb(
    uv_handle_t* handle,
    size_t suggested_size,
    uv_buf_t* buf,
    void* arg) {
    s_uv_read_start_arg_t* arg_ = (s_uv_read_start_arg_t*)arg;
    buf->base = arg_->buf;
    buf->len = arg_->buf_len;
}

static void s_uv_read_start_read_cb(
    uv_stream_t* stream,
    ssize_t nread,
    const uv_buf_t* buf,
    void* arg) {
    s_uv_read_start_arg_t* arg_ = (s_uv_read_start_arg_t*)arg;
    uv_read_stop(stream);
    arg_->nread = nread;
    arg_->trigged = true;
    s_event_set(&arg_->event);
}

int s_uv_read(__async__, uv_stream_t* handle, void* buf, size_t buf_len, ssize_t* nread) {
    int ret;
    s_uv_read_start_arg_t arg;
    arg.buf = buf;
    arg.buf_len = buf_len;
    arg.trigged = false;
    s_event_init(&arg.event);

    ret = uv_read_start(handle, s_uv_read_start_alloc_cb, s_uv_read_start_read_cb, &arg);
    if (ret != 0)
        return ret;
    while (!arg.trigged) {
        ret = s_event_wait(__await__, &arg.event);
        if (ret != 0)
            return ret;
    }
    uv_read_stop(handle);

    if (nread != NULL)
        *nread = arg.nread;

    ret = (int)(arg.nread < 0 ? arg.nread : 0);
    return ret;
}



typedef struct {
    s_event_t event;
    uv_write_t req;
    int status;
    bool trigged;
} s_uv_write_arg_t;

static void s_uv_write_cb(uv_write_t* req, int status) {
    s_uv_write_arg_t* arg_ = GET_PARENT_ADDR(req, s_uv_write_arg_t, req);
    /* printf("status = %d\n", status); */

    arg_->status = status;
    arg_->trigged = true;
    s_event_set(&arg_->event);
}

int s_uv_write_n(__async__, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs) {
    int ret;
    s_uv_write_arg_t arg;
    arg.trigged = false;
    s_event_init(&arg.event);

    ret = uv_write(&arg.req, handle, bufs, nbufs, s_uv_write_cb);
    if (ret != 0)
        return ret;

    while (!arg.trigged) {
        ret = s_event_wait(__await__, &arg.event);
        if (ret != 0)
            return ret;
    }

    ret = arg.status;

    return ret;
}

int s_uv_write(__async__, uv_stream_t* handle, const void* buf, size_t buf_len) {
    uv_buf_t uv_buf = uv_buf_init((char *)buf, buf_len);
    return s_uv_write_n(__await__, handle, &uv_buf, 1);
}


typedef struct {
    s_event_t event;
    void* buf;
    size_t buf_len;
    ssize_t nrecv;
    struct sockaddr addr;
    unsigned flags;
    bool trigged;
} s_uv_recv_start_arg_t;

static void uv_udp_recv_start_alloc_cb(
    uv_handle_t* handle,
    size_t suggested_size,
    uv_buf_t* buf,
    void* arg) {
    s_uv_recv_start_arg_t* arg_ = (s_uv_recv_start_arg_t*)arg;
    buf->base = arg_->buf;
    buf->len = arg_->buf_len;
}

static void uv_udp_recv_start_recv_cb(
    uv_udp_t* handle,
    ssize_t nread,
    const uv_buf_t* buf,
    const struct sockaddr* addr,
    unsigned flags,
    void *arg) {

    s_uv_recv_start_arg_t* arg_ = (s_uv_recv_start_arg_t*)arg;
    uv_udp_recv_stop(handle);
    arg_->nrecv = nread;
    arg_->flags = flags;
    arg_->addr = *addr;
    arg_->trigged = true;
    s_event_set(&arg_->event);
}

int uv_udp_recv(__async__,
    uv_udp_t* handle,
    void* buf,
    size_t buf_len,
    ssize_t* nrecv,
    struct sockaddr* addr,
    unsigned int *flags) {
    int ret;
    s_uv_recv_start_arg_t arg;
    arg.buf = buf;
    arg.buf_len = buf_len;
    arg.trigged = false;
    s_event_init(&arg.event);

    ret = uv_udp_recv_start(handle, uv_udp_recv_start_alloc_cb, uv_udp_recv_start_recv_cb, &arg);

    if (ret != 0)
        return ret;
    while (!arg.trigged) {
        ret = s_event_wait(__await__, &arg.event);
        if (ret != 0)
            return ret;
    }
    uv_udp_recv_stop(handle);

    if (nrecv != NULL)
        *nrecv = arg.nrecv;
    if (addr != NULL)
        *addr = arg.addr;
    if (flags != NULL)
        *flags = arg.flags;

    ret = (int)(arg.nrecv < 0 ? arg.nrecv : 0);
    return ret;
}




typedef struct {
    s_event_t event;
    uv_udp_send_t req;
    int status;
    bool trigged;
} s_uv_udp_send_t;

static void s_uv_udp_send_cb(uv_udp_send_t* req, int status) {
    s_uv_udp_send_t* arg_ = GET_PARENT_ADDR(req, s_uv_udp_send_t, req);
    /* printf("status = %d\n", status); */

    arg_->status = status;
    arg_->trigged = true;
    s_event_set(&arg_->event);
}

int s_uv_udp_send_n(__async__,
    uv_udp_t* handle,
    const uv_buf_t bufs[],
    unsigned int nbufs,
    const struct sockaddr* addr) {

    int ret;
    s_uv_udp_send_t arg;
    arg.trigged = false;
    s_event_init(&arg.event);

    ret = uv_udp_send(&arg.req, handle, bufs, nbufs, addr, s_uv_udp_send_cb);
    if (ret != 0)
        return ret;

    while (!arg.trigged) {
        ret = s_event_wait(__await__, &arg.event);
        if (ret != 0)
            return ret;
    }

    ret = arg.status;

    return ret;
}

int s_uv_udp_send(__async__, uv_udp_t* handle,
    const void* buf,
    size_t buf_len,
    const struct sockaddr* addr) {
    uv_buf_t uv_buf = uv_buf_init((char*)buf, buf_len);
    return s_uv_udp_send_n(__await__, handle, &uv_buf, 1, addr);
}



typedef struct {
    s_event_t event;
    uv_getaddrinfo_t req;
    int status;
    bool trigged;
} s_uv_getaddrinfo_t;

static void s_uv_getaddrinfo_cb(uv_getaddrinfo_t* req,
    int status,
    struct addrinfo* res) {
    s_uv_getaddrinfo_t* arg_ = GET_PARENT_ADDR(req, s_uv_getaddrinfo_t, req);
    arg_->status = status;
    arg_->trigged = true;
    s_event_set(&arg_->event);
}

struct addrinfo *s_uv_getaddrinfo(__async__,
    uv_loop_t* loop,
    const char* node,
    const char* service,
    const struct addrinfo* hints) {

    int ret;
    s_uv_getaddrinfo_t arg;
    arg.trigged = false;
    arg.req.addrinfo = NULL;
    /* arg.req.addrinfow = NULL; */
    s_event_init(&arg.event);

    ret = uv_getaddrinfo(loop, &arg.req, s_uv_getaddrinfo_cb, node, service, hints);
    if (ret != 0) {
        goto out;
    }

    while (!arg.trigged) {
        ret = s_event_wait(__await__, &arg.event);
        if (ret != 0) goto out;
    }

    ret = arg.status;

out:;
    if (ret != 0) {
        if(arg.req.addrinfo != NULL)
            uv_freeaddrinfo(arg.req.addrinfo);
        return NULL;
    }
    else {
        /* use uv_freeaddrinfo(arg.req.addrinfo) to free the return value; */
        return arg.req.addrinfo;
    }
}



typedef struct {
    s_event_t event;
    uv_connect_t req;
    int status;
    bool trigged;
} s_uv_tcp_connect_t;

static void s_uv_connect_cb(uv_connect_t* req, int status) {
    s_uv_tcp_connect_t* arg_ = GET_PARENT_ADDR(req, s_uv_tcp_connect_t, req);
    arg_->status = status;
    arg_->trigged = true;
    s_event_set(&arg_->event);
}

int s_uv_tcp_connect(__async__, uv_tcp_t* handle, const struct sockaddr* addr) {
    int ret;
    s_uv_tcp_connect_t arg;
    arg.trigged = false;
    s_event_init(&arg.event);

    ret = uv_tcp_connect(&arg.req, handle, addr, s_uv_connect_cb);
    if (ret != 0)
        return ret;

    while (!arg.trigged) {
        ret = s_event_wait(__await__, &arg.event);
        if (ret != 0)
            return ret;
    }
    ret = arg.status;

    return ret;
}

#endif
