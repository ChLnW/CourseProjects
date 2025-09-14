## DRAM project boilerplate
To help you move fast, we give you some boilerplate code for assignment 4 and 5. 
We hope it can assist you, but we are also keen to hear about where diverged from the
template. As disclaimer, this is the first time we run this assignment, so please _do_
question what you're seeing _do not_ hesitate to re-design the boilerplate code if desired. 

### Building

```sh
# build all  
make
# build and force dram configuration, conveniently in include/dram_info.h
make DRAM_INFO=SOME_CONFIG 
```

The Makefile expects to build on the target compute node.
Therefore, you could write a small shell script to build and run your current
task on the same compute node. For example `my_script.sh`:

```sh
#!/bin/bash
make 
./test__blacksmith -e
```

Then use `srun ./my_script.sh` to run both things on the compute node.

### Keep this git remote

Keep the git remote set up. For assignment 5, we may push some boilerplate files to
help you get started. 

```sh
# Assuming `origin` is currently comsec-edu, rename it to something else
git remote rename origin comsec-edu
# Assign your own origin
git remote add origin git@gitlab.ethz.ch...
```
