#!/bin/bash

ls -1 > list.txt

while read line
do
    echo $line
    sed -i "s/$old/$new/g" $line
done < list.txt
