co="$2"
awk -v "col=${co}" '{print tolower($col)}' $1 | sort | uniq -c | awk '{print $2" "$1}' | sort -r -k 2 > "1c_output _${co}_column.freq"
