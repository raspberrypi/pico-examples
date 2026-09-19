/**
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/rpi_connect.h"
#include "rpi_connect_ota_demo.h"
#include "pico/rpi_connect_ota.h"

/* Poll-only mode for data-limited or metered connections: skip the SSE event
 * stream and rely on the boot-time pending-deployment check. New deployments
 * are then only picked up on the next boot (applying one reboots anyway). */
#ifndef RPI_CONNECT_OTA_DEMO_POLL_ONLY
#define RPI_CONNECT_OTA_DEMO_POLL_ONLY 0
#endif

static char *deployment_id;

// Minimum interval between download progress reports.
#define RPI_CONNECT_OTA_DEMO_PROGRESS_INTERVAL_MS 200

static rpi_connect_ota_install_update_ctx_t *update_ctx;
static char *update_deployment_id;
static size_t update_progress_bytes;   // last download progress reported
static uint32_t update_progress_ms;    // wallclock time of the last report
static rpi_connect_event_context_t *event_ctx;

// Report download progress from the main loop: at most once every
// RPI_CONNECT_OTA_DEMO_PROGRESS_INTERVAL_MS, and only when it has advanced.
static void rpi_connect_ota_demo_report_progress(void) {
    if (!update_ctx) {
        return;
    }
    size_t bytes_so_far = rpi_connect_ota_install_update_bytes_so_far(update_ctx);
    size_t content_length = rpi_connect_ota_install_update_content_length(update_ctx);
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if (bytes_so_far == update_progress_bytes ||
            now_ms - update_progress_ms < RPI_CONNECT_OTA_DEMO_PROGRESS_INTERVAL_MS)
        return;
    update_progress_bytes = bytes_so_far;
    update_progress_ms = now_ms;
    if (content_length > 0) {
        RPI_CONNECT_OTA_DEMO_INFO("Download progress: %zu / %zu bytes (%u%%)\n",
               bytes_so_far, content_length,
               (unsigned)((bytes_so_far * 100) / content_length));
    } else {
        RPI_CONNECT_OTA_DEMO_INFO("Download progress: %zu bytes\n", bytes_so_far);
    }
}

#if !RPI_CONNECT_OTA_DEMO_POLL_ONLY
// Event-stream callback: the server has assigned a new deployment to this device.
static void demo_deploy_callback(const char *id, __unused void *arg) {
    if (!deployment_id) {
        deployment_id = strdup(id);
    } else {
        RPI_CONNECT_OTA_DEMO_INFO("Already processing deployment ID=%s, ignoring new deployment ID=%s\n",
               deployment_id, id);
    }
}
#endif

int rpi_connect_ota_demo_init(const char *client_id, const char *serial_number, const char *hostname) {
    rpi_connect_set_async_context(rpi_connect_default_async_context());

    // Learn wall-clock time from the server's /up endpoint before signing in,
    // so the device-identity exchange inside rpi_connect_ota_init() carries
    // X-Connect-Timestamp. Not fatal if unavailable: the header is optional.
    int64_t server_time = rpi_connect_update_time();
    if (server_time > 0) {
        RPI_CONNECT_OTA_DEMO_INFO("Server time: %lld\n", (long long)server_time);
    } else {
        RPI_CONNECT_OTA_DEMO_ERROR("Failed to get server time\n");
    }

    // The bootrom "buy" of a pending flash update is deferred to
    // rpi_connect_ota_boot_sync() so only an image that reaches the server is committed.
    return rpi_connect_ota_init(client_id, serial_number, hostname);
}

// Downloads verify the artefact host's TLS certificate against the Pi
// Connect root CA by default. Build with
// RPI_CONNECT_OTA_DEMO_UNVERIFIED_DOWNLOAD=1 if artefacts are hosted on a
// server that does not chain to that root: integrity then rests entirely on
// the deployment's SHA-256 checksum.
#ifndef RPI_CONNECT_OTA_DEMO_UNVERIFIED_DOWNLOAD
#define RPI_CONNECT_OTA_DEMO_UNVERIFIED_DOWNLOAD 0
#endif

// Start the firmware download for a deployment and track it in update_ctx.
static void handle_deployment_download(const char *token, const char *uri, const char *checksum) {
    update_progress_bytes = 0;
    // Back-date the last report so the first progress line prints immediately.
    update_progress_ms = to_ms_since_boot(get_absolute_time()) - RPI_CONNECT_OTA_DEMO_PROGRESS_INTERVAL_MS;
    update_ctx = rpi_connect_ota_install_update_start(uri, checksum,
                                                      RPI_CONNECT_OTA_DEMO_UNVERIFIED_DOWNLOAD);
    if (update_ctx) {
        update_deployment_id = deployment_id;
        deployment_id = NULL;
    } else {
        rpi_connect_ota_fail_deployment(token, deployment_id, "download failed");
    }
}

static void cleanup_deployment(char **uri, char **checksum) {
    free(*uri);
    free(*checksum);
    if (deployment_id) {
        free(deployment_id);
        deployment_id = NULL;
    }
}

// Resume an in-progress deployment that was interrupted by a reboot.
static void rpi_connect_ota_demo_resume_deployment(const char *token) {
    deployment_id = rpi_connect_ota_get_resumable_deployment_id();
    if (!deployment_id) {
        return;
    }

    char *uri = NULL;
    char *checksum = NULL;
    if (rpi_connect_ota_resume_deployment(token, deployment_id, &uri, &checksum) == 0) {
        handle_deployment_download(token, uri, checksum);
    }
    cleanup_deployment(&uri, &checksum);
}

