CC         := g++
CXXFLAGS   := -D_GNU_SOURCE -Wall -Wextra -Wpedantic -I./include
CXXFLAGS   += -g3 -O2 -march=skylake
CXXFLAGS   += -pipe -MP -MMD
LDLIBS     := -lm
LDFLAGS    := 
CWD        := $(shell pwd)
HOST       := $(subst ee-tik-,,$(shell hostname)) # e.g, "cn004"
TARGETS    := test__drama test__dram-fns test__blacksmith dram-attack
SRCS       := $(shell find . -not -ipath "./kmod_pgtrace/*" -name \*.cpp)
OBJS       := $(filter-out ./src/main.o ./src/test__%.o, $(SRCS:.cpp=.o))
MAIN_OBJS  := $(filter ./src/main.o ./src/test__%.o, $(SRCS:.cpp=.o))
GROUP_NAME := $(shell sed -n 's/^GROUP_NAME= *\([^ ]*\).*/\1/p' report.txt)

ifneq ($(strip $(DRAM_INFO)),)
CXXFLAGS += -DDEFAULT_DRAM_INFO=$(DRAM_INFO)
else ifneq ($(strip $(HOST)),)
CXXFLAGS += -DDEFAULT_DRAM_INFO=DRAM__$(HOST)
endif

ifneq ($(V), 1)
VERBOSE=@
endif

all: $(TARGETS)

dram-attack: src/main.o $(OBJS)
test__dram-fns: src/test__dram-fns.o $(OBJS)
test__drama: src/test__drama.o $(OBJS)
test__blacksmith: src/test__blacksmith.o $(OBJS)
$(TARGETS):
		@echo LD $@
		$(VERBOSE) $(CC) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
		@echo CC $@
		$(VERBOSE) $(CC) $(CXXFLAGS) -c $< -o $@

DEPFILES := $(MAIN_OBJS:.o=.d) $(OBJS:.o=.d)
-include $(wildcard $(DEPFILES))

.PHONY: submission
submission: TAR_OUT=$(GROUP_NAME).tar.gz
submission: clean
		@echo TAR $(TAR_OUT)
		-mkdir -m 0700 /tmp/$(USER)
		$(VERBOSE) tar czvf /tmp/$(USER)/$(TAR_OUT) --exclude-vcs --exclude-vcs-ignores -C $(shell dirname $(CWD)) $(shell basename $(CWD))
		$(VERBOSE) mv /tmp/$(USER)/$(TAR_OUT) .
		-rmdir /tmp/$(USER)

.PHONY: clean
clean:
		@echo CLEAN
		$(VERBOSE) $(RM) $(OBJS) $(TARGETS) $(MAIN_OBJS) $(DEPFILES)

