/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rpi_connect_ota_demo.h"
#include "pico/rpi_connect_ota.h"
#include "pico/rpi_connect.h"
#include "connect_crypto.h"
#include "request.h"
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <openssl/pem.h>
#include <openssl/ec.h>
#include <openssl/bn.h>

enum {
    OPT_CREATE_DEVICE_IDENTITY = 256,
    OPT_ORG_TOKEN,
    OPT_DEVICE_PRIVKEY,
    OPT_DEVICE_PUBKEY,
    OPT_DESCRIPTION,
    OPT_DEVICE_NAME,
    OPT_DEVICE_IDENTITY_EXCHANGE,
};

static void print_hex(const char *label, const unsigned char *data, size_t len) {
    RPI_CONNECT_OTA_DEMO_INFO("%s (%zu bytes): ", label, len);
    for (size_t i = 0; i < len; i++) {
        RPI_CONNECT_OTA_DEMO_INFO("%02x", data[i]);
    }
    RPI_CONNECT_OTA_DEMO_INFO("\n");
}

static int load_ec_p256_pem(const char *filename,
                            unsigned char privkey[RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE],
                            unsigned char pubkey[65]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        RPI_CONNECT_OTA_DEMO_ERROR("Error: cannot open %s: %s\n", filename, strerror(errno));
        return -1;
    }

    EVP_PKEY *pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);
    if (!pkey) {
        RPI_CONNECT_OTA_DEMO_ERROR("Error: failed to parse PEM from %s\n", filename);
        return -1;
    }

    const EC_KEY *ec = EVP_PKEY_get0_EC_KEY(pkey);
    if (!ec) {
        RPI_CONNECT_OTA_DEMO_ERROR("Error: key in %s is not an EC key\n", filename);
        EVP_PKEY_free(pkey);
        return -1;
    }

    const EC_GROUP *group = EC_KEY_get0_group(ec);
    if (EC_GROUP_get_curve_name(group) != NID_X9_62_prime256v1) {
        RPI_CONNECT_OTA_DEMO_ERROR("Error: key in %s is not P-256\n", filename);
        EVP_PKEY_free(pkey);
        return -1;
    }

    const BIGNUM *priv_bn = EC_KEY_get0_private_key(ec);
    if (!priv_bn || BN_bn2binpad(priv_bn, privkey, RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE) != RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE) {
        RPI_CONNECT_OTA_DEMO_ERROR("Error: failed to extract private key\n");
        EVP_PKEY_free(pkey);
        return -1;
    }

    const EC_POINT *pub = EC_KEY_get0_public_key(ec);
    if (!pub || EC_POINT_point2oct(group, pub, POINT_CONVERSION_UNCOMPRESSED,
                                    pubkey, 65, NULL) != 65) {
        RPI_CONNECT_OTA_DEMO_ERROR("Error: failed to extract public key\n");
        EVP_PKEY_free(pkey);
        return -1;
    }

    EVP_PKEY_free(pkey);
    return 0;
}

static int test_ec_sign(const char *filename) {
    unsigned char privkey[RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE];
    unsigned char pubkey[65];

    if (load_ec_p256_pem(filename, privkey, pubkey) != 0) {
        return -1;
    }

    print_hex("Private key", privkey, sizeof(privkey));
    print_hex("Public key", pubkey, sizeof(pubkey));

    /* Sign a test hash */
    unsigned char test_hash[RPI_CONNECT_SHA256_SIZE];
    size_t hash_len;
    if (rpi_connect_crypto_sha256("test message", 12, test_hash, &hash_len) != 0) {
        RPI_CONNECT_OTA_DEMO_ERROR("Error: SHA-256 failed\n");
        return -1;
    }
    print_hex("SHA-256(\"test message\")", test_hash, hash_len);

    unsigned char sig[RPI_CONNECT_CRYPTO_ECDSA_P256_SIG_MAX_SIZE];
    size_t sig_len = sizeof(sig);
    if (rpi_connect_crypto_ecdsa_p256_sign(test_hash, privkey, sig, &sig_len) != 0) {
        RPI_CONNECT_OTA_DEMO_ERROR("Error: ECDSA sign failed\n");
        return -1;
    }
    print_hex("ECDSA-P256 signature", sig, sig_len);

    RPI_CONNECT_OTA_DEMO_INFO("EC key loaded and test sign OK\n");
    return 0;
}

