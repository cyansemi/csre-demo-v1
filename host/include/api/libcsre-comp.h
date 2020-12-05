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

#ifndef LIBCSRE_COMP_H
#define LIBCSRE_COMP_H

#ifdef __cplusplus
extern "C" {
#endif
#include "core/constants.h"
#include "core/core.h"

/*
 * parse_packet - parse the given file contains packets to be scanned,
 *  convert them to the Regular Expression Kernel Format
 *
 * @file_path: the path to the file contains packets to be scanned,
 *   1 packet per line.
 * @job_packet_buffers[out]: the array of the memory buffer pointers for
 *   each kernel.
 * @job_packet_sizes[out]: the array of the memory buffer size for each kernel,
 * @num_jobs[in]: the number of jobs (equals the number of kernels)
 *
 * returns
 * integer indicating succeeded if 0, failed otherwise
 */
int csre_parse_packet (const char * file_path, void ** job_packet_buffers, size_t * job_packet_sizes, int num_jobs);

/*
 * compile - compile the given regular expression to the Regular Expression
 *   Kernel Format
 *
 * @patt: the regular expression to be processed
 * @regex_buffers [out]: the pointer to the memory buffer contains the
 *  processed result
 * @size [out]: the size of the memory buffer
 *
 * returns
 * integer indicating succeeded if 0, failed otherwise
 */
int csre_compile (const char * patt, void ** regex_buffers, size_t * size);

/*
 * compile_file - compile regular expressions in given file to the Regular
 *   Expression Kernel Format
 *
 * @patt: the regular expression to be processed
 * @regex_buffers [out]: the pointer to the memory buffer contains the
 *  processed result
 * @size [out]: the size of the memory buffer
 *
 * returns
 * integer indicating succeeded if 0, failed otherwise
 */
int csre_compile_file (const char * regex_file, void ** regex_buffers, size_t * size);

#ifdef __cplusplus
}
#endif

#endif // LIBCSRE_COMP_H
