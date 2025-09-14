#include "blacksmith.h"
#include "compiler.h"
#include "escalate_privileges.h"
#include "evict.h"
#include "massage.h"
#include "memory.h"
#include "stdint.h"
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* A5 TASK 5: Corrupt a page table and print its address. */
static int only_pt = 0;

/* clang-format off */
static struct option long_opts[] = {
    {"only-pt", no_argument, 0, 'p'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}};
/* clang-format on */

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt_long(argc, argv, "ph", long_opts, NULL)) != -1) {
        switch (opt) {
        case 0: {
        } break;
        case 'p': {
            /* A5 TASK 5 */
            only_pt = 1;
        } break;
        case 'h':
            fprintf(stderr, "Usage: %s [--only-pt]\n", argv[0]);
            fprintf(
                stderr,
                "  --only-pt print page table physical address, clean up, and exit\n");
            exit(0);
        }
    }
    /* A5 TASK 1 allocate hammer area; initialize blacksmith */
    // void *mem = allocate_contiguous_blocks(...); */
    // init_blacksmith(&bs, mem)

    /* A5 TASK 2 allocate hammer area */

    /* A5 TASK 6 flip the file pte to reference page table and gain phys read/write
     * primitive. */
    if (only_pt) {
        printf("PT=%012lx\n", _ul(0x1337000));
        return 0;
    }

    /* A5 TASK 7 get root */

    return 0;
}
