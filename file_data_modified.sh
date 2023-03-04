#!/bin/bash

while read line
do
    echo $line
    sed -i "s/if \[ \$cnt \-eq 8 \]\;then/if \[ \$cnt \-eq 5 \]\;then/g" $line
    sed -i '/hello/d' $line
done < list.txt
