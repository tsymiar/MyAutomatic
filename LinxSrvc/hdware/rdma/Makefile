CFLAGS = -g -Wall -lrdmacm -libverbs
ABS_GEN = $(abspath $(GEN))
all: rdma_server.exe rdma_client.exe
	@if command -v apt >/dev/null 2>&1; then \
		echo "fetch RDMA dependencies..."; \
		sudo apt install -y rdma-core librdmacm-dev ibverbs-utils; \
	fi;
%.o: %.c
	$(CC) $(CFLAGS) -o $@ $< $(CFLAGS)
rdma_server.exe:
	$(CC) server.c $(CFLAGS) -o $(ABS_GEN)/$@
rdma_client.exe:
	$(CC) client.c $(CFLAGS) -o $(ABS_GEN)/$@
clean:
	@rm -rvf $(ABS_GEN)/rdma_*.exe *.o
