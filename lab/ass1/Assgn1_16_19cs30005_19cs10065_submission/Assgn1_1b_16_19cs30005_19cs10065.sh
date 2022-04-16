mkdir -p 1.b.files.out;
for file in ./1.b.files/*
do
	sort -n $file > ./1.b.files.out/"$(basename $file)"
done
cat ./1.b.files/* | sort -n | uniq -c | awk '{print $2,$1}' > 1.b.out.txt;