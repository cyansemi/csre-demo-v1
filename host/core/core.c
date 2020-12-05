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

#include "core/core.h"
#include "recomp/fregex.h"
#include "ref/regex_ref.h"

#define VERBOSE0(fmt, ...) do {         \
        printf(fmt, ## __VA_ARGS__);    \
    } while (0)

#define VERBOSE1(fmt, ...) do {         \
        if (verbose_level > 0)          \
            printf(fmt, ## __VA_ARGS__);    \
    } while (0)

#define VERBOSE2(fmt, ...) do {         \
        if (verbose_level > 1)          \
            printf(fmt, ## __VA_ARGS__);    \
    } while (0)


#define VERBOSE3(fmt, ...) do {         \
        if (verbose_level > 2)          \
            printf(fmt, ## __VA_ARGS__);    \
    } while (0)

#define VERBOSE4(fmt, ...) do {         \
        if (verbose_level > 3)          \
            printf(fmt, ## __VA_ARGS__);    \
    } while (0)

static uint32_t PATTERN_ID = 0;
int verbose_level = 0;

csre_config_t csre_config = {16, 16, 32, 16, 4, 234, 230};

int update_csre_regex_configs (csre_config_t in)
{
    memcpy (&csre_config, &in, sizeof (csre_config_t));

    if ((in.CSRE_MAX_STATE_NUM <= 0) || (in.CSRE_MAX_CHAR_PER_TOKEN <= 0)
            || (in.CSRE_MAX_CHAR_NUM <= 0)  || (in.CSRE_MAX_TOKEN_NUM <= 0)
            || (in.CSRE_MAX_COUNT_NUM <= 0)) {
        return -1;
    }

    // The width of REGEX is calculated per the following equation
    int csre_regex_width_bits =
        REGEX_ID_WIDTH +
        csre_config.CSRE_MAX_CHAR_NUM * 16 +
        csre_config.CSRE_MAX_STATE_NUM * 8 +
        8 + 8 +
        csre_config.CSRE_MAX_STATE_NUM + 
        csre_config.CSRE_MAX_STATE_NUM * csre_config.CSRE_MAX_CHAR_NUM +
        csre_config.CSRE_MAX_STATE_NUM * csre_config.CSRE_MAX_STATE_NUM +
        csre_config.CSRE_MAX_STATE_NUM * csre_config.CSRE_MAX_STATE_NUM +
        csre_config.CSRE_MAX_STATE_NUM +
        csre_config.CSRE_MAX_COUNT_NUM * 32; // TODO: hardcoded COUNT_ENTRY_SIZE

    csre_config.CSRE_REGEX_WIDTH_BYTES =  csre_regex_width_bits / 8;
    int regex_width_without_id_bytes = csre_config.CSRE_REGEX_WIDTH_BYTES - REGEX_ID_WIDTH_BYTES;
    csre_config.CSRE_REGEX_WIDTH_WITHOUT_ID_BYTES = regex_width_without_id_bytes;

    VERBOSE0 ("CSRE CONFIG - MAX_CHAR_NUM                 : %d\n", csre_config.CSRE_MAX_CHAR_NUM);
    VERBOSE0 ("CSRE CONFIG - MAX_STATE_NUM                : %d\n", csre_config.CSRE_MAX_STATE_NUM);
    VERBOSE0 ("CSRE CONFIG - MAX_TOKEN_LEN                : %d\n", csre_config.CSRE_MAX_CHAR_PER_TOKEN);
    VERBOSE0 ("CSRE CONFIG - MAX_TOKEN_NUM                : %d\n", csre_config.CSRE_MAX_TOKEN_NUM);
    VERBOSE0 ("CSRE CONFIG - MAX_COUNT_NUM                : %d\n", csre_config.CSRE_MAX_COUNT_NUM);
    VERBOSE0 ("CSRE CONFIG - REGEX_WIDTH_BYTES            : %d\n", csre_config.CSRE_REGEX_WIDTH_BYTES);
    VERBOSE0 ("CSRE CONFIG - REGEX_WIDTH_WITHOUT_ID_BYTES : %d\n", csre_config.CSRE_REGEX_WIDTH_WITHOUT_ID_BYTES);

    return 0;
}

