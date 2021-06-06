#!/bin/bash
# Per l'esecuzione dello script si richiede
# un parametro:
#   - nome della cartella da
#     creare, quindi non deve già esistere,
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
confClient3="$commonConf -d ./SaveReadFileDirectory -r clientConfig.c"
confClient4="$commonConf -d ./SaveReadFileDirectory/AllCacheFiles -i ..,5 -w ..,5 -R0"
confClient5="$commonConf -d ./SaveReadFileDirectory/AllCacheFiles -i ..,10 -C pippo.txt -R1"

#setup array di configurazioni client
clientConf[0]=$confClient1
clientConf[1]=$confClient2
clientConf[2]=$confClient3
clientConf[3]=$confClient4
clientConf[4]=$confClient4
saveDir=$1
#echo $testType    
valgrind --leak-check=full --show-leak-kinds=all ./fileStorageServer ./ServerConf/config.txt &
serverPid=$!


rm -f $saveDir/*
rmdir $saveDir

if mkdir $saveDir;then
    for((i=0;$i < ${#clientConf[@]};i++));do
        nomeFile=$saveDir/outputClient$i.txt
        if touch $nomeFile;then
            ./fileStorageClient ${clientConf[$i]} > $nomeFile &
        else
            echo 'Errore creazione file'
        fi
        sleep 0.5
    done
    kill -s SIGHUP $serverPid
    wait $serverPid
else
    echo 'Errore creazione directory'
fi