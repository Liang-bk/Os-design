if cd test
then
	echo "get in the test"
	if cd test1
	then
		echo "get in the test1"
		ls
	else
		echo failed
	fi
else
	pwd
	ls
fi