CC = gcc
CFLAGS = -g -o
CONFIGFILE1 = ./ServerConf/config.txt
CONFIGFILE2 = ./ServerConf/config2.txt
CLIENTOUTPUT_DIR1 = ./outputClientsTest1
CLIENTOUTPUT_DIR2 = ./outputClientsTest2
FLAG_INCLUDE_DIR = -I./Headers 
SOCKNAME = ./serverConf/fileStorageSocket
TARGETS = fileStorageServer fileStorageClient
VALGRIND_OPTIONS = valgrind --leak-check=full --track-origins=yes
QUEUE_DATA_TYPES_DEP = ./Queue_Data_Types/
CONFIG_UTILITY_DEP = ./ConfigAndUtilities/

COMMON_DEPS = $(QUEUE_DATA_TYPES_DEP)generic_queue.c $(CONFIG_UTILITY_DEP)utility.c \
 	$(QUEUE_DATA_TYPES_DEP)request.c $(QUEUE_DATA_TYPES_DEP)file.c

SERVER_DEPS = fileStorageServer.c $(QUEUE_DATA_TYPES_DEP)descriptor.c \
 $(CONFIG_UTILITY_DEP)serverInfo.c $(COMMON_DEPS)

CLIENT_DEPS = fileStorageClient.c $(CONFIG_UTILITY_DEP)clientConfig.c \
 serverAPI.c $(COMMON_DEPS)

CLIENTCONFIGOPTIONS = -f $(SOCKNAME) -p -t 200 

OBJ_FOLDER = build/objs


#configurazioni di un client per testare singolarmente
# le funzioni dell'API
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

.PHONY : all, cleanall, test1, test2

all:
	make -B fileStorageServer
	make -B fileStorageClient

cleanall:
	rm $(SOCKNAME) $(TARGETS)
	rm $(CLIENTOUTPUT_DIR1)/*
	rmdir $(CLIENTOUTPUT_DIR1)
	rm $(CLIENTOUTPUT_DIR2)/*
	rmdir $(CLIENTOUTPUT_DIR2)

fileStorageServer: $(SERVER_DEPS)
				  $(CC) $(CFLAGS) $@ $^ $(FLAG_INCLUDE_DIR) -lpthread
				 #$(VALGRIND_OPTIONS) ./$@ $(CONFIGFILE1)

fileStorageClient: $(CLIENT_DEPS)
				  $(CC) $(CFLAGS) $@ $^ $(FLAG_INCLUDE_DIR)

#creo libreria s_api.so(da finire)
libs/libAPI.so: $(OBJ_FOLDER)/s_API.o
	$(CC) -shared -o $@ $^

$(OBJ_FOLDER)/s_API.o:
	$(CC) $(CFLAGS) $@ serverAPI.c $(FLAG_INCLUDE_DIR) -fPIC -c

test1:
	./fileStorageServer $(CONFIGFILE1) &
	./serverTestScript.sh $(CLIENTOUTPUT_DIR1)

test2:
	./fileStorageServer $(CONFIGFILE2) &
	./serverTestScript.sh $(CLIENTOUTPUT_DIR2)
