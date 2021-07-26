PROJECT_BASE=$(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD_PATH:=$(PROJECT_BASE)/build
MAKEFILE_DIR_PATSUBST:=$(patsubst %/,%,$(dir $(BUILD_PATH)))

TARGET:=$(notdir $(MAKEFILE_DIR_PATSUBST))
EXECUTALBE:=$(PROJECT_BASE)/../bin/$(TARGET)

$(BUILD_PATH)/Makefile:
	mkdir -p $(BUILD_PATH) && cd $(BUILD_PATH) \
	&& cmake $(PROJECT_BASE) -DWITH_EVENT=/usr/local/Cellar/libevent/2.1.12

$(EXECUTALBE):$(BUILD_PATH)/Makefile
	$(MAKE_COMMAND) -C $(BUILD_PATH)
	cp $(BUILD_PATH)/$(TARGET) $(EXECUTALBE)

.PHONY: build
build:$(EXECUTALBE)

.PHONY: clean
clean:
	@rm -rvf $(PROJECT_BASE)/build
cleanAll:
	@rm -rvf $(PROJECT_BASE)/build
	@rm -rvf $(EXECUTALBE)