int64_t diff_time (struct timespec * t_beg, struct timespec * t_end)
{
    if (t_end == NULL || t_beg == NULL) {
        return 0;
    }

    return ((t_end-> tv_sec - t_beg-> tv_sec) * 1000000000L + t_end-> tv_nsec - t_beg-> tv_nsec);
}

uint64_t get_usec (void)
{
    struct timeval t;

    gettimeofday (&t, NULL);
    return t.tv_sec * 1000000 + t.tv_usec;
}

int get_file_line_count (FILE * fp)
{
    int lines = 0;
    char ch;

    while (!feof (fp)) {
        ch = fgetc (fp);

        if (ch == '\n') {
            lines++;
        }
    }

    return lines;
}

int csre_align (int in)
{
    if (0 == (in % AXI4_IF_DATA_WIDTH_BYTES)) {
        return in;
    }

    return in + (AXI4_IF_DATA_WIDTH_BYTES - (in % AXI4_IF_DATA_WIDTH_BYTES));
}

void remove_newline (char * str)
{
    char * pos;

    if ((pos = strchr (str, '\n')) != NULL) {
        *pos = '\0';
    } else {
        VERBOSE0 ("Input too long for remove_newline ... ");
        exit (EXIT_FAILURE);
    }
}

float print_time (uint64_t elapsed, uint64_t size)
{
    int t;
    float fsize = (float)size / (1024 * 1024);
    float ft;

    if (elapsed > 10000) {
        t = (int)elapsed / 1000;
        ft = (1000 / (float)t) * fsize;
        VERBOSE0 (" end after %d msec (%0.3f MB/sec)\n", t, ft);
        //VERBOSE0 ("%d msec %0.3f\n", t, ft);
    } else {
        t = (int)elapsed;
        ft = (1000000 / (float)t) * fsize;
        VERBOSE0 (" end after %d usec (%0.3f MB/sec)\n", t, ft);
        //VERBOSE0 ("%d usec %0.3f\n", t, ft);
    }

    return ft;
}

void * alloc_mem (int align, size_t size)
{
    void * a;
    size_t size2 = size + align;

    VERBOSE2 ("%s Enter Align: %d Size: %zu\n", __func__, align, size);

    if (posix_memalign ((void **)&a, 4096, size2) != 0) {
        perror ("FAILED: posix_memalign()");
        return NULL;
    }

    VERBOSE2 ("%s Exit %p\n", __func__, a);
    return a;
}

void free_mem (void * a)
{
    VERBOSE2 ("Free Mem %p\n", a);

    if (a) {
        free (a);
    }
}

void * fill_one_packet (const char * in_pkt, int size, void * in_pkt_addr, int in_pkt_id)
{
    unsigned char * pkt_base_addr = in_pkt_addr;
    int pkt_id;
    uint32_t bytes_used = 0;
    uint16_t pkt_len = size;

    // The TAG ID
    pkt_id = in_pkt_id;

    VERBOSE2 ("PKT[%d] %s len %d\n", pkt_id, in_pkt, pkt_len);

    // The frame header
    for (int i = 0; i < PACKET_PREAMBLE_WIDTH_BYTES; i++) {
        pkt_base_addr[bytes_used] = 0x5A;
        bytes_used ++;
    }

    // The frame size
    pkt_base_addr[bytes_used] = (pkt_len & 0xFF);
    bytes_used ++;
    pkt_base_addr[bytes_used] = 0;
    pkt_base_addr[bytes_used] |= ((pkt_len >> 8) & 0xF);
    bytes_used ++;

    // Skip the reserved bytes
    for (int i = 0; i < HEADER_RESERVE_BYTES; i++) {
        pkt_base_addr[bytes_used] = 0;
        bytes_used++;
    }

    for (int i = 0; i < PACKET_ID_WIDTH_BYTES; i++) {
        pkt_base_addr[bytes_used] = ((pkt_id >> (8 * i)) & 0xFF);
        bytes_used++;
    }

    // The payload
    for (int i = 0; i < pkt_len; i++) {
        pkt_base_addr[bytes_used] = in_pkt[i];
        bytes_used++;
    }

    uint32_t remaining = ((uint64_t)pkt_base_addr + bytes_used) % AXI4_IF_DATA_WIDTH_BYTES;

    if (remaining == 0) {
        return pkt_base_addr + bytes_used;
    }

    for (int i = 0; i < (AXI4_IF_DATA_WIDTH_BYTES - remaining); i++) {
        pkt_base_addr[bytes_used] = 0;
        bytes_used++;
    }

    //// Padding to 64 bytes alignment
    //bytes_used--;

    //do {
    //    if ((((uint64_t) (pkt_base_addr + bytes_used)) & 0x3F) == 0x3F) { //the last address of the packet stream is 512bit/64byte aligned
    //        break;
    //    } else {
    //        bytes_used ++;
    //        pkt_base_addr[bytes_used] = 0x00; //padding 8'h00 until the 512bit/64byte alignment
    //    }

    //}   while (1);

    //bytes_used++;

    return pkt_base_addr + bytes_used;

}

