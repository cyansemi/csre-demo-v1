# Cyansemi RegEx Pattern Match FPGA Acceleration Demo

[V1.0]

![中文版README](README.CN.md) 

## Introducation

Regular Expressions are usually used to search or replace text with certain pattern. It's very crucial to process pattern match in many technical and business scenes. Regular Expressions are widely used in database and networking area, like parallel database query, deep packet inspection, DDOS and so on.

Although there are many algorithms to match multiple string patterns today, regular expressions still cannot be replaced because of its flexibility and powerful functions. However, regular expressions are also much more difficult to process, especially by software. Software cannot provide enough data processing throughput to match modern and future speed of data transactions.

This regular expression pattern match demo is coded in Verilog. Hardware acceleration can provide powerful speed. It can also be inserted into the network packets processing data path, as a part of SmartNIC or Smart Storage controller. 


## Test Environment

### Hardware Configuration

Test Server

| Server   | CPU   | Memory |
|----------|-------| ----|
| Dell R730 | Intel(R) Xeon(R) CPU E5-2690 v3 @ 2.60GHz, 24core, 2 sockets | 256GB |


FPGA Card

| FPGA Card | Developing Tools        | Target Platform                       |
|--------|-------------|-------------|
| Xilinx Alveo U50 | Vivado/Vitis 2019.2.1 | xilinx_u50_gen3x16_xdma_201920_3.xfpm |

### Software and Library

Operating System

| OS   | Kernel |
| ---------- | -------- |
| Centos 7.8 | 3.10.0   |


Software Requirements (Install them if not valid)

```
sudo yum install devtoolset-9
```

Xilinx XRT:

https://www.xilinx.com/products/boards-and-kits/alveo/u50.html#gettingStarted

How to install:

1. Download the Xilinx Runtime
2. Download the Deployment Target Platform

Set the environmental variable (Use your xrt path)

```bash
source /opt/xilinx/xrt/setup.sh
```

## How to run demo

### Directory Structure

In folder *"demo"*, decompress the tar packages: Deploy and Test.

Structure of "Deploy" folder:

```
k3_f180_char32_state8_pattbuf2_20200628-Sun-160244_2006670856_Performance_Explore
├── CMakeCache.txt
├── deploy
│   ├── check_regex.sh
│   ├── csre_check_regex
│   ├── csre_kn3_freq180_char32_state8_count4_05d3f08.xclbin
│   ├── csre-st-ocl
│   ├── libcsre-ocl.so
│   └── librecomp.so
└── runme.log
```



* `xclbin` is the programmable binary for FPGA (using Partial Reconfiguration)
* `csre-st-ocl` is the Host application executable (using OpenCL)
* Other two libraries are needed: `libcsre-ocl.so` and `librecomp.so`.
* check_regex.sh is used to check whether the regular expressions are supported. (The are are some limitations by the hardware kernel)

Structure of "Test" folder:

```
test
├── cleanup.sh
├── origin
├── output
├── pre_check.sh
├── README.md
├── remove_bigfiles.sh
├── run.sh
├── tclist
│   ├── func.tclist
│   ├── var.tclist
│   ├── perf.tclist
│   └── sim.tclist
└── testdata
    ├── large
    ├── quick
    ├── simfiles
    └── specific
```


* ./pre_check.sh will parse the supported regular expressions and put them in file `regex_xxxxx.txt.supp`.
* ./run.sh uses testcase list `*.tclist`。
* Packet files and regex files are in Testdata folder.



### Basic Test


```
./pre_check.sh <deploy_folder>
```

Use the script in <deploy_folder> to check `regex_*.txt` files. (Because the limitations depend on the detailed parameters configured to hardware kernel so pre_check.sh needs to get the information from deploy folder.)

* `regex_*.txt.unsupp` un-supported regular expressions
* `regex_*.txt.msg` the detailed reason/message why they are not supported.
* `regex_*.txt.supp` supported regular expressions which can be used by the hardware kernel.


```
 ./run.sh <deploy_folder> <tclist>
```

If everything is fine, it will print:

```
All Tests PASSED
```

In `output` folder, log file tells what has been done and result file prints the matched positions, packet ID and pattern ID.

For example, run as following:

```
cd test
./run.sh <path.....>/deploy/ tclist/func.tclist
./run.sh <path.....>/deploy/ tclist/var.tclist
```

The matching results will be compared with C pcre library by default.


There is another way not using 'tclist' but directly providing the input packet file name and pattern file name:

```
<deploy_folder>/csre-st-ocl -k <deploy_folder>/xxx.xclbin -p packet.txt -q regex.txt
```



### Performance Test

```
./run.sh <path.....>/deploy/ tclist/perf.tclist
```

When there are 3 kernels @ 180MHz, it will have:

```
----> [PERF] Exec time and perf: end after 45 msec (5594.867 MB/sec)
```

The bandwidth is linear proportional to the number of kernels and clock frequency.

When running performance test, because the speed of software reference is tooooo slow, you can set `nocheck` in perf.tclist to turn off the result checking (with software pcre).

A complete run includes following steps of time:

