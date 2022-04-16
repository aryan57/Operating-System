mkdir -p "./files_mod"
for file in $(find $1 -type f);do
	cat $file | awk '{$1=$1}1' OFS="," | awk '{print ++i "," $0}' > ./files_mod/"$(basename $file)"
done