DEST_PATH=../gen
TARGET=$(shell pwd | awk -F'/' '{print $NF}')
all:
	@ ./make.sh
	@ if [ -f $(ls -1 *.vsix) ]; then mv *.vsix $(DEST_PATH) || true; fi;
	@ echo "Built target ${TARGET}! Check out $(abspath $(DEST_PATH))"

clean:
	@ ./make.sh clean

.PHONY: all clean
