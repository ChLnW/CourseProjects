#include "drama.h"
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

#define NUM_TRIALS 50000

static unsigned long threshold = 0;
static int output_banks = 0;

static char hostname[64];

struct access_time {
  char *address;
  unsigned long time;
};

int main(int argc, char *argv[]) {
  gethostname(hostname, 64);

  int opt;
  while ((opt = getopt(argc, argv, "hbt:")) != -1) {
    switch (opt) {
    case 'b': {
      // maybe implement this automatic threshold thing idk
      output_banks = 1;
    } break;
    case 't': {
      char *endptr;
      threshold = strtoul(optarg, &endptr, 10);
      if (*endptr != '\0') {
        fprintf(stderr, "Invalid threshold value: %s\n", optarg);
        exit(EXIT_FAILURE);
      }
      output_banks = 1;
    } break;
    case 'h':
    default:
      fprintf(stderr, "Usage: %s [options]\n", argv[0]);
      fprintf(stderr,
              "Outputs a CSV list of timings for various memory addresses a,"
              "against a 'base' address (a*).\n"
              "  -b     Outputs #banks and the dynamically set bank-conflict "
              "threshold in a CSV row.\n"
              "  -t <t> Same as -b, but using a fixed threshold t.\n"
              "  -h     Help\n");
      exit(0);
    }
  }

  char filename[128];

  char *buffer = (char *)mmap(NULL, (1 << 30), PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANON | MAP_HUGETLB, -1, 0);
  if (buffer == MAP_FAILED) {
    perror("mmap failed");
    return 1;
  }

  srand((unsigned int)time(NULL));
  if (!output_banks) {
    

    char *a_base = pick_random_address(buffer);

    std::vector<access_time> times;
    times.push_back(access_time{a_base, time_addresses(a_base, a_base)});

    for (int i = 0; i < NUM_TRIALS; i++) {
      char *a;
      // bool found = false;
      // do {
      //   // pick a random NEW address
      //   a = pick_random_address(buffer);
      //   for (const auto &entry : times) {
      //     if (entry.address == a) {
      //       found = true;
      //     }
      //   }
      // } while (found);

      a = pick_random_address(buffer);
      times.push_back(access_time{a, time_addresses(a_base, a)});
      
    }
    snprintf(filename, sizeof(filename), "./data/%s__drama.csv", hostname);
    FILE *access_times_file;
    while (true) {
      access_times_file = fopen(filename, "w");
      // if (!access_times_file) {
      //   perror("Failed to open access time file for writing");
      //   return 1;
      // }
      if (access_times_file) {
        break;
      }
    }
    fprintf(access_times_file, "a*,a,timing\n");
    for (auto &t: times) {
      fprintf(access_times_file, "%p,%p,%lu\n", a_base, t.address, t.time);
    }
  } else {
    // printf("%d", RAND_MAX);
    std::vector<std::vector<char *>> conflict_sets =
        build_conflict_sets(buffer, threshold, 64, 5);
    
    FILE *banks_file;
    while (true) {
      banks_file = fopen("./data/dram_banks.csv", "a");
      if (banks_file) {
        break;
      }
    }
    
    fprintf(banks_file, "%s, %ld, %zu\n", hostname, threshold,
            conflict_sets.size());

    // for (size_t i = 0; i < conflict_sets.size(); ++i) {
    //   for (size_t j = 0; j < conflict_sets[i].size(); ++j) {
    //     fprintf(banks_file, "%zu, %p, %zu\n", i, conflict_sets[i][j], j);
    // }
    // }
  }

  if (munmap(buffer, (1 << 30)) == -1) {
    perror("munmap failed");
    return 1;
  }

  fcloseall();
  return 0;
}