void * fill_one_pattern (const char * in_patt, void * in_patt_addr)
{
    unsigned char * patt_base_addr = in_patt_addr;
    int config_len = 0;
    unsigned char config_bytes[csre_config.CSRE_REGEX_WIDTH_BYTES];
    int x;
    uint32_t pattern_id;
    uint16_t patt_byte_cnt;
    uint32_t bytes_used = 0;

    // fregex_get_config will detect if the pattern is legal
    //if (strlen (in_patt) > csre_config.CSRE_MAX_CHAR_NUM) {
    //    printf ("ERROR: The regex length (%zu) exceeds the limit (%d).\n",
    //            strlen (in_patt), csre_config.CSRE_MAX_CHAR_NUM);
    //    return NULL;
    //}

    for (x = 0; x < csre_config.CSRE_REGEX_WIDTH_BYTES; x++) {
        config_bytes[x] = 0;
    }

    // Generate pattern ID
    PATTERN_ID ++;
    pattern_id = PATTERN_ID;

    VERBOSE1 ("PATT[%d] %s\n", pattern_id, in_patt);

    if (0 > fregex_get_config (in_patt,
                               csre_config.CSRE_MAX_TOKEN_NUM,
                               csre_config.CSRE_MAX_STATE_NUM,
                               csre_config.CSRE_MAX_CHAR_NUM,
                               csre_config.CSRE_MAX_CHAR_PER_TOKEN,
                               csre_config.CSRE_MAX_COUNT_NUM,
                               config_bytes,
                               &config_len,
                               0)) {
        printf ("ERROR: failed to compile the regex string %s\n", in_patt);
        return NULL;
    }

    VERBOSE2 ("Config length (bits)  %d\n", config_len * 8);
    VERBOSE2 ("Config length (bytes) %d\n", config_len);

    for (int i = 0; i < REGEX_PREAMBLE_WIDTH_BYTES; i++) {
        patt_base_addr[bytes_used] = 0x5A;
        bytes_used++;
    }

    patt_byte_cnt = (csre_config.CSRE_REGEX_WIDTH_BYTES - REGEX_ID_WIDTH_BYTES);
    patt_base_addr[bytes_used] = patt_byte_cnt & 0xFF;
    bytes_used ++;
    patt_base_addr[bytes_used] = (patt_byte_cnt >> 8) & 0x7;
    bytes_used ++;

    for (int i = 0; i < HEADER_RESERVE_BYTES; i++) {
        patt_base_addr[bytes_used] = 0x00;
        bytes_used ++;
    }

    // Pattern ID;
    for (int i = 0; i < REGEX_ID_WIDTH_BYTES; i++) {
        patt_base_addr[bytes_used] = (pattern_id >> (i * 8)) & 0xFF;
        bytes_used ++;
    }

    memcpy (patt_base_addr + bytes_used, config_bytes, config_len);
    bytes_used += config_len;

    uint32_t remaining = ((uint64_t)patt_base_addr + bytes_used) % AXI4_IF_DATA_WIDTH_BYTES;

    if (remaining == 0) {
        return patt_base_addr + bytes_used;
    }

    for (int i = 0; i < (AXI4_IF_DATA_WIDTH_BYTES - remaining); i++) {
        patt_base_addr[bytes_used] = 0;
        bytes_used++;
    }

    //// Padding to 64 bytes alignment
    //bytes_used --;

    //do {
    //    if ((((uint64_t) (patt_base_addr + bytes_used)) & 0x3F) == 0x3F) { //the last address of the packet stream is 512bit/64byte aligned
    //        break;
    //    } else {
    //        bytes_used ++;
    //        patt_base_addr[bytes_used] = 0x00; //padding 8'h00 until the 512bit/64byte alignment
    //    }

    //} while (1);

    //bytes_used ++;

    return patt_base_addr + bytes_used;

}

