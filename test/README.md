# Cyan Semiconductor Regular Expression Test Suite


## Preparation

Get the deploy folder, which usually is built manually or provided by the vendor. 

## Steps
To avoid the DOS line-ends, run `dos2unix` if the files are uploaded from a Windows machine.

1. Prepare the packet file. The lines in packet file should not exceed 2047 bytes. It can be viewed by 
```
wc -L <packet_file.txt>
```

2. Prepare the regex list file. Check whether they can be handled by the hardware: 
```
./pre-check.sh <deploy_folder> <regex_file.txt>
```

`regex_file.txt.supp` (with suffix "supp") lists the supported ones.

`regex_file.txt.unsupp` (with suffix "unsupp") lists the regular expressions which are not supported.

`regex_file.txt.msg`(with suffix "msg") tells the failed reasons.


3. Write tclist and put it in folder `tclist`. Some examples are already there. Use `*.supp` file in the regex column. 

4. Run on hardware
```
./run.sh <deploy_folder> tclist/<testcase.tclist>
```

Examples:
```
./run.sh <deploy_folder> tclist/func.tclist
```

The output logs and match-results are saved in `output` folder. 

## Utils

* `testdata/large` contains some large files stored in ZIP format. They will be decompressed for the first time. `remove_bigfiles.sh` will remove the decompressed files.

* `cleanup.sh` will remove files in `output` folder and the generated `*.supp`, `*.unsupp` and `*.msg` files. 


