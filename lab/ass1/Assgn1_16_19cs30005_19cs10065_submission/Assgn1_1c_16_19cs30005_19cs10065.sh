#!/bin/bash
for filepath in $(find ./data1c/ -type f);do
	filename="${filepath##*/}"
	ext="${filename##*.}"
	if [[ "$ext" == "$filename" ]];then
		ext="Nil"
	fi
	mkdir -p $ext
	mv $filepath ./$ext
done