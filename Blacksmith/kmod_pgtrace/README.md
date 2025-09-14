## pgtrace

This is a very simple module that exposes a procfs file `/proc/pgtrace`. The module is
installed on all compute nodes. Its purpose is to keep track of a physical page after it
has been reclaimed. For simplicity, it can only keep track of one single page. For this
page, can see (1) its associated migrate type and (2) whether it contains a certain 8-byte
value at a given page offset,  This module can be interacted with through the following
`ioctl` system calls:

- `PGTRACE_IOC_ATTACH`: Takes a virtual page-aligned address and assigns the currently
    backing physical page to be traced.

- `PGTRACE_IOC_MT`: Returns the currently assigned migratetype for the pageblock of the currently assigned page. The pageblock's migratetype decides what type of purpose the pages in this pageblock can have. As such, individual pages in the pageblock may be associated with a different migratetype while they are in use or in a pcp list.

- `PGTRACE_IOC_EQ`: Returns 1 if the the given 8-byte value is equal to the value at the given page offset of the currently attached page.

## test

A simple test that demonstrates how the interaction with the module works is available
in [./test.c](./test.c).

Build and run it like this:

```
make test
./pgtrace
```
