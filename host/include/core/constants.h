/* 
 *  Copyright 2020 CyanSemi Semiconductor Co.
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *  
 *  Date: Sun May 03 2020
 */

#ifndef F_CONSTANTS
#define F_CONSTANTS

#define AXI4_IF_ID_WIDTH              1
#define AXI4_IF_ADDR_WIDTH            64
#define AXI4_IF_DATA_WIDTH            128
#define AXI4_IF_DATA_WIDTH_BYTES      (AXI4_IF_DATA_WIDTH/8)

#define AXI4_LITE_IF_ADDR_WIDTH       32
#define AXI4_LITE_IF_DATA_WIDTH       32

#define SHIM_REG_ADDR_WIDTH           32
#define SHIM_REG_DATA_WIDTH           32

#define PACKET_PREAMBLE_WIDTH         32
#define PACKET_PREAMBLE_WIDTH_BYTES   (PACKET_PREAMBLE_WIDTH/8)
#define REGEX_PREAMBLE_WIDTH          32
#define REGEX_PREAMBLE_WIDTH_BYTES    (REGEX_PREAMBLE_WIDTH/8)

#define PACKET_SIZE_WIDTH             11
#define PACKET_SIZE_WIDTH_BYTES       (PACKET_SIZE_WIDTH/8+1)
#define REGEX_SIZE_WIDTH              11
#define REGEX_SIZE_WIDTH_BYTES        (REGEX_SIZE_WIDTH/8+1)

#define PACKET_ID_WIDTH               32
#define PACKET_ID_WIDTH_BYTES         (PACKET_ID_WIDTH/8)
#define REGEX_ID_WIDTH                32
#define REGEX_ID_WIDTH_BYTES          (REGEX_ID_WIDTH/8)
#define MAX_ID_WIDTH                  ((PACKET_ID_WIDTH > REGEX_ID_WIDTH) ? PACKET_ID_WIDTH : REGEX_ID_WIDTH)
#define MAX_ID_WIDTH_BYTES            ((PACKET_ID_WIDTH_BYTES > REGEX_ID_WIDTH_BYTES) ? PACKET_ID_WIDTH_BYTES : REGEX_ID_WIDTH_BYTES)

#define HEADER_RESERVE_BITS           (AXI4_IF_DATA_WIDTH - PACKET_PREAMBLE_WIDTH - PACKET_SIZE_WIDTH - PACKET_ID_WIDTH)
#define HEADER_RESERVE_BYTES          (HEADER_RESERVE_BITS/8)

#define PACKET_MEANINGFUL_HEADER_WIDTH       (PACKET_SIZE_WIDTH + PACKET_ID_WIDTH)
#define REGEX_MEANINGFUL_HEADER_WIDTH        (REGEX_SIZE_WIDTH + REGEX_ID_WIDTH)

#define PACKET_HEADER_BYTES           (PACKET_PREAMBLE_WIDTH_BYTES + PACKET_SIZE_WIDTH_BYTES + PACKET_ID_WIDTH_BYTES + HEADER_RESERVE_BYTES)
#define REGEX_HEADER_BYTES            (REGEX_PREAMBLE_WIDTH_BYTES + REGEX_SIZE_WIDTH_BYTES + REGEX_ID_WIDTH_BYTES + HEADER_RESERVE_BYTES)

#define PACKET_MIN_BYTE_NUM           PACKET_HEADER_BYTES
// For test, the total size is constrainted within 64MB
#define PACKET_MAX_BYTE_NUM           (1 << 26)

// The actually packet size tested is PACKET_MAX_SIZE - 1
#define PACKET_MAX_SIZE               2048

#define REGEX_MIN_NUM                 0
#define REGEX_MAX_NUM                 8

