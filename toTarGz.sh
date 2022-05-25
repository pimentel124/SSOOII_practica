#!/bin/bash
#Script to convert a directory into a tar.gz
echo "~~~~~~~ Convert to tar.gz ~~~~~~~"
echo "Dir to convert to tar.gz: "
read dir
echo "Exit file name: "
read name
cp -r $dir $name
tar -czvf $name.tar.gz $name
rm -rf $name
echo "Dir converted to tar.gz completed succesfully."
