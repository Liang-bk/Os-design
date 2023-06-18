if echo 1
then
	echo abc
	if cd x
	then
		echo in second if
	else
		if echo 3
		then
			echo in third if
		fi
	fi
else
	if echo 4
	then
	echo in fourth if
	fi
fi