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

/*
 * Copyright 2019 International Business Machines
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ref/regex_ref.h"
#include "recomp/regex_config.h"
#include "api/libcsre.h"

extern csre_config_t csre_config;

int csre_parse_packet (const char * file_path, void ** job_packet_buffers, size_t * job_packet_sizes, int number_of_kernels)
{
    FILE * fp = fopen (file_path, "r");

    if (NULL == fp) {
        printf ("ERROR: invalid packet file %s\n", file_path);
        return -1;
    }

    int pkt_count = get_file_line_count (fp);
    fseek (fp, 0, SEEK_SET);

    if (MAX_PACKET_COUNT < pkt_count) {
        printf ("ERROR: number of packets in file (%d) exceeds the limit (%d)\n", pkt_count, MAX_PACKET_COUNT);
        return -1;
    }

    if (0 == (pkt_count / number_of_kernels)) {
        printf ("ERROR: at least %d packet(s) are needed, get %d for now!\n", number_of_kernels, pkt_count);
        return -1;
    }

    for (int i = 0; i < number_of_kernels; i++) {
        int pkt_count_last = (pkt_count / number_of_kernels) + (pkt_count % number_of_kernels);
        int pkt_count_per_job = (i == (number_of_kernels - 1)) ? pkt_count_last : pkt_count_last;
        size_t max_packet_buffer_size_job = csre_align ((PACKET_HEADER_BYTES + PACKET_MAX_SIZE)) * pkt_count_per_job;

        job_packet_buffers[i] = alloc_mem (PACKET_HEADER_BYTES, max_packet_buffer_size_job);
    }

    if (packet_parse_file (fp, number_of_kernels, job_packet_buffers, job_packet_sizes, pkt_count)) {
        printf ("ERROR: failed to parse packet file");
        return -1;
    }

    fclose (fp);

    return 0;
}

int csre_compile (const char * patt, void ** regex_buffers, size_t * size)
{
    size_t max_regex_buffer_size = csre_align ((REGEX_HEADER_BYTES + csre_config.CSRE_REGEX_WIDTH_WITHOUT_ID_BYTES));
    *regex_buffers = alloc_mem (REGEX_HEADER_BYTES, max_regex_buffer_size);

    if (regex_compile (patt, *regex_buffers, size)) {
        printf ("ERROR: failed to compile regex!");
        return -1;
    }

    return 0;
}

int csre_compile_file (const char * regex_file, void ** regex_buffers, size_t * size)
{
    FILE * fp = fopen (regex_file, "r");

    if (NULL == fp) {
        printf ("ERROR: invalid regex file %s\n", regex_file);
        return -1;
    }

    int regex_count = get_file_line_count (fp);
    fseek (fp, 0, SEEK_SET);

    if (MAX_PACKET_COUNT < regex_count) {
        printf ("ERROR: number of regex in file (%d) exceeds the limit (%d)\n", regex_count, MAX_REGEX_COUNT);
        return -1;
    }

    size_t max_regex_buffer_size = csre_align ((REGEX_HEADER_BYTES + csre_config.CSRE_REGEX_WIDTH_WITHOUT_ID_BYTES)) * regex_count;

    *regex_buffers = alloc_mem (REGEX_HEADER_BYTES, max_regex_buffer_size);

    if (regex_compile_file (fp, *regex_buffers, size)) {
        printf ("ERROR: failed to compile regex!");
        return -1;
    }

    fclose (fp);

    return 0;
}
