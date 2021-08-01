all:
	make -C IM all
	make -C hdware all
	make -C AuthAgent build
	make -C gSOAPverify || true
clean:
	make -C AuthAgent cleanAll
	make -C gSOAPverify clean
	rm -rvf bin/*.exe* obj