// Accept a newly assigned deployment and begin downloading the firmware.
static void rpi_connect_ota_demo_start_deployment(const char *token) {
    char *uri = NULL;
    char *checksum = NULL;
    if (rpi_connect_ota_start_deployment(token, deployment_id, &uri, &checksum) == 0) {
        handle_deployment_download(token, uri, checksum);
    }
    cleanup_deployment(&uri, &checksum);
}

// Finalise a finished download: report success/failure and reboot if needed.
static void rpi_connect_ota_demo_complete_deployment(const char *token, int rc_done,
                                                 const rpi_connect_ota_download_result_t *result) {
    bool transient = rc_done < 0 && rpi_connect_ota_install_update_error_is_transient(update_ctx);
    rpi_connect_ota_install_update_stop(update_ctx);
    update_ctx = NULL;

    if (rc_done > 0) {
        RPI_CONNECT_OTA_DEMO_INFO("Download succeeded for deployment ID=%s (%zu bytes sha256=%s)\n",
               update_deployment_id, result->total_bytes, result->sha256_hex);
        // Success is reported only after the new image boots and signs in:
        // mark the deployment APPLYING and reboot.
        rpi_connect_ota_set_deployment_status(RPI_CONNECT_OTA_DEPLOYMENT_APPLYING);
        RPI_CONNECT_OTA_DEMO_INFO("Rebooting to apply update...\n");
        rpi_connect_ota_try_booting_to_flash_update();
        // Only reached if the reboot did not happen.
        rpi_connect_ota_fail_deployment(token, update_deployment_id, "failed to apply update");
    } else if (transient) {
        // Transient transport failure: keep the deployment and let the resume
        // path retry it on the next boot.
        RPI_CONNECT_OTA_DEMO_ERROR("Download failed for deployment ID=%s; assuming transient error\n",
               update_deployment_id);
    } else {
        // HTTP error or checksum mismatch: not retrievable as-is, fail it.
        rpi_connect_ota_fail_deployment(token, update_deployment_id, "download failed");
    }

    free(update_deployment_id);
    update_deployment_id = NULL;
}

// One-shot check for a deployment assigned while the device was offline;
// the event stream does not replay these.
static void rpi_connect_ota_demo_check_pending_deployment(const char *token) {
    char *id = NULL;
    int rc = rpi_connect_get_pending_deployment(token, &id);
    if (rc != 0) {
        RPI_CONNECT_OTA_DEMO_ERROR("Failed to check pending deployments (%d)\n", rc);
        return;
    }
    if (id) {
        RPI_CONNECT_OTA_DEMO_INFO("Pending deployment ID=%s\n", id);
        deployment_id = id;
    } else {
        RPI_CONNECT_OTA_DEMO_INFO("No pending deployment\n");
    }
}

#if !RPI_CONNECT_OTA_DEMO_POLL_ONLY
// Start listening for deployment events from the server.
static int rpi_connect_ota_demo_start_event_listener(const char *token) {
    if (event_ctx) {
        return 0;
    }

    RPI_CONNECT_OTA_DEMO_INFO("Starting event listener\n");
    event_ctx = rpi_connect_ota_event_listen(
        rpi_connect_default_async_context(), token, demo_deploy_callback, NULL);
    if (!event_ctx) {
        RPI_CONNECT_OTA_DEMO_ERROR("Failed to start event listener\n");
        return -1;
    }
    return 0;
}
#endif

// Dispatch based on current state: drive an in-flight download or start a new
// deployment.
static int rpi_connect_ota_demo_check_ota(const char *token) {
    if (update_ctx) {
        rpi_connect_ota_download_result_t result;
        int rc = rpi_connect_ota_install_update_poll(update_ctx, &result);
        if (rc != 0) {
            rpi_connect_ota_demo_complete_deployment(token, rc, &result);
        }
    } else if (deployment_id) {
        rpi_connect_ota_demo_start_deployment(token);
    }
#if !RPI_CONNECT_OTA_DEMO_POLL_ONLY
    else if (!event_ctx) {
        return rpi_connect_ota_demo_start_event_listener(token);
    }
#endif
    return 0;
}

// Application main loop: reconcile OTA state at boot, resume any interrupted
// deployment, then poll for new deployments alongside the application's own
// (non-OTA) work.
int rpi_connect_ota_demo_main(const char *token) {
    RPI_CONNECT_OTA_DEMO_DEBUG("%s token = %s\n", __func__, token);

    // Register the OTA capability, report the outcome of a just-applied update
    // and commit it (the bootrom "buy"). On error the buy has not been issued:
    // return, so the bootrom rolls back on the next reset.
    int rc = rpi_connect_ota_boot_sync(token);
    if (rc != 0) {
        return rc;
    }

    event_ctx = NULL;
    update_ctx = NULL;
    update_deployment_id = NULL;
    update_progress_bytes = 0;
    update_progress_ms = 0;

    rpi_connect_ota_demo_resume_deployment(token);

    // A resumed download is already in flight and ends in a reboot; otherwise
    // check for a deployment assigned while offline. In event mode the
    // listener is started (by check_ota) once that deployment is handled.
    if (!update_ctx) {
        rpi_connect_ota_demo_check_pending_deployment(token);
    }

    while (1) {
        // A real application does its non-OTA work here. With a polled async
        // context the network only makes progress inside async_context_poll(),
        // so avoid blocking for long between calls.
#if PICO_ON_DEVICE
        async_context_poll(rpi_connect_default_async_context());
#else
        if (event_ctx) {
            rpi_connect_event_poll(event_ctx);
        }
#endif
        rpi_connect_ota_demo_report_progress();

        if (rpi_connect_ota_demo_check_ota(token) != 0) {
            rc = -1;
            break;
        }
        sleep_ms(10);
    }

    if (event_ctx) {
        rpi_connect_event_stop(event_ctx);
    }
    return rc;
}
