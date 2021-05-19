CC = gcc
CFLAGS = -g -o
SOCKNAME = /tmp/config.txt
COMMON_DEPS = generic_queue.c utility.c request.c file.c
SERVER_DEPS = fileStorageServer.c descriptor.c serverInfo.c $(COMMON_DEPS)
CLIENT_DEPS = fileStorageClient.c clientConfig.c serverAPI.c $(COMMON_DEPS) 

fileStorageServer: $(SERVER_DEPS)
				  $(CC) $(CFLAGS) $@ $^ -lpthread

fileStorageClient: $(CLIENT_DEPS)
				  $(CC) $(CFLAGS) $@ $^