int regex_compile_file (FILE * fp, void * patt_src_base, size_t * size)
{
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    PATTERN_ID = 0;

    if (fp == NULL) {
        VERBOSE0 ("ERROR: invalid file handler\n");
        return -1;
    }

    if (NULL == patt_src_base) {
        VERBOSE0 ("ERROR: invalid pattern input buffer!\n");
        return -1;
    }

    void * patt_src = patt_src_base;

    VERBOSE1 ("PATTERN Source Address Start at 0X%016lX\n", (uint64_t)patt_src);

    while ((read = getline (&line, &len, fp)) != -1) {
        remove_newline (line);
        read--;
        VERBOSE3 ("Pattern line read with length %zu :\n", read);
        VERBOSE3 ("%s\n", line);
        patt_src = fill_one_pattern (line, patt_src);

        if (NULL == patt_src) {
            VERBOSE0 ("ERROR compiling pattern!\n");
            return -1;
        }

        // regex ref model
        // regex_ref_push_pattern (line);
        VERBOSE3 ("Pattern Source Address 0X%016lX\n", (uint64_t)patt_src);
    }

    VERBOSE1 ("Total size of pattern buffer used: %ld\n", (uint64_t) (patt_src - patt_src_base));

    VERBOSE1 ("---------- Pattern Buffer: %p\n", patt_src_base);

    if (verbose_level > 2) {
        __hexdump (stdout, patt_src_base, (patt_src - patt_src_base));
    }

    if (line) {
        free (line);
    }

    (*size) = patt_src - patt_src_base;

    return 0;
}

int regex_compile (const char * patt, void * patt_src_base, size_t * size)
{
    const char * line = patt;

    VERBOSE0 ("Running with regular expression: %s\n", line);

    if (NULL == patt_src_base) {
        VERBOSE0 ("ERROR: invalid pattern input buffer!\n");
        return -1;
    }

    void * patt_src = patt_src_base;

    VERBOSE1 ("PATTERN Source Address Start at 0X%016lX\n", (uint64_t)patt_src);
    patt_src = fill_one_pattern (line, patt_src);

    if (NULL == patt_src) {
        VERBOSE0 ("ERROR compiling pattern!\n");
        return -1;
    }

    // regex ref model
    // regex_ref_push_pattern (line);
    VERBOSE3 ("Pattern Source Address 0X%016lX\n", (uint64_t)patt_src);
    VERBOSE1 ("Total size of pattern buffer used: %ld\n", (uint64_t) (patt_src - patt_src_base));
    VERBOSE1 ("---------- Pattern Buffer: %p\n", patt_src_base);

    if (verbose_level > 2) {
        __hexdump (stdout, patt_src_base, (patt_src - patt_src_base));
    }

    (*size) = patt_src - patt_src_base;

    return 0;
}

