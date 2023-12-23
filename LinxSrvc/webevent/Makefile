PROJECT_BASE=$(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD_PATH:=$(PROJECT_BASE)/../build
MAKEFILE_DIR_PATSUBST:=$(patsubst %/,%,$(patsubst %..,%,$(patsubst %/,%,$(dir $(BUILD_PATH)))))

TARGET:=$(notdir $(MAKEFILE_DIR_PATSUBST))_server
EXECUTALBE:=$(PROJECT_BASE)/../gen/$(TARGET)

$(BUILD_PATH)/Makefile:
	@if [ ! -d $(BUILD_PATH) ]; then mkdir -p $(BUILD_PATH); else rm -fr $(BUILD_PATH)/*; fi;
	@cd $(BUILD_PATH) && cmake $(PROJECT_BASE)/../$(notdir $(MAKEFILE_DIR_PATSUBST)) -DAPPOINT_EVENT=/usr/local && cd -
	@$(MAKE_COMMAND) -C $(BUILD_PATH) && \
	if [ -f $(BUILD_PATH)/$(TARGET) ] && [ -d $(PROJECT_BASE)/../gen ]; then \
		\cp $(BUILD_PATH)/$(TARGET) $(EXECUTALBE); \
		else rm -fr $(BUILD_PATH)/* ; fi;

 make-all:
	@make $(BUILD_PATH)/Makefile

.PHONY: build
build:clean make-all

.PHONY: clean
clean:
	@rm -rvf $(BUILD_PATH)
clean-all:clean
	@rm -rvf $(EXECUTALBE)
