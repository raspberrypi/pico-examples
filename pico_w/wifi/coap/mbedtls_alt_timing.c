#include "mbedtls/timing.h"

async_context_t *async_context;
async_at_time_worker_t worker;

void mbedtls_timing_set_delay(void *data, uint32_t int_ms, uint32_t fin_ms)
{
    mbedtls_timing_delay_context *ctx = (mbedtls_timing_delay_context*)data;
    if (fin_ms == 0) {
        //async_context_remove_at_time_worker(ctx->async_context, &ctx->worker);
        ctx->fin_time = nil_time;
        return;
    }
    // todo: do we need this?
    //async_context_add_at_time_worker_in_ms(ctx->async_context, &ctx->worker, fin_ms);
    ctx->fin_time = make_timeout_time_ms(fin_ms);
    ctx->int_time = make_timeout_time_ms(int_ms);
}

int mbedtls_timing_get_delay(void *data)
{
    mbedtls_timing_delay_context *ctx = (mbedtls_timing_delay_context*)data;
    if (is_nil_time(ctx->fin_time)) {
        return -1;
    }
    if (time_reached(ctx->fin_time)) {
        return 2;
    }
    if (time_reached(ctx->int_time)) {
        return 1;
    }
    return 0;
}
