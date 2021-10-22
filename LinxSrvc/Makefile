all:
	make -C IM all
	make -C hdware all
	make -C HttpAgent build
	make -C gSOAPverify || true
clean:
	make -C HttpAgent cleanAll
	make -C gSOAPverify clean
	rm -rvf bin/*.exe* ./obj
	rm -rvf jniLibs
