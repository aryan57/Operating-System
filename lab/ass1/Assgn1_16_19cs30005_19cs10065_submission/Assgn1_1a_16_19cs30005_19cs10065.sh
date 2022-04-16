i=2
n=$1
while [ `expr $i \* $i` -le $n ]
do
	while [ `expr $n % $i` == 0 ]
	do
		echo -n $i ""
		n=`expr $n / $i`
	done
	i=`expr $i + 1`
done
if [ $n != 1 ]; then
	echo $n
fi