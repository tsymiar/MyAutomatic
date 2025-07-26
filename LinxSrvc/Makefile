all:
	@echo "\033[4;3mMaking IM.exe client.exe etc.\033[0m"
	@make -C IM
	@echo "\033[4;3mMaking hdware thread etc.\033[0m"
	@make -C hdware all
	@make -C thread all
	@echo "\033[4;3mMaking webserver\033[0m"
	@make -C webev build
	@echo "\033[4;3mMaking gSOAPverify\033[0m"
	@make -C gSOAPverify || true
	@echo "\033[4;3mMaking CMakeLists\033[0m"
	@make cmake
cmake:
	@ if [ ! -d build ]; then mkdir -p build; else echo Deleting built files; rm -rf build/*; fi;
	@ cd build && cmake .. && make -j4 && cd -
clean:
	@make -C gSOAPverify clean
	@make -C webev clean-all
	@make -C thread clean
	@make -C hdware clean
	@make -C IM clean
	@ if [ -d "$(pwd)../lib" ]; then rm -rvf $(pwd)../lib; fi;
	@ rm -rvf bin/*.exe* ./*.o */.vs vs
	@ rm -rvf gen out jniLibs
