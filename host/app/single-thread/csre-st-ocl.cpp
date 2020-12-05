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

#include "xcl2/xcl2.hpp"
#include "ref/regex_ref.h"
#include "api/libcsre.h"
#include <getopt.h>
#include <streambuf>
#include <vector>

void usage (const char * prog)
{
    printf ("CSRE Regular Expression Test.\n"
            "    Use Option -p and -q for packet file and regex pattern\n"
            "    e.g. %s -k <xclbin file> -p <packet file> -r <regex pattern> -o <result_file> [-vv] [-d] [-n]\n",
            prog);
    printf ("Usage: %s\n"
            "    -h, --help           Print usage information\n"
            "    -p, --packet         Packet file for matching\n"
            "    -q, --regex          Regular expression for matching\n"
            "    -o, --result_file    File name to hold the regular expression matching result\n"
            "    -k, --kernel         Path to XCLBIN file\n"
            "    -d, --disable_check  Disable check with software\n"
            "    -v, --verbose        Verbose mode for more message\n"
            , prog);
}

int main (int argc, char ** argv)
{
    char packet_file[256] = "./packet.txt";
    char regex_file[256] = "./regex.txt";
    char result_file[256] = "./result.txt";
    char binaryFile[256] = "./csre.xclbin";
    cl_int err;
    int cmd;
    bool check = true;

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            { "help",          no_argument,       NULL, 'h' },
            { "packet",        required_argument, NULL, 'p' },
            { "regex_file",    required_argument, NULL, 'q' },
            { "result_file",   required_argument, NULL, 'o' },
            { "kernel",        required_argument, NULL, 'k' },
            { "disable_check", required_argument, NULL, 'd' },
            { "verbose",       required_argument, NULL, 'v' },
            { 0,               no_argument,       NULL, 0   },
        };
        cmd = getopt_long (argc, argv, "t:p:q:o:k:hdv",
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

        case 'o':
            sprintf (result_file, "%s", optarg);
            break;

        case 'k':
            sprintf (binaryFile, "%s", optarg);
            break;

        case 'v':
            verbose_level++;
            break;

        case 'd':
            check = false;
            break;

        default:
            usage (argv[0]);
            exit (EXIT_FAILURE);
        }
    }

    printf ("--> XCLBIN file: %s\n", binaryFile);

    printf ("--------> CyanSemi Regular Expression Acceleration OpenCL Demo\n");
    printf ("----> Initialize Hardware\n");
    int number_of_kernels = 0;
    OCL_CHECK (err, err = csre_init_hardware (binaryFile, &number_of_kernels));

    printf ("----> Compile regular expression pattern\n");
    printf ("--> Get regex from file: %s\n", regex_file);
    // Compile the regex and fill them to the user buffer
    size_t regex_buffers_size = 0;
    std::vector<void *> regex_buffers (number_of_kernels);

    for (int i = 0; i < number_of_kernels; i++) {
        OCL_CHECK (err, err = csre_compile_file (regex_file, &regex_buffers[i], &regex_buffers_size));
        printf ("--> Job[%d]: Regex buffer size: %zu @ %p\n", i, regex_buffers_size, regex_buffers[i]);
    }

    printf ("----> Parse packet file\n");
    printf ("--> Get packet from file: %s\n", packet_file);
    // Parse the packet file and fill them to the user buffer
    std::vector<void *> job_packet_buffers (number_of_kernels);
    std::vector<size_t> job_packet_sizes (number_of_kernels);
    OCL_CHECK (err, err = csre_parse_packet (packet_file, job_packet_buffers.data(), job_packet_sizes.data(), number_of_kernels));

    // Create Buffer Object for result based on the user pointer
    printf ("----> Prepare result buffer\n");
    std::vector<void *> result_buffers (number_of_kernels);

    size_t result_size = csre_align (OUTPUT_RESULT_WIDTH_AXI) * MAX_PACKET_COUNT; // TODO: remove hardcode threshold

    for (int i = 0; i < number_of_kernels; i++) {
        result_buffers[i] = alloc_mem (64, result_size);
    }

    // Run the software to get the golden reference
    if (check) {
        printf ("----> Run software for reference result\n");
        regex_ref_push_packet (packet_file);
        regex_ref_push_pattern (regex_file);
        regex_ref_run_match();
    }

    printf ("----> Scan on FPGA\n");
    // Call FPGA to scan packets
    OCL_CHECK (err, err = csre_scan (regex_buffers.data(), regex_buffers_size,
                                     job_packet_buffers.data(), job_packet_sizes.data(),
                                     result_buffers.data(), result_size));

    printf ("----> Harvest result\n");
    // Harvest the result
    OCL_CHECK (err, err = csre_result (result_buffers.data(), result_file, check));

    // Cleanup the hardware
    OCL_CHECK (err, err = csre_close_hardware());

    // Cleanup the buffers
    for (int i = 0; i < number_of_kernels; i++) {
        free (regex_buffers[i]);
        free (job_packet_buffers[i]);
        free (result_buffers[i]);
    }

    if (err) {
        std::cout << "TEST FAILED" << std::endl;
        return err;
    }

    std::cout << "TEST PASSED!" << std::endl;
    return 0;
}
