a=1
b=8
if echo 1
then
	while [ $a -lt $b ]
	do 
		if [ $a -ge 3 ]
		then
			echo a=$a
		fi
		a=[ $a + 1 ]
	done
fi