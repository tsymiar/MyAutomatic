PROJECT_BASE=$(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD_PATH:=$(PROJECT_BASE)/../build
MAKEFILE_DIR_PATSUBST:=$(patsubst %/,%,$(patsubst %..,%,$(patsubst %/,%,$(dir $(BUILD_PATH)))))

TARGET:=$(notdir $(MAKEFILE_DIR_PATSUBST))_server
EXECUTALBE:=$(PROJECT_BASE)/../gen/$(TARGET)

$(BUILD_PATH)/Makefile:
	@if [ ! -d $(BUILD_PATH) ]; then mkdir -p $(BUILD_PATH); else rm -rf $(BUILD_PATH)/*; fi;
	@cd $(BUILD_PATH) && cmake $(PROJECT_BASE) -DAPPOINT_EVENT=/usr/local && cd -

$(EXECUTALBE):$(BUILD_PATH)/Makefile
	@$(MAKE_COMMAND) -C $(BUILD_PATH) && \
	if [ -f $(BUILD_PATH)/$(TARGET) ] && [ -d $(PROJECT_BASE)/../gen ]; then cp $(BUILD_PATH)/$(TARGET) $(EXECUTALBE); fi;

.PHONY: build
build:$(EXECUTALBE)

.PHONY: clean
clean:
	@rm -rvf $(BUILD_PATH)
clean-all:
	@rm -rvf $(BUILD_PATH)
	@rm -rvf $(EXECUTALBE)
