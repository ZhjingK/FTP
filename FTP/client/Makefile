src=main.c ftpClient.c ../common/common.c
CC=gcc

ftpClient:$(src)
	$(CC) -o $@ $^

.PHONY:clean
clean:
	rm -rf ftpClient
