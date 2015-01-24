#!/bin/bash

file=$1
tmpfile="/tmp/${file##*/}"
header="${file##/*}"
header="${header%.*}.h"

declare -a includes
declare -a sorted

while read line
do
    echo $line | grep "^#include" | grep -v "$header" > /dev/null
    if [[ $? == 0 ]]
    then
        includes+=("$line")
    fi
done < $file

IFS=$'\n'
sorted=( $(for e in "${includes[@]}"
           do
               echo "$e" | grep "/" > /dev/null
               if [[ $? != 0 ]]
               then
                   echo -ne "1"
               fi
               echo "$e"
           done | sort) )

IFS=$'\n'

sorted=( $(first=0
           for e in "${sorted[@]}"
           do
               echo $e | sed 's:^1::'
           done) )               

i=0

>$tmpfile

IFS=$'\n'

first=0
while read -r line
do
    echo "$line" | grep "^#include" | grep -v "$header" > /dev/null
    if [[ $? == 0 ]]
    then
        if [[ $first == 0 ]]
        then
            echo "${sorted[$i]}" | grep -v "/" > /dev/null
            if [[ $? != 0 ]]
            then
                first=1
                echo >> $tmpfile
            fi
        fi

        echo "${sorted[$i]}" >> $tmpfile
        i=$(($i + 1))
    else
        echo "$line" >> $tmpfile
    fi
done < $file

# cat $tmpfile
mv $tmpfile $file
