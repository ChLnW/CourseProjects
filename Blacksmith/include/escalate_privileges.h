#include <stdint.h>

/* An empty PTE with NX(b63), ACCESSED(b5), USER(b2), RW(b1), PRESENT(b0) flags. */
#define PTE_EMPTY 0x8000000000000027UL

/* Mask to mask off the PTE flags and just get the PFN bits. */
#define PTE_PFN_MASK 0xffffffffff000UL

/* Taken from include/linux/cred.h. You want to find something like this in 
 * physical memory that corresponds to your current process (task) to elevate 
 * privileges. Linux can reuse the cred struct for many purposes. */
struct cred {
    int usage; /* This object is ref-counted so this can be any value > 0. */
    int uid;   /* Real UID of the task. */
    int gid;   /* Real GID of the task. */
    int suid;  /* Saved UID of the task. */
    int sgid;  /* Saved GID of the task. */
    int euid;  /* Effective UID of the task. */
    int egid;  /* Effective GID of the task. */
    int fsuid; /* UID for VFS ops. */
    int fsgid; /* GID for VFS ops. */
};

/* A5 TASK 7 */
/* Scan physical memory for the credentials of our process. Once found, set them to root.
 * When this function returns we should have root. All that is left is to restore the fake
 * mappings and pop a root shell. */
int escalate_privileges(char *corrupt_mapping);
