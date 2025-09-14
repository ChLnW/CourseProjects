#define pr_fmt(fmt) "pgtrace: " fmt

#include "./pgtrace_ioc.h"
#include <asm-generic/memory_model.h>
#include <linux/kprobes.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

/** This is the single page that we're tracing. */
static unsigned long pgtrace_attached;

static struct proc_dir_entry *procfs_file;
static struct pgtrace_ioc_msg msg;

/* get_pfnblock_flags_mask is not exported for kernel modules, so we will find the address
 * of its symbol using kallsyms_lookup_name and assign it to this function pointer. */
unsigned long (*unexp__get_pfnblock_flags_mask)(const struct page *page,
                                                unsigned long pfn, unsigned long mask);

void find_syms(void) {
    /* kallsyms_lookup_name is not exported, so we find it temporarily attaching a kprobe
     * to it */
    unsigned long (*kallsyms_lookup_name)(const char *name);
    struct kprobe kp = {.symbol_name = "kallsyms_lookup_name"};
    register_kprobe(&kp);
    kallsyms_lookup_name = (void *) kp.addr;
    unregister_kprobe(&kp);
    unexp__get_pfnblock_flags_mask =
        (void *) kallsyms_lookup_name("get_pfnblock_flags_mask");
}

/* Translate a user virtual address to physical one */
unsigned long translate(unsigned long uaddr) {
    unsigned long pfn = 0;
    struct page *p;

    pagefault_disable();
    if (get_user_page_fast_only(uaddr, 0, &p)) {
        pfn = page_to_pfn(p);
        put_page(p);
    }
    pagefault_enable();
    return pfn << 12;
}

static long handle_ioctl(struct file *filp, unsigned int request, unsigned long argp) {
    long result = 0;
    if (copy_from_user(&msg, (void *) argp, sizeof(msg)) != 0) {
        pr_err("Cannot copy from user\n");
        return -EINVAL;
    }

    switch (request) {
    case PGTRACE_IOC_ATTACH: {
        pgtrace_attached = translate((long) msg.va);
    } break;
    case PGTRACE_IOC_MT: {
        struct page *pg = pfn_to_page(pgtrace_attached >> PAGE_SHIFT);
        result = unexp__get_pfnblock_flags_mask(pg, page_to_pfn(pg), MIGRATETYPE_MASK);
        if (copy_to_user((void *) argp, &msg, sizeof(msg))) {
            pr_err("Cannot copy to user\n");
            return -EINVAL;
        }
    } break;
    case PGTRACE_IOC_EQ: {
        char *physmap = (void *) PAGE_OFFSET;
        result = *(long *) &physmap[pgtrace_attached + msg.pgoff] == msg.val;
    } break;
    default:
        return -ENOIOCTLCMD;
    }

    return result;
}

int pgtrace_open(struct inode *inode, struct file *filp) {
    int res = generic_file_open(inode, filp);
    pgtrace_attached = 0;
    return res;
}

static struct proc_ops pops = {
    .proc_ioctl = handle_ioctl,
    .proc_open = pgtrace_open,
    .proc_lseek = no_llseek,
};

static int pgtrace_init(void) {
    procfs_file = proc_create(PROC_PGTRACE, 0, NULL, &pops);
    find_syms();
    pr_info("init\n");
    return 0;
}

static void pgtrace_exit(void) {
    pr_info("exit\n");
    proc_remove(procfs_file);
}

module_init(pgtrace_init);
module_exit(pgtrace_exit);
MODULE_LICENSE("GPL");