static char *read_file_to_string(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        RPI_CONNECT_OTA_DEMO_ERROR("Error: cannot open %s: %s\n", filename, strerror(errno));
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    if (!buf) {
        fclose(fp);
        return NULL;
    }
    size_t nread = fread(buf, 1, len, fp);
    buf[nread] = '\0';
    fclose(fp);
    return buf;
}

static void usage(const char *progname) {
    fprintf(stderr, "Usage: %s [options]\n", progname);
    fprintf(stderr, "\nAuthentication:\n");
    fprintf(stderr, "  --signin                    Run signin flow to obtain token\n");
    fprintf(stderr, "  --auth <key>                Exchange auth key for token\n");
    fprintf(stderr, "\nActions (require RPI_CONNECT_TOKEN):\n");
    fprintf(stderr, "  --ota                       Run OTA test\n");
    fprintf(stderr, "  --start-deployment <id>     Start deployment\n");
    fprintf(stderr, "  --complete-deployment <id>  Complete deployment\n");
    fprintf(stderr, "  --cancel-deployment <id>    Cancel deployment\n");
    fprintf(stderr, "\nConfiguration:\n");
    fprintf(stderr, "  --serial <serial>           Device serial (or RPI_CONNECT_SERIAL env)\n");
    fprintf(stderr, "  --client-id <id>            Client ID (or RPI_CONNECT_CLIENT_ID env)\n");
    fprintf(stderr, "  --token <token>             Access token (or RPI_CONNECT_TOKEN env)\n");
    fprintf(stderr, "\nCrypto:\n");
    fprintf(stderr, "  --ec-key <file>             Load ECDSA P-256 PEM key and test sign\n");
    fprintf(stderr, "\nDevice identity:\n");
    fprintf(stderr, "  --create-device-identity    Register a device identity with an organisation\n");
    fprintf(stderr, "  --device-identity-exchange  Exchange a registered device identity for an access token\n");
    fprintf(stderr, "  --org-token <token>         Organisation token (or RPI_CONNECT_ORG_TOKEN env)\n");
    fprintf(stderr, "  --device-privkey <file>     EC P-256 private key PEM (for signing)\n");
    fprintf(stderr, "  --device-pubkey <file>      Device public key PEM (registered as identity)\n");
    fprintf(stderr, "  --description <text>        Description for the device identity\n");
    fprintf(stderr, "  --device-name <name>        Optional device name\n");
    fprintf(stderr, "\nOther:\n");
    fprintf(stderr, "  -v, --verbose               Increase verbosity\n");
    fprintf(stderr, "  -h, --help                  Show this help\n");
}

async_context_t *rpi_connect_default_async_context(void) {
    return NULL;
}

