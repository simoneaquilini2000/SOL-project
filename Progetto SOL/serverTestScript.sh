#!/bin/bash

if [ $# -lt 2 ];then
    echo 'Devi mettere 2 parametri'
    exit
fi
numbers=$1
saveDir=$2
serverPid=$(pidof fileStorageServer)
#echo Server pid: $serverPid
#kill -s SIGINT $serverPid
./stampaNumero $numbers &
if mkdir $saveDir;then
    for((i=0;$i <= $numbers;i++))do
        nomeFile=$saveDir/fileDaCreare$i.txt
        if touch $nomeFile;then
            ./stampaNumero $i > $nomeFile &
        else
            echo 'Errore creazione file'
        fi
        #echo Voglio creare $nomeFile
    done
else
    echo 'Errore creazione directory'
fi