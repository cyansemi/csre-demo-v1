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

# This script is used to pre-check the regular expressions before running 
# on hardware; point out the regex that couldn't be parsed by hardware.

# The script requires the directory of "deploy" folder
# and regex_file

usage () {
    echo "Usage: With two arguments:"
    echo "Example: $0 <deploy_folder> <regex_file>"
    echo ""
    echo "Usage: With one argument: (Process all of the regex_*.txt under ./testdata"
    echo "Example: $0 <deploy_folder>"
}

###################################################
if [ -z $1 ]; then
    usage
    exit -1
fi

deploy_folder=`realpath $1` #Convert the input relative path to absolute on
if [ ! -d $deploy_folder ]; then
    echo "ERROR! deploy_folder $deploy_folder doesn't exist"
    exit -1
fi
sh=$deploy_folder/check_regex.sh


###################################################
if [ -z $2 ]; then
    #simfiles need to list explicitly, because it is a link
    if [ -L ./testdata/simfiles ]; then
        argument2="./testdata ./testdata/simfiles/"
    else
        argument2="./testdata"
    fi
    file_list=`find $argument2 -type f -name regex_*.txt`
else
    argument2=$2
    file_list=`find $argument2 -type f`
fi

echo "Start parsing $argument2"

for f in $file_list; do
    file=`realpath $f`
    long_files+=$file
    long_files+=" "
done

for lf in $long_files; do
    $sh $deploy_folder $lf
done
###################################################