int main(int argc, char *argv[]) {
    char *serial_number = getenv("RPI_CONNECT_SERIAL");
    char *client_id = getenv("RPI_CONNECT_CLIENT_ID");
    if (!client_id) {
        client_id = rpi_connect_client_id();
    }
    // Test hook: point the API at a local mock server (host or host:port).
    char *api_host = getenv("RPI_CONNECT_API_HOST");
    if (api_host) {
        rpi_connect_set_api_host(api_host);
    }
    char *auth_key = getenv("RPI_CONNECT_AUTH_KEY");
    char *token = getenv("RPI_CONNECT_TOKEN");
    char *deployment_id = NULL;
    char *ec_key_file = NULL;
    int verbose_level = 0;
    int do_signin = 0;
    int do_ota = 0;
    int do_auth = 0;
    int do_start_deployment = 0;
    int do_complete_deployment = 0;
    int do_cancel_deployment = 0;
    int do_ec_key = 0;
    int do_create_device_identity = 0;
    int do_device_identity_exchange = 0;
    char *org_token = getenv("RPI_CONNECT_ORG_TOKEN");
    char *device_privkey_file = NULL;
    char *device_pubkey_file = NULL;
    char *description = NULL;
    char *device_name_arg = NULL;
    int opt;
    int rc = -1;

    static struct option long_options[] = {
        {"signin",    no_argument,       0, 's'},
        {"ota",       no_argument,       0, 'o'},
        {"auth",      required_argument, 0, 'a'},
        {"serial",    required_argument, 0, 'S'},
        {"client-id", required_argument, 0, 'C'},
        {"token",     required_argument, 0, 'T'},
        {"start-deployment", required_argument, 0, 'd'},
        {"complete-deployment", required_argument, 0, 'D'},
        {"cancel-deployment", required_argument, 0, 'c'},
        {"ec-key",    required_argument, 0, 'e'},
        {"create-device-identity", no_argument, 0, OPT_CREATE_DEVICE_IDENTITY},
        {"device-identity-exchange", no_argument, 0, OPT_DEVICE_IDENTITY_EXCHANGE},
        {"org-token", required_argument, 0, OPT_ORG_TOKEN},
        {"device-privkey", required_argument, 0, OPT_DEVICE_PRIVKEY},
        {"device-pubkey", required_argument, 0, OPT_DEVICE_PUBKEY},
        {"description", required_argument, 0, OPT_DESCRIPTION},
        {"device-name", required_argument, 0, OPT_DEVICE_NAME},
        {"verbose",   no_argument,       0, 'v'},
        {"help",      no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "vh", long_options, NULL)) != -1) {
        switch (opt) {
        case 's':
            do_signin = 1;
            break;
        case 'o':
            do_ota = 1;
            break;
        case 'a':
            do_auth = 1;
            auth_key = optarg;
            break;
        case 'd':
            do_start_deployment = 1;
            deployment_id = optarg;
            break;
        case 'D':
            do_complete_deployment = 1;
            deployment_id = optarg;
            break;
        case 'c':
            do_cancel_deployment = 1;
            deployment_id = optarg;
            break;
        case 'e':
            do_ec_key = 1;
            ec_key_file = optarg;
            break;
        case OPT_CREATE_DEVICE_IDENTITY:
            do_create_device_identity = 1;
            break;
        case OPT_DEVICE_IDENTITY_EXCHANGE:
            do_device_identity_exchange = 1;
            break;
        case OPT_ORG_TOKEN:
            org_token = optarg;
            break;
        case OPT_DEVICE_PRIVKEY:
            device_privkey_file = optarg;
            break;
        case OPT_DEVICE_PUBKEY:
            device_pubkey_file = optarg;
            break;
        case OPT_DESCRIPTION:
            description = optarg;
            break;
        case OPT_DEVICE_NAME:
            device_name_arg = optarg;
            break;
        case 'S':
            serial_number = optarg;
            break;
        case 'C':
            client_id = optarg;
            break;
        case 'T':
            token = optarg;
            break;
        case 'v':
            verbose_level++;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (serial_number) {
        serial_number = strdup(serial_number);
    }
    if (client_id) {
        client_id = strdup(client_id);
    }
    if (auth_key) {
        auth_key = strdup(auth_key);
    }
    if (token) {
        token = strdup(token);
    }

    /* --ec-key is self-contained; handle it before the serial/client-id checks */
    if (do_ec_key) {
        rc = test_ec_sign(ec_key_file);
        goto end;
    }

    if (do_device_identity_exchange) {
        if (!client_id || !*client_id) {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: --client-id required (or RPI_CONNECT_CLIENT_ID env)\n");
            rc = -1;
            goto end;
        }
        if (!serial_number || !*serial_number) {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: --serial required (or RPI_CONNECT_SERIAL env)\n");
            rc = -1;
            goto end;
        }
        if (!device_privkey_file) {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: --device-privkey required\n");
            rc = -1;
            goto end;
        }
        if (!device_pubkey_file) {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: --device-pubkey required\n");
            rc = -1;
            goto end;
        }

        unsigned char privkey[RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE];
        unsigned char pubkey_raw[65];
        if (load_ec_p256_pem(device_privkey_file, privkey, pubkey_raw) != 0) {
            rc = -1;
            goto end;
        }

        char *pubkey_pem = read_file_to_string(device_pubkey_file);
        if (!pubkey_pem) {
            rc = -1;
            goto end;
        }

        rpi_connect_request_set_verbose(verbose_level);
        const char *hostname = device_name_arg ? device_name_arg : "rpi_connect_ota_demo";
        rpi_connect_ota_demo_init(client_id, serial_number, hostname);

        char *device_id = NULL;
        char *new_token = rpi_connect_device_identity_exchange(
            client_id, privkey, pubkey_pem, hostname, serial_number, &device_id);
        free(pubkey_pem);

        if (new_token) {
            rpi_connect_ota_store_auth_token(new_token);
            RPI_CONNECT_OTA_DEMO_INFO("Success: device_id=%s\n",
                             device_id ? device_id : "(null)");
#if !PICO_ON_DEVICE
            printf("RPI_CONNECT_TOKEN=%s\n", new_token);
#endif
            free(new_token);
            free(device_id);
            rc = 0;
        } else {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: device identity exchange failed\n");
            free(device_id);
            rc = -1;
        }
        goto end;
    }

    if (do_create_device_identity) {
        if (!org_token || !*org_token) {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: --org-token required (or RPI_CONNECT_ORG_TOKEN env)\n");
            rc = -1;
            goto end;
        }
        if (!device_privkey_file) {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: --device-privkey required\n");
            rc = -1;
            goto end;
        }
        if (!device_pubkey_file) {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: --device-pubkey required\n");
            rc = -1;
            goto end;
        }
        if (!description) {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: --description required\n");
            rc = -1;
            goto end;
        }

        unsigned char privkey[RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE];
        unsigned char pubkey_raw[65];
        if (load_ec_p256_pem(device_privkey_file, privkey, pubkey_raw) != 0) {
            rc = -1;
            goto end;
        }

        char *pubkey_pem = read_file_to_string(device_pubkey_file);
        if (!pubkey_pem) {
            rc = -1;
            goto end;
        }

        rpi_connect_request_set_verbose(verbose_level);

        char *id = rpi_connect_create_device_identity(
            org_token, privkey, pubkey_pem, description, device_name_arg);
        free(pubkey_pem);

        if (id) {
            free(id);
            rc = 0;
        } else {
            rc = -1;
        }
        goto end;
    }

    if (!do_signin && !do_ota && !do_start_deployment && !do_complete_deployment && !do_cancel_deployment && !do_auth) {
        RPI_CONNECT_OTA_DEMO_ERROR("Error: must specify --signin, --ota, --auth, --start-deployment, --complete-deployment, --cancel-deployment, --ec-key, --create-device-identity, or --device-identity-exchange\n");
        usage(argv[0]);
        rc = -1;
        goto end;
    }

    if (!serial_number || !*serial_number) {
        RPI_CONNECT_OTA_DEMO_ERROR("Error: serial number required (--serial or RPI_CONNECT_SERIAL)\n");
        rc = -1;
        goto end;
    }

    if (!client_id || !*client_id) {
        RPI_CONNECT_OTA_DEMO_ERROR("Error: client ID required (--client-id or RPI_CONNECT_CLIENT_ID)\n");
        rc = -1;
        goto end;
    }

    rpi_connect_request_set_verbose(verbose_level);
    // A token supplied at runtime (--token / RPI_CONNECT_TOKEN) is stored in
    // the FFS stub so init's FFS-token path picks it up, mirroring how a
    // provisioned token reaches the device build.
    if (token && *token) {
        rpi_connect_ota_store_auth_token(token);
    }
    rpi_connect_ota_demo_init(client_id, serial_number, "rpi_connect_ota_demo");

    if (do_signin) {
        t_rpi_connect_signin *signin = rpi_connect_signin(client_id, serial_number);
        if (!signin) {
            RPI_CONNECT_OTA_DEMO_ERROR("Failed to get signin information\n");
            rc = -1;
            goto end;
        }

        RPI_CONNECT_OTA_DEMO_INFO("Device code: %s\n", signin->device_code);
        RPI_CONNECT_OTA_DEMO_INFO("User code: %s\n", signin->user_code);
        RPI_CONNECT_OTA_DEMO_INFO("Verification URI: %s\n", signin->verification_uri_complete);
        RPI_CONNECT_OTA_DEMO_INFO("\nPlease visit the verification URI in your browser and enter the user code.\n");
        while (1) {
            token = rpi_connect_retrieve_token_with_device_code(client_id, signin->device_code, serial_number);
            if (token) {
                RPI_CONNECT_OTA_DEMO_INFO("\n");
                RPI_CONNECT_OTA_DEMO_INFO("Access token received successfully!\n");
                RPI_CONNECT_OTA_DEMO_INFO("Access token: %s\n", token);
                rpi_connect_signin_cleanup(signin);
                rc = 0;
                break;
            }
            RPI_CONNECT_OTA_DEMO_INFO(".");
            fflush(stdout);
            sleep_ms(3000);
        }
    } else if (do_start_deployment) {
        char *uri = NULL;
        char *checksum = NULL;
        rc = rpi_connect_start_deployment(token, deployment_id, &uri, &checksum);
        if (rc != 0) {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: failed to start deployment (%d)\n", rc);
        } else {
            RPI_CONNECT_OTA_DEMO_INFO("Success: deployment started uri=%s checksum=%s\n",
                             uri ? uri : "(null)", checksum ? checksum : "(null)");
            rc = 0;
        }
        free(uri);
        free(checksum);
    } else if (do_complete_deployment) {
        rc = rpi_connect_complete_deployment(token, deployment_id);
        if (rc != 0) {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: failed to complete deployment (%d)\n", rc);
        } else {
            RPI_CONNECT_OTA_DEMO_INFO("Success: deployment completed\n");
            rc = 0;
        }
    } else if (do_cancel_deployment) {
        rc = rpi_connect_fail_deployment(token, deployment_id, "cancelled by unit-test");
        if (rc != 0) {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: failed to cancel deployment (%d)\n", rc);
        } else {
            RPI_CONNECT_OTA_DEMO_INFO("Success: deployment cancelled\n");
            rc = 0;
        }
    } else if (do_auth) {
        if (token) {
            free(token);
        }
        token = rpi_connect_handle_auth_key(auth_key, serial_number, "rpi_connect_ota_demo", client_id);
        if (!token) {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: failed to get token\n");
            rc = -1;
            goto end;
        } else {
            RPI_CONNECT_OTA_DEMO_INFO("Success: TOKEN=%s\n", token);
            rc = 0;
        }
    } else if (do_ota) {
        if (!token || !*token) {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: token required for OTA (--token or RPI_CONNECT_TOKEN)\n");
            rc = -1;
            goto end;
        }
        rc = rpi_connect_ota_demo_main(rpi_connect_ota_get_auth_token());
        if (rc != 0) {
            RPI_CONNECT_OTA_DEMO_ERROR("Error: failed to send OTA\n");
        } else {
            RPI_CONNECT_OTA_DEMO_INFO("Success: OTA completed\n");
            rc = 0;
        }
    }
end:
    if (token) {
        free(token);
    }
    if (serial_number) {
        free(serial_number);
    }
    if (client_id) {
        free(client_id);
    }
    if (auth_key) {
        free(auth_key);
    }
    return rc;
}
