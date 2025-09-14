.PHONY: all submission


CWD        := $(shell pwd)
GROUP_NAME := $(shell sed -n 's/^GROUP_NAME= *\([^ ]*\).*/\1/p' report.txt)


all: submission


.PHONY: submission
submission: TAR_OUT=$(GROUP_NAME).tar.gz
submission: clean
	$(MAKE) -C retbleed submission
	@echo TAR $(TAR_OUT)
	-mkdir -m 0700 /tmp/$(USER)
	$(VERBOSE) tar czvf /tmp/$(USER)/$(TAR_OUT) --exclude-vcs --exclude-vcs-ignores -C $(shell dirname $(CWD)) $(shell basename $(CWD))
	$(VERBOSE) mv /tmp/$(USER)/$(TAR_OUT) .
	-rmdir /tmp/$(USER)


clean:
	$(MAKE) -C retbleed clean
	$(MAKE) -C meltdown_spectre clean
	rm -f $(TAR_OUT)
