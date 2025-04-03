all:
	@echo Making IM.exe client.exe etc.
	@make -C IM
	@echo Making hdware thread etc.
	@make -C hdware all
	@make -C thread all
	@echo Making webserver
	@make -C webev build
	@echo Making gSOAPverify
	@make -C gSOAPverify || true
	@echo Making CMakeLists
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
