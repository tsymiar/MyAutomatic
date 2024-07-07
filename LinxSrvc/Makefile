all:
	@echo Making IM.exe client.exe etc.
	@make -C IM
	@echo Making hdware thread
	@make -C hdware all
	@make -C thread all
	@echo Making webev
	@make -C webev build
	@echo Making gSOAPverify
	@make -C gSOAPverify || true
	@echo Making CMakeLists
	@make cmake
cmake:
	@ if [ ! -d build ]; then mkdir -p build; else rm -rf build/*; fi;
	@ cd build && cmake .. && make -j4 && cd -
clean:
	make -C webev clean-all
	make -C gSOAPverify clean
	make -C thread clean
	make -C hdware clean
	make -C IM clean
	@rm -rvf bin/*.exe* ./*.o */.vs vs
	@rm -rvf jniLibs gen out
	@rm -rvf gn/out*
