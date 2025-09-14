#pragma once
#define _GNU_SOURCE
#include <linux/ioctl.h>

#ifndef __KERNEL__

#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#endif

#define RETBLEED_ORACLE_IOCTL_MAGIC 0x1337

struct retbleed_oracle_ioctl_info_args {
    size_t size;                                    ///< Size of the struct. Set to sizeof(struct retbleed_oracle_ioctl_info_args)
    uintptr_t* leak_gadget_address;                 ///< A pointer to the leak gadget function address
    uintptr_t* spec_gadget_return_address;          ///< A pointer to the return address of the speculative gadget
    uintptr_t* spec_gadget_recursive_call_address;  ///< A pointer to the recursive call address of the speculative gadget
};

struct retbleed_oracle_ioctl_run_func_args {
    size_t size;        ///< Size of the struct. Set to sizeof(struct retbleed_oracle_ioctl_run_func_args)
    uint64_t options;   ///< Options. Set to 0 or ASSIGNMENT_OPTION_* flags (ZERO points in the latter case)
    uintptr_t arg1;     ///< The first argument
    uintptr_t arg2;     ///< The second argument

};

struct retbleed_oracle_ioctl_initialize_secret_args {
    size_t size;                                        // Size of the struct. Set to sizeof(struct retbleed_oracle_ioctl_initialize_secret_args)
    uint64_t   feature_flags;                           // Feature flags. Set to zero or ASSIGNMENT_OPTION_* flags (ZERO points in the latter case)
    uintptr_t* flush_reload_buffer_virtual_address;     // Input: A pointer to the user-space address of the reload buffer, output: The pointer is filled with the kernel-virtual (pagemap) address of the reload buffer)
    uintptr_t* flush_reload_buffer_physical_address;    // Output: The pointer is filled with the physical address of the reload buffer
    uintptr_t* secret_kernel_address;                   // Output: The address of the kernel secret structure (kernel address - do not access directly from user space)
};

struct retbleed_oracle_ioctl_verify_secret_args {
    size_t size;                                        // Size of the struct. Set to sizeof(struct retbleed_oracle_ioctl_verify_secret_args)
    uint64_t* points;                                   // Output: The points that will be awarded for this secret. May be NULL. The grade may or may not depend on this.
    uint8_t secret_bytes[128];                          // The secret bytes to verify
};

/* IOCTL definitions. You may use them directly, but we provide you with the required functions below */
#define RETBLEED_ORACLE_IOCTL_INFO _IOWR(RETBLEED_ORACLE_IOCTL_MAGIC, 0, struct retbleed_oracle_ioctl_info_args)
#define RETBLEED_ORACLE_IOCTL_RUN_FUNC _IOW(RETBLEED_ORACLE_IOCTL_MAGIC, 1, struct retbleed_oracle_ioctl_run_func_args)
#define RETBLEED_ORACLE_IOCTL_INITIALIZE_SECRET _IOWR(RETBLEED_ORACLE_IOCTL_MAGIC, 2, struct retbleed_oracle_ioctl_initialize_secret_args)
#define RETBLEED_ORACLE_IOCTL_VERIFY_SECRET _IOWR(RETBLEED_ORACLE_IOCTL_MAGIC, 3, struct retbleed_oracle_ioctl_initialize_secret_args)


/* Assignment modifiers. Use with EXTREME caution - basically, if any of these is used to create the secret you provide us, you get zero points.
However, we provide them to you to ensure that you can test parts of your code while other parts are not yet working as intended.

Note that if you plan to use ANY of them, you MUST pass the same flags when creating a secret, or the called functions will return -EINVAL.
Also, DO NOT try to enumerate the flags. They are not enumerable, and you will run into undefined behavior if you try to do so.
*/

/* This option enables architectural calls (instead of speculative calls).
Use as an argument to oracle_run_func() to ensure that the secret area (and only the secret area) is accessed architecturally,
_guaranteeing_ that you should see a signal if your flush+reload code is correct.
*/
#define ASSIGNMENT_OPTION_USE_ARCHITECTURAL_CALL (0xac << 0) /* 0b10101100*/

/* This option disables function location randomization.
For this assignment, we randomize the function addresses every time the module is opened.
A well-behaving program can handle this by calling oracle_get_info() after opening the oracle and acting accordingly.
However, if you want to remove any uncertainties, you can set this option when creating the secret to force page-aligned functions.
Keep in mind that you need to call get_info() again if you use this option to obtain the new function addresses, and of course: No points.
*/
#define ASSIGNMENT_OPTION_NO_RANDOM_POSITION (0x12 << 0) /* 0b00010010*/


