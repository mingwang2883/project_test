#!/bin/bash

str="\"Hello World\""
echo $str

str=`echo ${str#*\"}`
str=`echo ${str%%\"*}`

echo $str