#define INPUT_BATCH_WIDTH             128
#define INPUT_BATCH_PER_PACKET        1
#define PIPE_INDATA_WIDTH             64
#define REGEX_RESULT_WIDTH            104
#define INPUT_PACKET_WIDTH            128
#define OUTPUT_RESULT_WIDTH           80
#define OUTPUT_RESULT_WIDTH_AXI       64 // TODO: need to set as the same value of OUTPUT_RESULT_WIDTH
#define REGEX_NUM_NFA_RESULTES        8
#define REGEX_NUM_NFA_TOKEN           8

//#define MAX_STATE_NUM                 8
//#define MAX_TOKEN_NUM                 16
//#define MAX_CHAR_NUM                  32
//#define MAX_CHAR_PER_TOKEN            16
//#define MAX_OR_NUM                    8

//#define REGEX_WIDTH_BYTES (REGEX_WIDTH_BITS/8)
//#define REGEX_WIDTH_WITHOUT_ID_BYTES (REGEX_WIDTH_BYTES - REGEX_ID_WIDTH_BYTES)

// Packet pipelines, 16 pipelines in total
#define NUM_BUFFER_SL                   1
#define NUM_BUFFER_TL                   2
#define NUM_BUFFER_4THL                 1
#define NUM_PIPELINE_IN_A_GROUP         1
#define NUM_OF_PIPELINE_GROUP           16
#define NUM_STRING_MATCH_PIPELINE       16

// Regex pipelines, 8 pipelines in total
#define REGEX_NUM_FL                    1
#define REGEX_NUM_SL                    1
#define NUM_OF_PU                       8

#define RESULT_SIZE_WIDTH               12

// Register layout
#define    ADDR_STD_CONTROL          0x00
#define    ADDR_GIE                  0x04//Global Interrupt Enable (not implemented)
#define    ADDR_IER                  0x08//IP Interrupt Enable (not implemented)
#define    ADDR_ISR                  0x0C//IP Interrupt Status (not implemented)
#define    ADDR_PACKET_SRC_ADDR_L    0x10
#define    ADDR_PACKET_SRC_ADDR_H    0x14
#define    ADDR_REGEX_SRC_ADDR_L     0x18
#define    ADDR_REGEX_SRC_ADDR_H     0x1C
#define    ADDR_RESULT_TGT_ADDR_L    0x20
#define    ADDR_RESULT_TGT_ADDR_H    0x24
#define    ADDR_PACKET_TOTAL_SIZE_L  0x28
#define    ADDR_PACKET_TOTAL_SIZE_H  0x2C
#define    ADDR_REGEX_TOTAL_SIZE     0x30
#define    ADDR_RESULT_TOTAL_SIZE_L  0x34
#define    ADDR_RESULT_TOTAL_SIZE_H  0x38
#define    ADDR_RESERVED_1           0x3C
#define    ADDR_KERNEL_STATUS        0x40
#define    ADDR_KERNEL_CONTROL       0x44
#define    ADDR_RESERVED_2           0x48
#define    ADDR_RESERVED_3           0x4C
#define    ADDR_KERNEL_NAME1         0x50
#define    ADDR_KERNEL_NAME2         0x54
#define    ADDR_KERNEL_NAME3         0x58
#define    ADDR_KERNEL_NAME4         0x5C
#define    ADDR_KERNEL_NAME5         0x60
#define    ADDR_KERNEL_NAME6         0x64
#define    ADDR_KERNEL_NAME7         0x68
#define    ADDR_KERNEL_NAME8         0x6C

#define    ADDR_STD_CONTROL_AP_START 0 // ap_start (Read/Write/COH)
#define    ADDR_STD_CONTROL_AP_DONE  1 // ap_done (Read/COR)
#define    ADDR_STD_CONTROL_AP_IDLE  2 // ap_idle (Read)
#define    ADDR_STD_CONTROL_AP_READY 3 // ap_ready (Read)

// Limit the number of packet and regex to a reasonable range
#define MAX_PACKET_COUNT              1024000 // 1M lines of packets, 2GB file size if each line has 2048 bytes.
#define MAX_REGEX_COUNT               REGEX_MAX_NUM

#endif
