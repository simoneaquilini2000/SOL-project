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
CLIENTCONFIGOPTIONS = -f $(SOCKNAME) -p -t 200 

CONFTEST1OPENFILE = -i .,1 -i .,1 -o fileDaLeggere.txt,clientConfig.c
CONFTEST1CLOSEFILE = -i .,2 -C fileDaLeggere.txt,clientConfig.c,clientConfig.c 
CONFTEST1READFILE = -d ./TestFileBinari -i .,2 -C clientConfig.c \
 -r fileDaLeggere.txt,clientConfig.c,clientConfig.h
CONFTEST1READNFILESWRONG = -d ./Pippo -i .,5 -R0
CONFTEST1READNFILESRIGHT = -d ./TestFileBinari -i .,5 -R0
CONFTEST1REMOVEFILE = -i .,5 -c fileDaLeggere.txt,clientConfig.c
CONFTEST1WRITEFILE = -i .,5 \
	-W clientConfig.h,pippo.txt,serverTestScript.sh \
	-C descriptor.h -W descriptor.h -o descriptor.h \
	-W descriptor.h

.PHONY : all, cleanall

all:
	make -B fileStorageServer
	make -B fileStorageClient

cleanall:
	rm $(SOCKNAME) $(TARGETS) 

fileStorageServer: $(SERVER_DEPS)
				  $(CC) $(CFLAGS) $@ $^ -lpthread
				  valgrind $(VALGRIND_OPTIONS) ./$@ $(CONFIGFILE) 

fileStorageClient: $(CLIENT_DEPS)
				  $(CC) $(CFLAGS) $@ $^
				  ./$@ $(CLIENTCONFIGOPTIONS) $(CONFTEST1WRITEFILE)