* Program the xclbin to FPGA card. If previous programming is successful, it will not be reprogrammed.
* Compile Regex pattern file. If some regular expressions cannot be handled by hardware kernel, it will stop and report errors. (That's why pre_check.sh is needed). Store the compiled pattern db content in host memory and insert headers.
* Read packet file into host memory and insert headers according to the software-hardware protocol.
* Apply result buffer in host memory.
* Run on FPGA (pipelined):
  * Copy Regex buffer and Packet buffer into FPGA HBM
  * Exec: scan the packets with patterns in parallel
  * Copy Result from FPGA HBM to Result buffer in host memory. 
* Compare the matching results with software PCRE library.

Example time spend on FPGA:
```
----> [PERF] Copy time and perf: end after 30 msec (8392.422 MB/sec)
----> [PERF] Exec time and perf: end after 45 msec (5594.867 MB/sec)
----> [PERF] Result time and perf: end after 19 msec (3289.474 MB/sec)
```



## Configure Regex Pattern Match IP

### Parameters

The parameters can be found in <deploy_folder>/CMakeCache.txt


| Item                  | Value | Description                                                      |
| ----------------------- | ------ | ------------------------------------------------------------ |
| KERNEL_NUM (K)          | 3      | Number of kernels                                                 |
| KERNEL_FREQ (F)         | 180MHz | Clock frequency                                            |
| CSRE_NUM_PIPELINE (N)   | 16     | Number of Packet pipelines: Each packet pipeline process 1 character per cycle |
| CSRE_NUM_PU (M)         | 8      | Number of Pattern pipelines: Each pattern pipeline process 1 regular expression |
| CSRE_MAX_CHAR_NUM       | 32     | Number of character matchers or the length of regular expression          |
| csre_max_token_num      | 16     | The maximum number of characters in one token (sub string)                |
| csre_max_char_per_token | 16     | The maximum number of characters in one token (sub string)                |
| csre_max_state_num      | 8      | State number of NFA                                       |
| csre_max_count_num      | 4      | Max number of {m,n}                                       |
| c_max_payload_bytes     | 2047   | (Fixed) Max Packet Payload length                         |

With the above configuration, one kernel can process M=8 regular expressions at same time. The theoretical bandwidth is: 

<img src="https://render.githubusercontent.com/render/math?math=%5Cbegin%7Balign*%7D%0ABytes%5C_per%5C_second%20%26%3D%20K%20*%20%5Cfrac%7BN%20*%20min%5C%7BM%2CX%5C%7D%7D%7BX%7D%20*%20F%20%5C%5C%0A%26%3D%20K%20*%20%5Cfrac%7B16%20*%20min%5C%7B8%2C8%5C%7D%7D%7B8%7D%20*%20180%20*%2010%5E6%20%5C%5C%0A%26%3D%20K%20*%202.88%20(GB%2Fs)%20%5C%5C%0A%26%3D%208.64%20(GB%2Fs)%0A%5Cend%7Balign*%7D">


### Utilizaiton and Power

(@Alveo U50 Vitis Platform)

```
+--------------------------+-----------+----------+-----------+-----------------+
| On-Chip                  | Power (W) | Used     | Available | Utilization (%) |
+--------------------------+-----------+----------+-----------+-----------------+
| Clocks                   |     2.464 |       56 |       --- |             --- |
| CLB Logic                |     1.894 |  1311761 |       --- |             --- |
|   LUT as Logic           |     1.235 |   475034 |    870016 |           54.60 |
|   LUT as Distributed RAM |     0.421 |     8613 |    402016 |            2.14 |
|   Register               |     0.137 |   665852 |   1740032 |           38.27 |
|   LUT as Shift Register  |     0.072 |     4421 |    402016 |            1.10 |
|   CARRY8                 |     0.029 |    17768 |    108752 |           16.34 |
|   BUFG                   |    <0.001 |        4 |        64 |            6.25 |
|   Others                 |    <0.001 |     9779 |       --- |             --- |
|   F7/F8 Muxes            |     0.000 |     2342 |    870016 |            0.27 |
| Signals                  |     3.132 |  1062891 |       --- |             --- |
| Block RAM                |     1.532 |    412.5 |      1344 |           30.69 |
| HBM                      |     3.468 |        2 |         2 |          100.00 |
| MMCM                     |    <0.001 |        0 |       --- |             --- |
| PLL                      |     0.054 |        1 |       --- |             --- |
| DSPs                     |    <0.001 |        4 |      5940 |            0.07 |
| I/O                      |     0.006 |       10 |       416 |            2.40 |
| GTY                      |     2.863 |       16 |        20 |           80.00 |
| Hard IPs                 |     0.538 |        1 |       --- |             --- |
|   PCIE                   |     0.538 |        1 |       --- |             --- |
| Static Power             |     2.599 |          |           |                 |
|   HBM Static             |     0.264 |          |           |                 |
|   Device Static          |     2.335 |          |           |                 |
| Total                    |    18.551 |          |           |                 |
+--------------------------+-----------+----------+-----------+-----------------+
```

Power: 

```
+--------------------------+--------------+
| Total On-Chip Power (W)  | 18.571       |
|   FPGA Power (W)         | 16.533       |
|   HBM Power (W)          | 2.038        |
| Design Power Budget (W)  | 63.000       |
| Power Budget Margin (W)  | 44.429 (MET) |
| Dynamic (W)              | 15.972       |
| Device Static (W)        | 2.599        |
| Effective TJA (C/W)      | 0.5          |
| Max Ambient (C)          | 90.2         |
| Junction Temperature (C) | 34.8         |
| Confidence Level         | Medium       |
| Setting File             | ---          |
| Simulation Activity File | ---          |
| Design Nets Matched      | NA           |
+--------------------------+--------------+
```



### Regular Expression Limitations

Contact yong.lu@cyansemi.com for more information. 

*All of above is the demo for Version 1. Version 2 is under development and will support more parallel regular expressions and with less limitations.*
