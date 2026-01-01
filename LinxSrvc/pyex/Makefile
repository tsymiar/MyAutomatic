DEST_PATH=../gen
all:
	@ ./make.sh
	@ if [ -f $(ls -1 *.whl) ]; then mv dist/*.whl $(DEST_PATH) || true; fi;
	@ echo "Built target ${PWD##*/}! Check out $(abspath $(DEST_PATH))"

clean:
	@ ./make.sh clean

.PHONY: all clean
