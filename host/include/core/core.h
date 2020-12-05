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
 * Copyright 2017 International Business Machines
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

#ifndef __CORE_H__
#define __CORE_H__

#include "unistd.h"
#include "core/constants.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/time.h>
#include <getopt.h>
#include <ctype.h>

#define MEGAB       (1024*1024ull)
#define GIGAB       (1024 * MEGAB)

typedef struct {
    int CSRE_MAX_TOKEN_NUM;
    int CSRE_MAX_STATE_NUM;
    int CSRE_MAX_CHAR_NUM;
    int CSRE_MAX_CHAR_PER_TOKEN;
    int CSRE_MAX_COUNT_NUM;
    int CSRE_REGEX_WIDTH_BYTES;
    int CSRE_REGEX_WIDTH_WITHOUT_ID_BYTES;
} csre_config_t;

int update_csre_regex_configs(csre_config_t in);
int64_t diff_time (struct timespec * t_beg, struct timespec * t_end);
uint64_t get_usec (void);
int get_file_line_count (FILE * fp);
int csre_align(int in);
void remove_newline (char * str);
float print_time (uint64_t elapsed, uint64_t size);
void * alloc_mem (int align, size_t size);
void free_mem (void * a);

void * fill_one_packet (const char * in_pkt, int size, void * in_pkt_addr, int in_pkt_id);
void * fill_one_pattern (const char * in_patt, void * in_patt_addr);

int regex_compile_file (FILE * fp, void * patt_src_base, size_t * size);
int regex_compile (const char * patt, void * patt_src_base, size_t * size);
int packet_parse_file (FILE * file_path, int num_jobs, void ** job_pkt_src_bases, size_t * job_sizes, int pkt_count);
int process_results (void * result_dest_base, bool check, int * processed_results, FILE * out_fp);
int compare_num_matched_pkt (size_t num_matched_pkt);
int compare_result_id (uint32_t pkt_id, uint32_t patt_id, uint64_t offset);

static inline void __hexdump(FILE *fp, const void *buff, unsigned int size)
{
	unsigned int i;
	const uint8_t *b = (uint8_t *)buff;
	char ascii[17];
	char str[2] = { 0x0, };

	if (size == 0)
		return;

	for (i = 0; i < size; i++) {
		if ((i & 0x0f) == 0x00) {
			fprintf(fp, " %08x:", i);
			memset(ascii, 0, sizeof(ascii));
		}
		fprintf(fp, " %02x", b[i]);
		str[0] = isalnum(b[i]) ? b[i] : '.';
		str[1] = '\0';
		strncat(ascii, str, sizeof(ascii) - 1);

		if ((i & 0x0f) == 0x0f)
			fprintf(fp, " | %s\n", ascii);
	}
	/* print trailing up to a 16 byte boundary. */
	for (; i < ((size + 0xf) & ~0xf); i++) {
		fprintf(fp, "   ");
		str[0] = ' ';
		str[1] = '\0';
		strncat(ascii, str, sizeof(ascii) - 1);

		if ((i & 0x0f) == 0x0f)
			fprintf(fp, " | %s\n", ascii);
	}
	fprintf(fp, "\n");
}

extern int verbose_level;
extern bool show_result;
#endif
