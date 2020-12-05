#!/bin/bash

#This script is used to look up the results after "bagualu.py" to inspect the results.
if [ -z $1 ]; then
	echo "Usage: $0 <directory_pool>"
	exit -1
fi

directory_pool=$1

#Slurm Queue to see if there are some jobs still running
echo "==================================================="
echo "=     Check if some jobs are still running...     ="
echo "==================================================="

squeue

#Get the timing results from each directory
echo "==================================================="
echo "=    Check .PASSED and Extract the timing results..." 
echo "==================================================="

for dir in $(ls -d $directory_pool/k*)
do
    if [ -f $dir/.PASSED ]; then
        echo "[PASSED] $dir"
        grep "Post Physical Optimization Timing Summary" $dir/kernel/build_dir.hw.xilinx_u50_gen3x16_xdma_201920_3/link/vivado/vpl/runme.log
        echo ""
    fi
done


