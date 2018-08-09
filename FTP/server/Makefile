src=main.c ftpServer.c ../common/common.c
CC=gcc
LIB=-lpthread

ftpServer:$(src)
	$(CC) -o $@ $^ $(LIB)

.PHONY:clean
clean:
	rm -rf ftpServer
