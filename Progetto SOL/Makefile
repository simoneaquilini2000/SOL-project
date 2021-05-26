CC = gcc
OTHER_FLAGS = -fsanitize=address -fno-omit-frame-pointer 
CFLAGS = -g -o
SOCKNAME = /tmp/config.txt
TARGETS = fileStorageServer fileStorageClient
COMMON_DEPS = generic_queue.c utility.c request.c file.c
SERVER_DEPS = fileStorageServer.c descriptor.c serverInfo.c $(COMMON_DEPS)
CLIENT_DEPS = fileStorageClient.c clientConfig.c serverAPI.c $(COMMON_DEPS) 
.PHONY : all, cleanall

all:
	make fileStorageServer
	make fileStorageClient

cleanall:
	rm $(SOCKNAME) $(TARGETS) 

fileStorageServer: $(SERVER_DEPS)
				  $(CC) $(CFLAGS) $@ $^ -lpthread

fileStorageClient: $(CLIENT_DEPS)
				  $(CC) $(CFLAGS) $@ $^