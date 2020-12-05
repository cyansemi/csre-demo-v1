# 青芯正则表达式加速引擎演示系统

[V1.0]

## 概述

正则表达式通常被用来检索、替换符合某个模式(规则)的文本。快速处理文本对于许多技术和商业的场合都十分关键。正则表达式匹配被广泛应用于数据库和网络安全领域，例如数据库查询、深度包检测 (Deep Packet Inspection)、抗DDoS攻击等。

正则表达式在数据查询上的丰富和灵活功能无可替代，但软件处理速度慢，拖慢整体系统性能，无法匹配现代网络和处理速度需求。

使用硬件加速可以直接提供可观的性能。正则表达式处理单元可嵌入硬件数据流处理路径中，成为智能网卡或智能存储控制器的重要组件。


## 测试环境

### 硬件配置

服务器配置

| 服务器   | CPU  | 内存 |
|----------|-------| ----|
| Dell R730 | Intel(R) Xeon(R) CPU E5-2690 v3 @ 2.60GHz, 24core, 2 sockets | 256GB |

加速卡配置

| 加速卡 | 开发工具        | Target Platform                       |
|--------|-------------|-------------|
| Xilinx Alveo U50 | Vivado/Vitis 2019.2.1 | xilinx_u50_gen3x16_xdma_201920_3.xfpm |

### 软件和库

操作系统

| 操作系统   | 内核版本 |
| ---------- | -------- |
| Centos 7.8 | 3.10.0   |



需要预先安装的软件库：

```
sudo yum install devtoolset-9
```

需要预先安装Xilinx XRT:

https://www.xilinx.com/products/boards-and-kits/alveo/u50.html#gettingStarted

安装

1. Download the Xilinx Runtime
2. Download the Deployment Target Platform

并设置XILINX_XRT环境变量，例如（请换成xrt的具体安装位置）

```bash
source /opt/xilinx/xrt/setup.sh
```

## 使用步骤

### 目录结构

在demo目录下，包含两个文件夹：Deploy和Test。（如为压缩包请解压）

执行 (Deploy) 的目录结构为：

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

其中:

* `xclbin`为FPGA的烧写文件，进行Partial Reconfiguration。
* `csre-st-ocl`为基于OpenCL的Host端执行文件。
* 另外需要两个库`libcsre-ocl.so`和`librecomp.so`。
* check_regex.sh用于检查正则表达式列表是否被硬件支持。

测试 (Test) 的目录结构为：

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

其中：

* ./pre_check.sh会检查正则表达式，把硬件支持的正则表达式筛选出来，放在`regex_xxxxx.txt.supp` 文件里。
* ./run.sh 会执行测试用例列表`*.tclist`。
* testdata目录存放packet文件和regex文件。



### 基本测试

运行步骤：

```
./pre_check.sh <deploy_folder>
```

这一步利用<deploy_folder>中的测试脚本对testdata目录下的`regex_*.txt` 进行检查。

* `regex_*.txt.unsupp` 不被硬件所支持的正则表达式
* `regex_*.txt.msg` 具体不支持的原因
* `regex_*.txt.supp` 支持的正则表达式，可供tclist使用。



```
 ./run.sh <deploy_folder> <tclist>
```

运行正确时，会打印出

```
All Tests PASSED
```

否则，会输出错误信息在`output` 目录里，分为log文件和result文件。result文件打印了匹配的位置，packet和pattern的ID(即行号)。



例如：

```
cd test
./run.sh <path.....>/deploy/ tclist/func.tclist
./run.sh <path.....>/deploy/ tclist/var.tclist
```

默认情况下，硬件匹配结果会和C的pcre库进行比对。



也可直接执行：

```
<deploy_folder>/csre-st-ocl -k <deploy_folder>/xxx.xclbin -p packet.txt -q regex.txt
```





### 性能测试

```
./run.sh <path.....>/deploy/ tclist/perf.tclist
```

例如：三个Kernel（180MHz）的情况下，会输出：