#ifndef __KERNEL__

/**
 * @brief Open the oracle device.
 */
static inline int oracle_open(void) {
    return open("/dev/retbleed_oracle", O_RDONLY | O_CLOEXEC);
}

/**
 * @brief Obtain information from the oracle.
 * @note This must be called after opening the oracle. The oracle may randomize on open - do not hard-code these addresses!
 * @param oracle_fd The file descriptor of the oracle device previously opened with `oracle_open()`
 * @param leak_gadget_address [out] A storage location for the leak gadget address
 * @param spec_gadget_return_address [out] A storage location for the speculative gadget return address
 * @param spec_gadget_recursive_call_address [out] A storage location for the speculative gadget recursive call address
 * @return 0 on success. -EACCESS if any of the target addresses could not be read by the kernel module.
 */
static inline int oracle_get_info(
    int oracle_fd,
    uintptr_t* leak_gadget_address,
    uintptr_t* spec_gadget_return_address,
    uintptr_t* spec_gadget_recursive_call_address
) {
    struct retbleed_oracle_ioctl_info_args args = {
        .size = sizeof(args),
        .leak_gadget_address = leak_gadget_address,
        .spec_gadget_return_address = spec_gadget_return_address,
        .spec_gadget_recursive_call_address = spec_gadget_recursive_call_address
    };
    return ioctl(oracle_fd, RETBLEED_ORACLE_IOCTL_INFO, &args);
}

/**
 * @brief Run the kernel module function to fill the RSB.
 * @param oracle_fd The file descriptor of the oracle device previously opened with `oracle_open()`
 * @param arg1 The first argument to the function
 * @param arg2 The second argument to the function
 * @param options Options. Set to 0 unless explicitly instructed otherwise.
 */
static inline int oracle_run_func(int oracle_fd, uintptr_t arg1, uintptr_t arg2, uint64_t options) {
    struct retbleed_oracle_ioctl_run_func_args args = {
        .size = sizeof(args),
        .options = options,
        .arg1 = arg1,
        .arg2 = arg2,
    };
    return ioctl(oracle_fd, RETBLEED_ORACLE_IOCTL_RUN_FUNC, &args);
}

/**
 * @brief Initialize the secret oracle at a given address
 * @param oracle_fd The file descriptor of the oracle device previously opened with `oracle_open()`
 * @param virtual_reload_buffer_address [in/out] The address of the reload buffer to use. If NULL, the oracle will allocate a buffer. The buffer must be a (transparent) huge page, or the call will fail with -EINVAL. If successful, the value is updated with the kernel virtual address
 * @param physical_reload_buffer_address [out] The physical address of the reload buffer hugepage.
 * @param secret_address The kernel-side address where the secret resides.
 * @param feature_flags The feature flags to use. Set to zero unless instructed otherwise for debugging.
 * @note If you provide NULL, you will not receive full points for the assignment.
 */
static inline int oracle_initialize_secret(int oracle_fd,
    uintptr_t* virtual_reload_buffer_address, uintptr_t* physical_reload_buffer_address,
    uintptr_t* secret_address,
    uint64_t feature_flags) {
    struct retbleed_oracle_ioctl_initialize_secret_args args = {
        .size = sizeof(args),
        .feature_flags = feature_flags,
        .secret_kernel_address = secret_address,
        .flush_reload_buffer_virtual_address = virtual_reload_buffer_address,
        .flush_reload_buffer_physical_address = physical_reload_buffer_address,
    };
    return ioctl(oracle_fd, RETBLEED_ORACLE_IOCTL_INITIALIZE_SECRET, &args);
}

/**
 * @brief Verify a secret with the oracle.
 * @param oracle_fd The file descriptor of the oracle device previously opened with `oracle_open()`
 * @param secret_bytes The secret bytes to verify
 * @param secret_length The length of the secret bytes. Must be 128 for this assignment
 * @param points [out] A pointer to a storage location to store the (fantasy) points awarded for this secret.
 */
static inline int oracle_verify_secret(int oracle_fd, const uint8_t* secret_bytes, size_t secret_length, uint64_t* points)
{
    struct retbleed_oracle_ioctl_verify_secret_args args = {
        .size = sizeof(args),
        .points = points
    };
    if (secret_length != sizeof(args.secret_bytes)) {
        return -1;
    }
    memcpy(args.secret_bytes, secret_bytes, sizeof(args.secret_bytes));
    long r = ioctl(oracle_fd, RETBLEED_ORACLE_IOCTL_VERIFY_SECRET, &args);
    return r;
}

#endif