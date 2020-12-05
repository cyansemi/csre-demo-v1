__='
Copyright 2020 CyanSemi Semiconductor Co.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

'
#!/bin/bash

# The script requires the directory of "deploy" folder
# and tclist file

if [ -z $1 ]; then
    echo "ERROR! usage: $0 <deploy_folder> <tclist>"
    exit -1
fi

if [ -z $2 ]; then
    echo "ERROR: usage: $0 <deploy_folder> <tclist>"
    echo "Testcase list file doesn't exist"
    exit -1
fi


deploy_folder=`realpath $1` #Convert the input relative path to absolute one
tclist=$2

if [ ! -d $deploy_folder ]; then
    echo "ERROR! deploy_folder $deploy_folder doesn't exist"
    exit -1
fi

exec=$deploy_folder/csre-st-ocl
xclbin=`find $deploy_folder -name *.xclbin`
xclbin_num=`find $deploy_folder -name *.xclbin | wc -l`

xcl_target=hw


if [ ! -f $exec ]; then
    echo "ERROR: cannot find $exec"
    exit -1
fi
if [[ $xclbin_num > 1 ]]; then
    echo "ERROR: $xclbin_num xclbin found in $deploy_folder. Please keep only one in the deploy folder."
    exit -1
fi

if [ ! -f $xclbin ]; then
    echo "ERROR: cannot find $xclbin"
    exit -1
fi


#Unzip the test data for the first time
for packet_file in testdata/*/*.zip; do
    packet_file=${packet_file%.zip}.txt

    if [ ! -f $packet_file ]; then
        if [ -f ${packet_file%.txt}.zip ]; then
            echo "For the first time, unzip file $packet_file ... "
            unzip -qq ${packet_file%.txt}.zip -d `dirname $packet_file`
        else
            echo "ERROR: $packet_file not exist!"
            return -1
        fi
    fi
done

if [ ! -d output ]; then
    mkdir output
fi

failed_num=0
failed_list=""

#Test body
run_test () {
    packet_file=testdata/$1
    regex_file=testdata/$2
    if [ ! -f $packet_file ]; then
        echo "ERROR: $packet_file doesn't exist!"
        exit -1
    fi
    if [ ! -f $regex_file ]; then
        echo "ERROR: $regex_file doesn't exist!"
        echo "Have you run ./pre_check.sh <folder_dir> yet?"
        exit -1
    fi

    msg=$3
    if [ -z $msg ]; then
        msg="No_Name"
    fi

    nocheck=$4

    test_name=${msg}-`basename ${packet_file%.txt}`-`basename ${regex_file%.txt}`

    echo "------------------------------------------------------------"
    echo "Test:    $msg [$nocheck]"
    echo "Packet:  $packet_file"
    echo "Regex:   $regex_file"
    echo ""

    test_log=output/${test_name}.log

    if [[ $nocheck == "nocheck" ]]; then
        cmd="$exec -k $xclbin -p $packet_file -q $regex_file -d"
    else
        cmd="$exec -k $xclbin -p $packet_file -q $regex_file"
    fi
        

    echo "Run -> $cmd"

    #Run!!!
    if [ $xcl_target == "hw_emu" ]; then
        LD_LIBRARY_PATH=$deploy_folder XCL_EMULATION_MODE=$xcl_target $cmd > ${test_log} 2> ${test_log}
    else 
        LD_LIBRARY_PATH=$deploy_folder $cmd > ${test_log} 2> ${test_log}
    fi
    rc=$? #Save the rc
    

    if [ -f result.txt ]; then
        mv result.txt output/$test_name.result
    fi

    if [[ $rc != 0 ]]; then
        echo "ERROR: Failed to run ${test}"
        echo "Look at log: ${test_log}"
        failed_num=$((failed_num+1))
        failed_list+="output/${test_name}.log "
    else
        grep -i 'Exec time and perf' ${test_log}
    fi
    echo ""

}

############################################################
# Read tclist and run here

while IFS= read -r line
do
    # Skip the comment and blank lines
    if [[ ! $line =~ "#" && -n $line ]]; then
        run_test $line
    fi
done < $tclist



############################################################
# If everything goes well...

good='
　　　　　ｘ　　　ｘｘ　　　　
　　　　　ｘｘ　　ｘ　　　　　
　　ｘｘ　ｘ　　　ｘ　　　　　
　　　ｘ　ｘ　ｘ　ｘｘｘｘｘ　
　　　ｘ　ｘ　ｘｘｘｘｘｘ　　
　　　ｘ　ｘ　ｘ　ｘ　　ｘ　　
　　　ｘ　ｘ　ｘ　ｘ　ｘｘ　　
　　ｘｘｘｘ　ｘ　ｘ　ｘｘ　　
　　　ｘｘｘ　ｘ　ｘｘｘｘ　　
　　　　ｘｘ　ｘ　ｘ　ｘｘ　　
　　　ｘｘ　　　　ｘ　　　　　
　　ｘｘ　　　　　ｘ　　　　　
　　　　　　　　　ｘ　　　　　
'
if (( $failed_num == 0 )); then
    echo "------------------------------------------------------------"
    echo "All Tests PASSED"
    echo "$good"
else
    echo "$failed_num testcases failed. Please check details:"
    echo "------------------------------------------------------------"
    for logfile in $failed_list; do 
        echo $logfile
    done
    echo "------------------------------------------------------------"
    
fi
