#!/bin/bash

ls -1 | grep ak_ipcam_tutk > list.txt

while read line
do
    echo $line
    file=`echo $line|sed 's/ak_ipcam_tutk/tutk_ipc/g'`
    mv $line $file
done < list.txt

