#!/bin/bash

function log_debug () {
    if [[ $VERBOSE -eq 1 ]]; then
        echo "$@"
    fi
}

if  [[ $1 = "-v" ]]; then
	VERBOSE=1
else
	VERBOSE=0
fi

log_debug "a) curl GET request and save in example.html"
curl https://www.example.com > example.html

log_debug "b) get ip and response headers"
curl -i http://ip.jsontest.com/

log_debug "c) set REQ_HEADERS and parse json accordingly"
IFS=',' read -ra ARR <<< "$REQ_HEADERS"
KEYS=$(printf ".\"%s\", " "${ARR[@]}")
curl http://headers.jsontest.com/ | jq "${KEYS::-2}"

log_debug "d) check validity of JSON files"
>valid.txt
>invalid.txt
for file in ./1eJSONData/*;do
	read res < <(curl -d "json=$(cat $file)" -X POST http://validate.jsontest.com/ | jq .validate)
	if [[ "$res" == true ]];then
		echo "$(basename $file)" >> valid.txt
	else
		echo "$(basename $file)" >> invalid.txt
	fi
done
sort valid.txt | < valid.txt
sort invalid.txt | < invalid.txt