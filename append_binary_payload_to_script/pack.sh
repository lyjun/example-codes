#!/bin/sh
read -p "Input a tar file path: " file

# is file exsted >
if [ ! -f ${file} ] 
then
	echo "no such file"
	exit 1
fi

# check file extension
extension=$(basename ${file} | tr "[:upper:]" "[:lower:]" | tr '.' '\n' | tail -n 1)
if [ ${extension} != "tar" ]
then
	echo "only support tar file"
	exit 1
fi

# append install.sh first
echo "packing...."
cat install_script.sh > install.sh
echo "PAYLOAD:" >> install.sh

# append tar file
cat ${file}  >> install.sh
echo "done...."
