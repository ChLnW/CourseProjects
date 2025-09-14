#pragma once
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

/* A5 TASK 3 */
void map_file(void *unflipped_ptr);

/* A5 TASK 4 */
void map_pt(void *target, void *block, long spray_offset);
