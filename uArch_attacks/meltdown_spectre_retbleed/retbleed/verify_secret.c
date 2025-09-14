#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <pwd.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include "retbleed_oracle.h"
#include <unistd.h>
#include <fcntl.h>

int parse_hex_string(const char* str_in, uint8_t* out_buf, size_t* maxout)
{
    assert(maxout);
    assert(str_in);
    assert(out_buf);
    size_t consumed = 0;
    size_t written = 0;
    uint8_t interpreted = 0;
    uint8_t byte = 0;
    while (*str_in) {
        if (*str_in == '\n' || written >= *maxout) break;

        char c = tolower(*str_in);
        if ('0' <= c && c <= '9') {
            c -= '0';
            byte = (byte << 4) | c;
            ++interpreted;
        } else if ('a' <= c && c <= 'f') {
            c = c - 'a' + 10;
            byte = (byte << 4) | c;
            ++interpreted;
        } else {
            // Pass
        }
        if (interpreted == 2) {
            out_buf[written] = byte;
            ++written;
            interpreted = 0;
            byte = 0;
        }

        ++consumed;
        ++str_in;
    }
    *maxout = written;
    return 0;
}

#define SECRET_VALUE(secret, offset, type) (*(type*)&(secret)[(offset)])

#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_RESET "\033[0m"

int main(int argc, char* argv[])
{
    (void)argc;
    /* Read the secret from stdin */
    int use_module = 1;
    while (*++argv) {
        if (strcmp(*argv, "--no-module") == 0) {
            use_module = 0;
        } else {
            printf("Unknown argument: %s\n", *argv);
            return -1;
        }
    }

    char* buf = malloc(4096);
    assert(buf);

    int oracle_fd = -1;
    if (use_module) {
        oracle_fd = oracle_open();
        if (oracle_fd < 0) {
            fprintf(stderr, "Could not open oracle. Error code %d\n", errno);
            use_module = 0;
        }
    }

    struct {
        uint8_t nonce[16];
    } nonces[16];
    size_t nonce_it = 0;
    memset(nonces, 0, sizeof(nonces));

    while (fgets(buf, 4096, stdin) != NULL) {
            
        
        // From the secret (as a string), we should convert to bytes
        uint8_t secret[128];
        size_t secret_len = sizeof(secret);

        if (parse_hex_string(buf, secret, &secret_len)) {
            fprintf(stderr, "Failed to parse secret\n");
            return 1;
        }
        if (secret_len != sizeof(secret)) {
            // Empty lines may trigger this
            if (secret_len > 0) {
                fprintf(stderr, "Secret length is not 128 bytes - found only %ld bytes - checking next\n", secret_len);
            }
            continue;
        }

        char hostname[32];
        // We can now check if the secret matches the parameters
        uint64_t timestamp = SECRET_VALUE(secret, 0, uint64_t);
        uint32_t uid = SECRET_VALUE(secret, 8, uint32_t);
        strncpy(hostname, (char*)&secret[32], 32);

        int nonce_fresh = 1;
        memcpy(nonces[nonce_it].nonce, &secret[80], sizeof(nonces[nonce_it].nonce));
        for (size_t n = 0; n < 16; ++n) {
            if (memcmp(nonces[n].nonce, nonces[nonce_it].nonce, sizeof(nonces[n].nonce)) == 0 && n != nonce_it) {
                // Mismatch
                nonce_fresh = 0;
                break;
            }
        }
        ++nonce_it;

        uint64_t feature_flags = SECRET_VALUE(secret, 64, uint64_t);

        struct tm *time_info = localtime((time_t*)&timestamp);
        char buffer[80];
        strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", time_info);

        // Convert uid to user name
        struct passwd* pwd = getpwuid(uid);
        const char* username = "unknown";

        if (pwd) {
            username = pwd->pw_name;
        }

        int verified = -1;
        if (use_module) {
            // Call to the module to verify the signature
            uint64_t points = 0;
            int ret = oracle_verify_secret(oracle_fd, secret, sizeof(secret), &points);
            if (ret == 0) {
                verified = 1;
            }
            else {
                //printf("Error during verification: %d (errno %d)\n", ret, errno);
                verified = 0;
            }
            if (feature_flags != 0) {
                verified = (verified) ? 2 : 3;
            }
        }

        printf("[+] Secret: Created at %s by user %s (uid %d) on host %s features 0x%02lx %s\n",
            buffer, username, uid, hostname, feature_flags, 
            (nonce_fresh == 0) ?
                ANSI_RED "DUPLICATE" ANSI_RESET :
            (verified == 0) ? 
                ANSI_RED "TAMPERED" ANSI_RESET : 
            (verified == 1) ? 
                ANSI_GREEN "VERIFIED" ANSI_RESET : 
            (verified == 2) ? 
                ANSI_RED "VCHEATS" ANSI_RESET : 
            (verified == 3) ? 
                ANSI_RED "FCHEATS" ANSI_RESET : 
                ANSI_YELLOW "UNKNOWN" ANSI_RESET
            );
    }
    return 0;
}