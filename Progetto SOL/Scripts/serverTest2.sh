#!/bin/bash
# Per l'esecuzione dello script si richiede
# un parametro:
#   - nome della cartella da
#     creare(se già esiste verrà rimossa e ricreata con nuovo contenuto
#     altrimenti verrà semplicemente creata)
#     in cui salvare i file che riportano
#     l'output del singolo client.
#
if [ $# -lt 1 ];then
    echo 'Devi mettere almeno un parametro(la cartella in cui salvare gli output dei client)'
    exit
fi
commonConf="-f ./ServerConf/fileStorageSocket -t 200 -p"
#configurazioni di clients
confClient1="$commonConf -i .,20 -w .,13 -C clientConfig.c"
confClient2="$commonConf -i ../Processi,0 -W ../Processi/processi.pdf -o clientConfig.c"
confClient3="$commonConf -d ./SaveReadFileDirectory -r clientConfig.c -i ../../../../Primo_Anno,4 \
    -w ../../../../Primo_Anno,2"
confClient4="$commonConf -d ./SaveReadFileDirectory/AllCacheFilesTest2 -i ..,5 -w ..,5 -R0"
confClient5="$commonConf -t 200 -d ./SaveReadFileDirectory -i ..,10 \
                    -C ./ConfigAndUtilities/serverInfo.c -R1"

#setup array di configurazioni client
clientConf[0]=$confClient1
clientConf[1]=$confClient2
clientConf[2]=$confClient3
clientConf[3]=$confClient4
clientConf[4]=$confClient5
saveDir=$1 #ottengo dal primo parametro la cartella dove salvare gli output de clients

   
./fileStorageServer ./ServerConf/config2.txt &
serverPid=$(pidof fileStorageServer) #ottengo il PID del server in modo da poter segnalarlo

#rimuovo saveDir(se non esiste fallirà)
rm -f $saveDir/* 
rmdir $saveDir

if mkdir $saveDir;then
    for((i=0;$i < ${#clientConf[@]};i++));do
        nomeFile=$saveDir/outputClient$i.txt
        if touch $nomeFile;then
            ./fileStorageClient ${clientConf[$i]} > $nomeFile & #redirigo output client sul file
        else
            echo 'Errore creazione file'
        fi
        sleep 1 #intervallo di creazione dei clients
    done
    kill -s SIGHUP $serverPid #invio SIGHUP al server
    wait $serverPid
else
    echo 'Errore creazione directory'
fi