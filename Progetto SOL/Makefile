CC = gcc
CFLAGS = -g -o
CONFIGFILE1 = ./ServerConf/config.txt
CONFIGFILE2 = ./ServerConf/config2.txt
CLIENTOUTPUT_DIR1 = ./outputClientsTest1
CLIENTOUTPUT_DIR2 = ./outputClientsTest2
FLAG_INCLUDE_DIR = -I./Headers 
SOCKNAME = ./serverConf/fileStorageSocket
TARGETS = fileStorageServer fileStorageClient
VALGRIND_OPTIONS = valgrind --leak-check=full
QUEUE_DATA_TYPES_DEP = ./Queue_Data_Types/
CONFIG_UTILITY_DEP = ./ConfigAndUtilities/

COMMON_DEPS = $(QUEUE_DATA_TYPES_DEP)generic_queue.c $(CONFIG_UTILITY_DEP)utility.c \
 	$(QUEUE_DATA_TYPES_DEP)request.c $(QUEUE_DATA_TYPES_DEP)file.c

SERVER_DEPS = fileStorageServer.c libs/libcommon.so libs/libserv.so

#fileStorageServer.c $(QUEUE_DATA_TYPES_DEP)descriptor.c \
 $(CONFIG_UTILITY_DEP)serverInfo.c $(COMMON_DEPS)

CLIENT_DEPS = fileStorageClient.c libs/libclient.so libs/libapi.so libs/libcommon.so

#fileStorageClient.c $(CONFIG_UTILITY_DEP)clientConfig.c \
 serverAPI.c $(COMMON_DEPS)

CLIENTCONFIGOPTIONS = -f $(SOCKNAME) -p -t 200 

OBJ_FOLDER = ./build/objs
LIB_FOLDER = ./libs


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
	rm -f $(OBJ_FOLDER)/*.o
	rm -f $(LIB_FOLDER)/*.so
	rm -f ./SaveReadFileDirectory/AllCacheFilesTest2/*
	rm -f ./SaveReadFileDirectory/AllCacheFilesTest1/* 


#rm $(CLIENTOUTPUT_DIR1)/*
#rmdir $(CLIENTOUTPUT_DIR1)
#rm $(CLIENTOUTPUT_DIR2)/*
#rmdir $(CLIENTOUTPUT_DIR2)

#targets per generare gli eseguibili
fileStorageServer: $(SERVER_DEPS)
				  $(CC) $(CFLAGS) $@ fileStorageServer.c $(FLAG_INCLUDE_DIR) \
				   -Wl,-rpath,./libs -L ./libs -lcommon -lserv -lpthread

fileStorageClient: $(CLIENT_DEPS)
				  $(CC) $(CFLAGS) $@ fileStorageClient.c $(FLAG_INCLUDE_DIR) \
				  -Wl,-rpath,./libs -L ./libs -lcommon -lclient -lapi

#costruisco librerie
$(LIB_FOLDER)/libcommon.so: $(OBJ_FOLDER)/generic_queue.o $(OBJ_FOLDER)/utility.o $(OBJ_FOLDER)/request.o $(OBJ_FOLDER)/file.o
	$(CC) -shared $(CFLAGS) $@ $^

$(LIB_FOLDER)/libserv.so: $(OBJ_FOLDER)/descriptor.o $(OBJ_FOLDER)/serverInfo.o
	$(CC) -shared $(CFLAGS) $@ $^

$(LIB_FOLDER)/libclient.so: $(OBJ_FOLDER)/clientConfig.o
	$(CC) -shared $(CFLAGS) $@ $^

$(LIB_FOLDER)/libapi.so: $(OBJ_FOLDER)/s_API.o
	$(CC) -shared -o $@ $^

#costruisco file oggetto per le varie librerie

# per libapi
$(OBJ_FOLDER)/s_API.o:
	$(CC) $(CFLAGS) $@ serverAPI.c $(FLAG_INCLUDE_DIR) -fPIC -c

# per libcommon 
$(OBJ_FOLDER)/generic_queue.o:
	$(CC) $(CFLAGS) $@ $(QUEUE_DATA_TYPES_DEP)/generic_queue.c $(FLAG_INCLUDE_DIR) -fPIC -c

$(OBJ_FOLDER)/request.o:
	$(CC) $(CFLAGS) $@ $(QUEUE_DATA_TYPES_DEP)/request.c $(FLAG_INCLUDE_DIR) -fPIC -c

$(OBJ_FOLDER)/file.o:
	$(CC) $(CFLAGS) $@ $(QUEUE_DATA_TYPES_DEP)/file.c $(FLAG_INCLUDE_DIR) -fPIC -c

$(OBJ_FOLDER)/utility.o:
	$(CC) $(CFLAGS) $@ $(CONFIG_UTILITY_DEP)/utility.c $(FLAG_INCLUDE_DIR) -fPIC -c

# per libserv
$(OBJ_FOLDER)/serverInfo.o:
	$(CC) $(CFLAGS) $@ $(CONFIG_UTILITY_DEP)/serverInfo.c $(FLAG_INCLUDE_DIR) -fPIC -c

$(OBJ_FOLDER)/descriptor.o:
	$(CC) $(CFLAGS) $@ $(QUEUE_DATA_TYPES_DEP)/descriptor.c $(FLAG_INCLUDE_DIR) -fPIC -c

# per libclient
$(OBJ_FOLDER)/clientConfig.o:
	$(CC) $(CFLAGS) $@ $(CONFIG_UTILITY_DEP)/clientConfig.c $(FLAG_INCLUDE_DIR) -fPIC -c


test1:
	./Scripts/serverTest1.sh $(CLIENTOUTPUT_DIR1)

test2:
	./Scripts/serverTest2.sh $(CLIENTOUTPUT_DIR2)
