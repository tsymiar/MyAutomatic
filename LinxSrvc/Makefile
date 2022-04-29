all:
	make -C IM all
	make -C hdware all
	make -C pthdpool all
	make -C httpserver build
	make -C gSOAPverify || true
clean:
	make -C httpserver cleanAll
	make -C gSOAPverify clean
	make -C pthdpool clean
	make -C IM clean
	rm -rvf bin/*.exe* ./obj
	rm -rvf jniLibs
