#define WOM_MAGIC_NUM   0x1337
#define WOM_GET_ADDRESS _IOR(WOM_MAGIC_NUM, 0, unsigned long)
#define WOM_PATH        "/dev/wom"

#ifndef __KERNEL__
#include <fcntl.h>
#include <sys/ioctl.h>

static inline int wom_open() {
    return open("/dev/wom", O_RDONLY);
}

static inline int wom_get_address(int fd, void **addr) {
    return ioctl(fd, WOM_GET_ADDRESS, addr);
}

#endif
