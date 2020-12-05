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

#include <getopt.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include "libcsre-comp.h"

extern int verbose_level;

void usage (const char * prog)
{
    printf ("CSRE Regular Expression Compile Example.\n"
            "    Use Option -p and -q for packet file and regex pattern\n"
            "    e.g. %s -p <packet file> -q <regex pattern> [-v]\n",
            prog);
    printf ("Usage: %s\n"
            "    -h, --help           Print usage information\n"
            "    -p, --packet         Packet file for matching\n"
            "    -q, --regex          Regular expression for matching\n"
            "    -v, --verbose        Verbose mode for more message\n"
            , prog);
}

int main (int argc, char ** argv)
{
    int num_jobs = 1;
    char packet_file[256] = "./packet.txt";
    char regex_file[256] = "./regex.txt";
    int err = 0;
    int cmd;

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            { "help",         no_argument,       NULL, 'h' },
            { "packet",       required_argument, NULL, 'p' },
            { "regex_file",   required_argument, NULL, 'q' },
            { "verbose",      required_argument, NULL, 'v' },
            { 0,              no_argument,       NULL, 0   },
        };
        cmd = getopt_long (argc, argv, "p:q:hv",
                           long_options, &option_index);

        if (cmd == -1) {  /* all params processed ? */
            break;
        }

        switch (cmd) {

        case 'h':   /* help */
            usage (argv[0]);
            exit (EXIT_SUCCESS);;

        case 'p':
            sprintf (packet_file, "%s", optarg);
            break;

        case 'q':
            sprintf (regex_file, "%s", optarg);
            break;

        case 'v':
            verbose_level++;
            break;

        default:
            usage (argv[0]);
            exit (EXIT_FAILURE);
        }
    }

    printf ("--------> CyanSemi Regular Expression Compile Example\n");

    printf ("----> Compile regular expression pattern\n");
    printf ("--> Get regex from file: %s\n", regex_file);
    // Compile the regex and fill them to the user buffer
    size_t regex_buffer_size = 0;
    void * regex_buffer = NULL;
    err = csre_compile_file (regex_file, &regex_buffer, &regex_buffer_size);

    if (err) {
        printf ("ERROR: failed to process regex file: %s!\n", regex_file);
        return -1;
    }

    printf ("--> Regex buffer size: %zu @ %p\n", regex_buffer_size, regex_buffer);

    if (verbose_level > 0) {
        __hexdump (stdout, (void*) regex_buffer, regex_buffer_size);
    }

    printf ("----> Parse packet file\n");
    printf ("--> Get packet from file: %s\n", packet_file);
    // Parse the packet file and fill them to the user buffer
    void ** job_packet_buffer = (void **) malloc (num_jobs * sizeof (void *));
    size_t * job_packet_sizes = (size_t *) malloc (num_jobs * sizeof (size_t));
    err = csre_parse_packet (packet_file, job_packet_buffer, job_packet_sizes, num_jobs);

    if (err) {
        printf ("ERROR: failed to process packet file: %s!\n", packet_file);
        return -1;
    }

    uint64_t packet_buffer_size = 0;

    // Create Buffer Object for packet based on the user pointer
    for (int i = 0; i < num_jobs; i++) {
        printf ("--> Job[%d] packet buffer size: %zu @ %p\n", i, job_packet_sizes[i], job_packet_buffer[i]);
        packet_buffer_size += job_packet_sizes[i];

        if (verbose_level > 0) {
            __hexdump (stdout, (void*) job_packet_buffer[i], job_packet_sizes[i]);
        }
    }

    for (int i = 0; i < num_jobs; i++) {
        free_mem (job_packet_buffer[i]);
    }

    free_mem (regex_buffer);
    free_mem (job_packet_buffer);
    free_mem (job_packet_sizes);

    return err;

}
