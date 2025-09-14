#include "dram_info.h"
#include "drama.h"
#include <fstream>
#include <map>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>
static char hostname[64];
static int print_addressing_bits = 0;
static int print_bank_fns = 0;
static int print_row_mask = 0;

static dram_info_t dram;

bool read_dram_banks_csv(const char *hostname, const std::string &filename,
                         unsigned long &threshold, size_t &nbanks) {
  std::ifstream file(filename);
  std::string line;

  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, ',')) {
      tokens.push_back(token);
    }

    if (tokens.size() >= 3 && tokens[0] == hostname) {
      try {
        threshold = std::stoul(tokens[1]);
        nbanks = std::stoul(tokens[2]);
        return true;
      } catch (...) {
        return false;
      }
    }
  }

  return false;
}

size_t find_bank_bits(std::vector<std::vector<char *>> &conflict_sets,
                      unsigned long threshold) {
  // std::vector<size_t> col_bits;
  // col_bits.reserve(conflict_sets.size());
  std::vector<size_t> bank_bits;
  bank_bits.reserve(conflict_sets.size());
  for (auto &set : conflict_sets) {
    char *a0 = set[0];
    // size_t col_bit = 0;
    size_t bank_bit = 0;
    for (size_t mask = 1; mask < (1 << 30); mask <<= 1) {
      char *a1 = (char *)(((size_t)a0) ^ mask);
      if (!is_bank_conflict(threshold, a0, a1)) {
        char *a2 = set[1];
        // if (is_bank_conflict(threshold, a1, a2)) {
        //   col_bit ^= mask;
        // } else {
        //   bank_bit ^= mask;
        // }
        if (!is_bank_conflict(threshold, a1, a2)) {
          bank_bit ^= mask;
        }
      }
    }
    // col_bits.push_back(col_bit);
    bank_bits.push_back(bank_bit);
  }

  // light-weight pseudo argmax
  size_t bits = bank_bits[0];
  int count = 0;
  for (auto &b : bank_bits) {
    if (b == bits) {
      ++count;
    } else if (count == 0) {
      bits = b;
      count = 1;
    } else {
      --count;
    }
  }
  // for (size_t i=0; i<conflict_sets.size(); ++i) {
  //   printf("%2lu: 0x%012lx\t0x%012lx\n", i, bank_bits[i], col_bits[i]);
  // }
  return bits;
}

int idx_xor(size_t addr, int i1, int i2) {
  return ((addr >> i1) ^ (addr >> i2)) & 1;
}

std::map<unsigned int, unsigned int>
find_bank_fns(size_t bits, std::vector<std::vector<char *>> &conflict_sets) {
  dram.nbank_sel_fn = 0;
  std::vector<unsigned int> bitPos;
  for (size_t i = bits, p = 0; i != 0;) {
    if (i & 1) {
      dram.nbank_sel_fn += 1;
      bitPos.push_back(p);
      // printf("%lu,", p);
    }
    ++p;
    i >>= 1;
  }
  dram.nbank_sel_fn /= 2;
  // printf("\nnbank_fn: %d\n", dram.nbank_sel_fn);

  std::map<unsigned int, unsigned int> res;
  for (size_t i = 0, f = 0; i < bitPos.size() - 1; ++i) {
    const unsigned int p1 = bitPos[i];
    if (res.find(p1) != res.end()) {
      continue;
    }
    for (size_t j = i + 1; j < bitPos.size(); ++j) {
      const unsigned int p2 = bitPos[j];
      if (res.find(p2) != res.end()) {
        continue;
      }
      bool toAdd = true;
      int fail1 = conflict_sets.size() / 8;
      for (auto &set : conflict_sets) {
        // bool same = true;
        int fail2 = set.size() / 4;
        int b = idx_xor((size_t)set[0], p1, p2);
        for (size_t k = 1; k < set.size(); ++k) {
          if (b != idx_xor((size_t)set[k], p1, p2)) {
            // same = false;
            --fail2;
            // fn deemed valid if works for more than 3/4 addrs of 1 conflict
            // set some redundency introduced to tolerate incorrect conflict
            // sets resulting from noise.
            if (fail2 < 0)
              break;
          }
        }
        // if (!same) {
        if (fail2 < 0) {
          --fail1;
          // fn deemed valid if works for more than 7/8 of conflict sets
          // some redundency introduced to tolerate incorrect conflict sets
          // resulting from noise.
          if (fail1 < 0) {
            toAdd = false;
            break;
          }
        }
      }
      if (toAdd) {
        res[p1] = p2;
        res[p2] = p1;
        dram.bank_sel_fn[f] = (1 << p1) | (1 << p2);
        ++f;
      }
    }
  }

  return res;
}

int main(int argc, char *argv[]) {
  gethostname(hostname, 64);

  int opt;
  while ((opt = getopt(argc, argv, "hbfm")) != -1) {
    switch (opt) {
    case 'b': {
      print_addressing_bits = 1;
    } break;
    case 'f': {
      print_bank_fns = 1;
    } break;
    case 'm': {
      print_row_mask = 1;
    } break;
    case 'h':
    default:
      fprintf(stderr, "Usage: %s [options]\n", argv[0]);
      fprintf(
          stderr,
          "Outputs physical address bits used in DRAM row addressing.\n"
          "  -b     Print a bit mask of the bits used in bank addressing.\n"
          "  -f     Print the DRAM bank functions that selects bank in hex.\n"
          "  -m     Print the row mask.\n"
          "  -h     Help\n");
      exit(1);
    }
  }

  char *buffer = (char *)mmap(NULL, (1 << 30), PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANON | MAP_HUGETLB, -1, 0);
  if (buffer == MAP_FAILED) {
    perror("mmap failed");
    return 1;
  }

  // defaults
  unsigned long threshold = 400;
  size_t nbanks = 16;
  if (!read_dram_banks_csv(hostname, "./data/dram_banks.csv", threshold,
                           nbanks)) {
    printf("No matching hostname found in CSV. Using default values.\n");
  }

  std::vector<std::vector<char *>> conflict_sets =
      build_conflict_sets(buffer, threshold, 33, 5);

  size_t bits = find_bank_bits(conflict_sets, threshold);

  if (print_addressing_bits) {
    /* A4 TASK 3. Detect bank selection mask. */

    printf("%012lx, ", bits);
  }

  auto fns = find_bank_fns(bits, conflict_sets);

  if (print_bank_fns) {
    /* A4 TASK 4. Detect bank selection functions. */
    for (int i = 0; i < dram.nbank_sel_fn; ++i) {
      printf("%012lx ", dram.bank_sel_fn[i]);
    }
  }

  size_t a = (size_t)buffer;
  for (unsigned int i = 12; i < 30; ++i) {
    size_t mask = 1 << i;
    if (fns.find(i) != fns.end()) {
      if (i < fns[i])
        continue;
      mask |= 1 << fns[i];
    }
    char *a1 = (char *)(a ^ mask);
    if (is_bank_conflict(threshold, (char *)a, a1)) {
      dram.row_mask |= 1 << i;
    }
  }

  if (print_row_mask) {
    /* A4 TASK 5. Detect the row selection mask. */
    printf(", %016lx\n", dram.row_mask);
  } else {
    printf("\n");
  }

  return 0;
}
