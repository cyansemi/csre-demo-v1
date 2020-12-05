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

#ifndef LIBCSRE_H
#define LIBCSRE_H

#ifdef __cplusplus
extern "C" {
#endif
#include "libcsre-comp.h"
/*
 * init_hardware - initialize FPGA hardware including the card handler,
 *  action handler, runtime worker, etc.
 *
 * @binaryFile: The XCLBIN file path
 * @number_of_kernels: Number of available kernrels in FPGA
 *
 * returns
 * integer indicating succeeded if 0, failed otherwise
 */
int csre_init_hardware (const char * binaryFile, int * number_of_kernels);

/*
 * scan - perform the regular expression matching in FPGA
 *
 * @regex_buffers: the pointer to the regex memory buffer given
 *   by compile()
 * @regex_buffer_size: the size of the regex memory buffer given by
 *   compile()
 * @job_packet_buffers: the array of the packet memory buffer pointers
 *   given by parse_packet()
 * @job_packet_sizes: the array of the packet memory buffer size for
 *   given by parse_packet()
 * @result_buffers: the pointer to the result memory buffer
 * @result_buffer_size: the size of the result memory buffer
 *
 * returns
 * integer indicating succeeded if 0, failed otherwise
 */
int csre_scan (void ** regex_buffers, size_t regex_buffer_size, void ** job_packet_buffers, size_t * job_packet_sizes,
               void ** result_buffers, size_t result_buffer_size);

/*
 * result - get the result of scan()
 *
 * @result_buffers: the pointers to the result buffer
 * @result_file: the file to hold the result
 * @check: check the result with the software reference model
 *
 * returns
 * integer indicating succeeded if 0, failed otherwise
 */
int csre_result (void ** result_buffers, const char * result_file, bool check);

/*
 * close_hardware - close the FPGA card and cleanup
 *
 * returns
 * integer indicating succeeded if 0, failed otherwise
 */
int csre_close_hardware();
#ifdef __cplusplus
}
#endif

#endif // LIBCSRE_H
