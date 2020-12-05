#!/bin/bash

#This script is used to look up the results after "bagualu.py" to inspect the results.

directory=$1

if [ -z $1 ]; then
	echo "Usage: $0 <selected_directory>"
	exit -1
fi
if [ ! -d $directory ]; then
	echo "Error: Directory $directory doesn't exist."
	exit -1
fi


echo "==================================================="
echo "=   Package the deploy folders                     "
echo "==================================================="
mkdir togo
cp $directory/deploy togo/ -fr
cp $directory/*.sh togo/
cp $directory/CMakeCache.txt togo/
cp $directory/kernel/build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3/link/vivado/vpl/runme.log togo/
folder_name=`basename $directory`
mv togo $folder_name
tar zcvf ${folder_name}.tar.gz $folder_name
rm -fr $folder_name


echo "==================================================="
echo "=   Package ${folder_name}.tar.gz is generated." 
echo "==================================================="




