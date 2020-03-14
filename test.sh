#!/bin/bash

cur_cnt=0
max_chn=0
ret=`cat ./wifi_chn_loading |wc -l`
if [ $ret -gt 0 ]; then
    while read line
    do
        let cur_cnt++
        #echo $cur_cnt
        #echo $line
        #frist get loading
        if [ $cur_cnt -eq 1 ];then
            max_loading=$line
            max_chn=$cur_cnt
            continue
        fi

        if [ $line -ge $max_loading ];then
            max_loading=$line
            max_chn=$cur_cnt
            #echo $max_loading
            #echo $max_chn
        fi
    done < ./wifi_chn_loading
fi

