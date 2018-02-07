#!/bin/sh

unpack()
{
	file=$1
	match=$(($(grep --text --line-number -m1 '^PAYLOAD:$' ${file} | cut -d ':' -f 1) + 1))
	echo "extract payload after line ${match}"
	echo "create destination folder \"unpack\""
	mkdir unpack
	echo "extracting...."
	tail -n +${match} $0 | tar -xvf - -C unpack
}


read -p "install ? (y/n) " ans
result=`echo ${ans} | tr "[:upper:]" "[:lower:]"`

# exit installation
if [ ${result} != "y" ]
then
	echo "exit installation"
	exit 0
fi

# start to install
unpack $0
exit 0
