all:
	make -C IM all
	make -C hdware all
	make -C thread all
	make -C httpsample build
	make -C gSOAPverify || true
clean:
	make -C httpsample cleanAll
	make -C gSOAPverify clean
	make -C thread clean
	make -C hdware clean
	make -C IM clean
	@rm -rvf bin/*.exe* ./obj vs
	@rm -rvf jniLibs
