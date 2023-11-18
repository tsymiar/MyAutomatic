all:
	make -C IM
	make -C hdware all
	make -C thread all
	make -C webevent build
	@echo Making gSOAPverify
	@make -C gSOAPverify || true
	make cmake
cmake:
	@ if [ ! -d build ]; then mkdir -p build; else rm -rf build/*; fi;
	@ cd build && cmake .. && make -j4 && cd -
clean:
	make -C webevent clean-all
	make -C gSOAPverify clean
	make -C thread clean
	make -C hdware clean
	make -C IM clean
	@rm -rvf bin/*.exe* ./gen ./out ./*.o */.vs vs
	@rm -rvf jniLibs
