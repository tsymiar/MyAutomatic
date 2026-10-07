UNAME_S:=$(shell uname -s)

all:
	@echo "\033[4;3mMaking IM.exe client.exe etc.\033[0m"
	@make -C IM
	@echo "\033[4;3mMaking hdware tools\033[0m"
	@make -C hdware all
	@echo "\033[4;3mMaking thread\033[0m"
	@make -C thread all
	@echo "\033[4;3mMaking webevs serve\033[0m"
	@make -C webevs build
	@echo "\033[4;3mMaking gSOAPverify\033[0m"
	@make -C gSOAPverify || true
	@echo "\033[4;3mMaking CMake targets\033[0m"
	@make cmake
	@echo "\033[4;3mMaking macOS Transfer\033[0m"
	@if [ "$(UNAME_S)" = "Darwin" ]; then \
		cd Mac/Transfer && ./build_app.sh; \
	else echo "\t skipped: requires macOS (Darwin), current = $(UNAME_S)"; fi
	@echo "\033[4;3mMaking python extensions\033[0m"
	@make -C pyex all
	@echo "\033[4;3mMaking vscode extensions\033[0m"
	@make -C vs-extension all
cmake:
	@ if [ ! -d build ]; then mkdir -p build; else echo Deleting built files; rm -rf build/*; fi;
	@ cd build && cmake .. && make -j4 && cd -
clean:
	@make -C gSOAPverify clean
	@make -C webevs clean-all
	@make -C thread clean
	@make -C hdware clean
	@make -C IM clean
	@make -C pyex clean
	@make -C vs-extension clean
	@ if [ -d "$(pwd)../lib" ]; then rm -rvf $(pwd)../lib; fi;
	@ rm -rvf cyber/__pycache__ cyber/.llama-tools cyber/outputs
	@ rm -rvf trade/__pycache__
	@ rm -rvf bin/*.exe* ./*.o */.vs vs
	@ rm -rvf gen out jniLibs
