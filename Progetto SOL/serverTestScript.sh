#!/bin/bash

# Per l'esecuzione dello script si richiede
# un parametro:
#   - nome della cartella da
#     creare, quindi non deve già esistere,
#     in cui salvare i file che riportano
#     l'output del singolo client.
#
if [ $# -lt 1 ];then
    echo 'Devi mettere 2 parametri'
    exit
fi
saveDir=$1
serverPid=$(pidof fileStorageServer)
if mkdir $saveDir;then
    for((i=0;$i <= $numbers;i++))do
        nomeFile=$saveDir/outputClient$i.txt
        if touch $nomeFile;then
            ./stampaNumero $i > $nomeFile &
        else
            echo 'Errore creazione file'
        fi
        #echo Voglio creare $nomeFile
    done
    kill -s SIGHUP $serverPid
else
    echo 'Errore creazione directory'
fi