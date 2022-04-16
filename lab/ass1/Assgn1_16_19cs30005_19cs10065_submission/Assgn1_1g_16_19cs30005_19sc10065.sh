#!/bin/bash

co=$2
for((i=1; i<=150; i++)) do
	for((j=0; j<10; j++)) do
		number[$j]=$RANDOM
	done
echo "${number[*]}" >> $1
done

for name in $(cut -d' ' -f$2 $1); do
	if [[ $name =~ $3 ]];
	then
		echo "YES";
		exit 1
	fi
done
echo "NO"