```
----> [PERF] Exec time and perf: end after 45 msec (5594.867 MB/sec)
```

即实测约5.6GB/s。性能随着Kernel数变多及主频提高会线性提升。



做性能测试时，因为软件算的比较慢，可在perf.tclist里设置`nocheck`, 关掉软件和硬件结果的比对。

一次完整的运行时间由以下部分组成：

* 把xclbin烧到FPGA上的时间（如果上一次执行成功的话，不会重新烧）
* Compile Regex pattern file：如果有不支持的正则表达式，会报错退出。
* Parse packet file：把文件读入内存，并按IP core的需求添加Header。
* 在内存中申请Result Buffer
* FPGA执行：
  * Copy Regex buffer and Packet buffer to FPGA HBM
  * Exec：执行正则表达式扫描
  * Copy Result from FPGA HBM to Result buffer in memory

* 和软件pcre的计算结果进行比较。



其中，FPGA上的第一步，Vitis U50平台上的XDMA进行数据拷贝的速率为8GB/s左右。把正则表达式引擎应用于网络时无须考虑它们。

```
----> [PERF] Copy time and perf: end after 30 msec (8392.422 MB/sec)
----> [PERF] Exec time and perf: end after 45 msec (5594.867 MB/sec)
----> [PERF] Result time and perf: end after 19 msec (3289.474 MB/sec)
```



## 正则表达式IP配置

### 配置内容

注：准确信息可以在<deploy_folder>/CMakeCache.txt中查看。



| 配置项                  | 配置值 | 注释                                                         |
| ----------------------- | ------ | ------------------------------------------------------------ |
| KERNEL_NUM (K)          | 3      | Kernel的个数                                                 |
| KERNEL_FREQ (F)         | 180MHz | 工作频率                                                     |
| CSRE_NUM_PIPELINE (N)   | 16     | Packet pipelines的数目（每个Packet pipeline每cycle处理一个字符byte） |
| CSRE_NUM_PU (M)         | 8      | Pattern pipelines的数目（每个Pattern pipeline处理一个正则表达式） |
| CSRE_MAX_CHAR_NUM       | 32     | 同时匹配的字符数                                             |
| CSRE_MAX_TOKEN_NUM      | 16     | 字符段Token的最大数目                                        |
| CSRE_MAX_CHAR_PER_TOKEN | 16     | 每个Token含有的最大字符数                                    |
| CSRE_MAX_STATE_NUM      | 8      | 自动状态机的状态数目                                         |
| CSRE_MAX_COUNT_NUM      | 4      | {m,n}出现的最大次数                                          |
| C_MAX_PAYLOAD_BYTES     | 2047   | （固定值）Packet Payload的最大字节数                         |

在该配置下，一个kernel能够并行支持M=8个正则表达式，当匹配X=8个正则表达式时，提供理论吞吐率为：


<img src="https://render.githubusercontent.com/render/math?math=%5Cbegin%7Balign*%7D%0ABytes%5C_per%5C_second%20%26%3D%20K%20*%20%5Cfrac%7BN%20*%20min%5C%7BM%2CX%5C%7D%7D%7BX%7D%20*%20F%20%5C%5C%0A%26%3D%20K%20*%20%5Cfrac%7B16%20*%20min%5C%7B8%2C8%5C%7D%7D%7B8%7D%20*%20180%20*%2010%5E6%20%5C%5C%0A%26%3D%20K%20*%202.88%20(GB%2Fs)%20%5C%5C%0A%26%3D%208.64%20(GB%2Fs)%0A%5Cend%7Balign*%7D">


### 资源和功耗

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



### 限制条件

支持的正则表达式规则解释见《正则表达式匹配器-规格说明书》，2.1节，“正则表达式的规则和限制”。
Contact: yong.lu@cyansemi.com

当前所述为Version 1. Version 2正在开发中，支持更多并行正则表达式，并取消了大多数正则表达式的限制。
