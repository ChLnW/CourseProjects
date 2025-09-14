#pragma once

/** Defines the DRAM address information. */
typedef struct dram_info {
  /* The number of bank selection functions. */
  int nbank_sel_fn;
  /* The bank selection functions. */
  unsigned long bank_sel_fn[16];
  /* The row selection bits. */
  unsigned long row_mask;
} dram_info_t;

/* A4 TASK 6. Define to use to use for fuzzing evaluation and later
 * exploitation. */
#define UNKNOWN_CONFIG                                                         \
  _Static_assert(0, "UNKNOWN_CONFIG is not working. Please define a valid "    \
                    "configuration in dram_info.h.")

/* This is an example showing how you should define the DRAM address
 * configuration. */
#define SOME_CONFIG                                            \
  {2,                                                          \
   {0x1001000, 0x200200},                                      \
   0x0fffffffff}

#define DRAM__cn001 UNKNOWN_CONFIG
#define DRAM__cn002 UNKNOWN_CONFIG
#define DRAM__cn003 UNKNOWN_CONFIG
#define DRAM__cn004 UNKNOWN_CONFIG
#define DRAM__cn005 UNKNOWN_CONFIG
#define DRAM__cn006 UNKNOWN_CONFIG
#define DRAM__cn007 UNKNOWN_CONFIG
#define DRAM__cn008                                            \
  {5,                                                          \
   {0x2040, 0x44000, 0x88000, 0x110000, 0x220000},             \
   0x3ffc0000}
#define DRAM__cn009 UNKNOWN_CONFIG
#define DRAM__cn010                                            \
  {4,                                                          \
   {0x2040, 0x24000, 0x48000, 0x90000},                        \
   0x3ffe0000}
#define DRAM__cn011 UNKNOWN_CONFIG
#define DRAM__cn012 UNKNOWN_CONFIG
#define DRAM__cn013 UNKNOWN_CONFIG
#define DRAM__cn014 UNKNOWN_CONFIG
#define DRAM__cn015                                            \
  {5,                                                          \
   {0x2040, 0x44000, 0x88000, 0x110000, 0x220000},             \
   0x3ffc0000}
#define DRAM__cn016 UNKNOWN_CONFIG
#define DRAM__cn017                                            \
  {4,                                                          \
   {0x2040, 0x24000, 0x48000, 0x90000},                        \
   0x3ffe0000}
#define DRAM__cn018 UNKNOWN_CONFIG
#define DRAM__cn019 UNKNOWN_CONFIG
#define DRAM__cn020 UNKNOWN_CONFIG
#define DRAM__cn021 UNKNOWN_CONFIG
#define DRAM__cn022 UNKNOWN_CONFIG
#define DRAM__cn023 UNKNOWN_CONFIG
#define DRAM__cn024 UNKNOWN_CONFIG

/**
 * We set -DDEFAULT_DRAM_INFO=DRAM__<your-host-prefix>.
 * We may already set this to an appropriate value from Makefile.
 */
#ifndef DEFAULT_DRAM_INFO
#define DEFAULT_DRAM_INFO {0, {0}, 0}
#endif
