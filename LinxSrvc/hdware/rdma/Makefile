CFLAGS = -g -Wall -lrdmacm -libverbs
ABS_GEN = $(abspath $(GEN))
RDMA=$(shell echo $(shell pwd) | awk -F '/' '{print $$NF}')
FILES := $(shell find . -maxdepth 1 -type f -name "*.c")
TARGETS := $(patsubst %.c,%,$(notdir $(FILES)))

define FILE_TARGET
$(RDMA)_$(1):
	@$(CC) $(2) $(CFLAGS) -o $(ABS_GEN)/$(RDMA)_$(1).exe
	@echo "-- Built: \033[1m$(RDMA)_$(1)\033[0m"
endef

$(foreach file,$(FILES), \
	$(eval $(call FILE_TARGET,$(basename $(notdir $(file))),$(file))) \
)

all: $(addprefix $(RDMA)_,$(TARGETS))
	@echo "All targets processed: $(addprefix $(RDMA)_,$(TARGETS))"

.PHONY: all $(addprefix $(RDMA)_,$(TARGETS))

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $< $(CFLAGS)

env:
	@if command -v apt >/dev/null 2>&1; then \
		echo "Fetching RDMA dependencies..."; \
		{ \
			if [ "$$(id -u)" -ne 0 ]; then \
				echo "\033[33mWarning\033[0m: Must be superuser to continue."; \
			fi; \
			$(shell pwd)/set_sw-rxe.sh; \
		} \
	fi;

clean:
	@rm -rvf $(ABS_GEN)/$(RDMA)_*.exe *.o
