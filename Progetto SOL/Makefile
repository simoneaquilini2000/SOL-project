CC = gcc
OTHER_FLAGS = -fsanitize=address -fno-omit-frame-pointer 
CFLAGS = -g -o
CONFIGFILE = /tmp/config.txt
SOCKNAME = /tmp/fileStorageSocket
TARGETS = fileStorageServer fileStorageClient
VALGRIND_OPTIONS = --leak-check=full --track-origins=yes
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
				  valgrind $(VALGRIND_OPTIONS) ./$@ $(CONFIGFILE) 

fileStorageClientTemp: $(CLIENT_DEPS)
				  $(CC) $(CFLAGS) $@ $^
				  ./$@ -f $(SOCKNAME) -p

fileStorageClient: $(CLIENT_DEPS)
				  $(CC) $(CFLAGS) $@ $^
				  ./$@ -f $(SOCKNAME) -p -t 200 -d ./Test_Bash_Safe \
				  -i ./Test_Bash_Safe/FilesDaCreare,0 -w ./Test_Bash_Safe/FilesDaCreare,2 \
				  -C ./Test_Bash_Safe/FilesDaCreare/fileDaCreare0.txt \
				  -o /Test_Bash_Safe/FilesDaCreare/fileDaCreare0.txt -R0 