int packet_parse_file (FILE * fp, int num_jobs, void ** job_pkt_src_bases, size_t * job_sizes, int pkt_count)
{
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int lines_read = 0;
    int curr_job_id = 0;

    if (NULL == fp) {
        VERBOSE0 ("ERROR: invalid file handler\n");
        return -1;
    }

    for (int i = 0; i < num_jobs; i++) {
        if (NULL == job_pkt_src_bases[i]) {
            VERBOSE0 ("ERROR: Invalid pointer in job_pkt_src_bases[%d]\n", i);
            return -1;
        }

        job_sizes[i] = 0;
    }

    if (0 == (pkt_count / num_jobs)) {
        VERBOSE0 ("ERROR: at least %d packet(s) are needed, get %d for now!\n", num_jobs, pkt_count);
        return -1;
    }

    void * pkt_src = job_pkt_src_bases[0];

    VERBOSE0 ("PACKET Source Address Start at 0X%016lX\n", (uint64_t)pkt_src);

    while ((read = getline (&line, &len, fp)) != -1) {
        if (curr_job_id != num_jobs - 1 &&
            lines_read == (curr_job_id + 1) * (pkt_count / num_jobs)) {
            job_sizes[curr_job_id] = (unsigned char *) pkt_src - (unsigned char *) job_pkt_src_bases[curr_job_id];
            curr_job_id++;
            pkt_src = job_pkt_src_bases[curr_job_id];
        }

        remove_newline (line);
        read--;
        VERBOSE3 ("PACKET line read with length %zu :\n", read);
        VERBOSE3 ("%s\n", line);

        if (read >= PACKET_MAX_SIZE) {
            VERBOSE0 ("ERROR: packet[%d] length (%zu) exceeds the limit (%d)", lines_read + 1, read, PACKET_MAX_SIZE);
            return -1;
        }

        pkt_src = fill_one_packet (line, read, pkt_src, lines_read + 1);
        // regex ref model
        // regex_ref_push_packet (line);
        VERBOSE3 ("PACKET Source Address 0X%016lX\n", (uint64_t)pkt_src);

        lines_read++;
    }

    job_sizes[curr_job_id] = (unsigned char *)pkt_src - (unsigned char *) job_pkt_src_bases[curr_job_id];

    if (line) {
        free (line);
    }

    return 0;
}

int process_results (void * result_dest_base, bool check, int * processed_results, FILE * out_fp)
{
    int i = 0, j = 0, err = 0;
    uint16_t offset = 0;
    uint32_t pkt_id = 0;
    uint32_t patt_id = 0;

    if (NULL == out_fp) {
        VERBOSE0 ("ERROR: invalid output file handler!\n");
        return -1;
    }

    if (NULL == result_dest_base) {
        VERBOSE0 ("ERROR: invalid result buffer!\n");
        return -1;
    }

    VERBOSE1 ("---- Result buffer address: %p ----\n", result_dest_base);
    VERBOSE1 ("PKT(HW) PATT(HW) OFFSET(HW)\n");
    fprintf (out_fp, "Packet_ID,Regex_ID,Offset\n");

    do {
        for (j = 0; j < 2; j++) {
            offset |= (((uint8_t *)result_dest_base)[i * 8 + j] << (j % 2) * 8);
        }

        for (j = 2; j < 4; j++) {
            patt_id |= (((uint8_t *)result_dest_base)[i * 8 + j] << (j % 2) * 8);
        }

        for (j = 4; j < 8; j++) {
            pkt_id |= (((uint8_t *)result_dest_base)[i * 8 + j] << (j % 4) * 8);
        }

        if (patt_id == -1 || pkt_id == -1 || patt_id == 0 || pkt_id == 0) {
            patt_id = 0;
            pkt_id = 0;
            offset = 0;
            break;
        }


        if (check) {
            err += compare_result_id (pkt_id, patt_id, offset);
        } else {
            VERBOSE1 ("%7d\t%6d\t%7d\n", pkt_id, patt_id, offset);
        }

        fprintf (out_fp, "%7d,%6d,%7d\n", pkt_id, patt_id, offset);

        patt_id = 0;
        pkt_id = 0;
        offset = 0;

        i++;
    } while (1);

    VERBOSE1 ("%d processed results\n", i);
    VERBOSE1 ("%d error results\n", err);

    *processed_results = i;

    return err;
}

int compare_num_matched_pkt (size_t num_matched_pkt)
{
    int rc = 0;

    if ((int)num_matched_pkt != regex_ref_get_num_matched_pkt()) {
        VERBOSE0 ("ERROR! Num matched packets mismatch\n");
        VERBOSE0 ("EXPECTED: %d\n", regex_ref_get_num_matched_pkt());
        VERBOSE0 ("ACTUAL: %d\n", (int)num_matched_pkt);
        rc = 1;
    }

    return rc;
}

int compare_result_id (uint32_t pkt_id, uint32_t patt_id, uint64_t offset)
{
    int rc = regex_ref_compare_result (pkt_id, patt_id, offset);

    if (rc != 0) {
        VERBOSE0 ("PKT(HW): %7d -- PATT(HW): %7d -- OFFSET(HW): %7d -- MISMATCHED\n",
                  pkt_id, patt_id, offset);
    }

    return rc;
}
