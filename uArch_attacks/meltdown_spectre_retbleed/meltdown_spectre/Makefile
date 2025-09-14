CC := gcc
CFLAGS += -O3 # -O3 is aggressive but produces clean assembly
CFLAGS += -mrtm # TSX
CFLAGS += -pipe -MP -MMD
CFLAGS += -march=skylake # incl. skylake-only feats.

TARGETS    := meltdown_segv meltdown_tsx meltdown_spectre
SRCS       := $(shell find . -not -ipath "./wom/*" -name \*.c)
OBJS       := $(filter-out ./main.o, $(SRCS:.c=.o))
MAIN_OBJS  := $(TARGETS:=.o)

ifneq ($(V), 1)
VERBOSE=@
endif

all: $(TARGETS)

meltdown_tsx: CFLAGS += -DPF_SUPPRESS=TSX
meltdown_segv: CFLAGS += -DPF_SUPPRESS=SEGV
meltdown_spectre: CFLAGS += -DPF_SUPPRESS=SPECTRE
$(TARGETS): %: %.o $(OBJS)
	@echo LD $@
	$(VERBOSE) $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(MAIN_OBJS): main.c
	@echo CC $@
	$(VERBOSE) $(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	@echo CC $@
	$(VERBOSE) $(CC) $(CFLAGS) -c $< -o $@

DEPFILES := $(MAIN_OBJS:.o=.d) $(OBJS:.o=.d)
-include $(wildcard $(DEPFILES))


.PHONY: clean
clean:
	@echo "CLEAN"
	-$(VERBOSE)rm -f $(OBJS) $(MAIN_OBJS) $(TARGETS) $(DEPFILES) log
