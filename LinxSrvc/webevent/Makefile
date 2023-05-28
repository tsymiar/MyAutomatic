PROJECT_BASE=$(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD_PATH:=$(PROJECT_BASE)/../build
MAKEFILE_DIR_PATSUBST:=$(patsubst %/,%,$(patsubst %..,%,$(patsubst %/,%,$(dir $(BUILD_PATH)))))

TARGET:=$(notdir $(MAKEFILE_DIR_PATSUBST))
EXECUTALBE:=$(PROJECT_BASE)/../bin/$(TARGET)

$(BUILD_PATH)/Makefile:
	@ if [ ! -d $(BUILD_PATH) ]; then mkdir -p $(BUILD_PATH); fi;
	@ cd $(BUILD_PATH) && cmake $(PROJECT_BASE) -DAPPOINT_EVENT=/usr/local && cd -

$(EXECUTALBE):$(BUILD_PATH)/Makefile
	@$(MAKE_COMMAND) -C $(BUILD_PATH) && \
	if [ -f $(BUILD_PATH)/$(TARGET) ] && [ -d $(PROJECT_BASE)/../bin ]; then cp $(BUILD_PATH)/$(TARGET) $(EXECUTALBE); fi;

.PHONY: build
build:$(EXECUTALBE)

.PHONY: clean
clean:
	@rm -rvf $(BUILD_PATH)
cleanAll:
	@rm -rvf $(BUILD_PATH)
	@rm -rvf $(EXECUTALBE)
