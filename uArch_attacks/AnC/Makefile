CC := gcc
CFLAGS := -Wall -fPIC
CFLAGS += -march=skylake # incl. skylake-only feats.

# https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
CFLAGS += -O1 # tweak as you like
CFLAGS += -pipe -MP -MMD

CWD  := $(shell pwd)
SRCS := $(shell find . -name \*.c)
OBJS := $(SRCS:.c=.o)

ANC_OBJS          := $(filter-out ./test__%.o, $(OBJS))
TEST_CACHE_OBJS   := $(filter-out ./anc.o ./test__evict_tlb.o, $(OBJS))
TEST_TLB_OBJS     := $(filter-out ./anc.o ./test__evict_cache.o, $(OBJS))
TARGETS           := anc test__evict_tlb test__evict_cache
GROUP_NAME        := $(shell sed -n 's/^GROUP_NAME= *\([^ ]*\).*/\1/p' report.txt)

all: $(TARGETS)

anc: $(ANC_OBJS)
test__evict_tlb: $(TEST_TLB_OBJS)
test__evict_cache: $(TEST_CACHE_OBJS)

$(TARGETS):
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

DEPFILES := $(OBJS:.o=.d)
-include $(wildcard $(DEPFILES))

.PHONY: submission
submission: clean
	-mkdir -m 0700 /tmp/$(USER)
	tar czvf /tmp/$(USER)/$(GROUP_NAME).tar.gz --exclude-vcs --exclude-vcs-ignores -C $(shell dirname $(CWD)) $(shell basename $(CWD))
	mv /tmp/$(USER)/$(GROUP_NAME).tar.gz .
	-rmdir /tmp/$(USER)


.PHONY: clean
clean:
	-@rm -f $(ANC_OBJS) $(TEST_TLB_OBJS) $(TEST_CACHE_OBJS) $(TARGETS) $(DEPFILES)

