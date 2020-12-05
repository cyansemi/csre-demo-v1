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
#include <thread>
#include "ref/regex_ref.h"
#include "recomp/regex_config.h"
#include "core/constants.h"
#include "api/libcsre.h"
#include "utils/xrt_utils.h"
#include "experimental/xrt-next.h"
#include "boost/chrono.hpp"

using namespace boost::chrono;

void usage (const char * prog)
{
    printf ("CSRE Regular Expression Test.\n"
            "    Use Option -p and -r for packet file and regex pattern\n"
            "    e.g. %s -p <packet file> -r <regex pattern> -o <result_file> [-vv] [-s] [-n]\n",
            prog);
    printf ("Usage: %s\n"
            "    -h, --help           Print usage information\n"
            "    -p, --packet         Packet file for matching\n"
            "    -r, --regex          Regular expression for matching\n"
            "    -o, --result_file    File name to hold the regular expression matching result\n"
            "    -k, --kernel         Path to XCLBIN file\n"
            "    -j, --num_jobs       Number of jobs (threads)\n"
            "    -d, --disable_check  Disable check with software\n"
            "    -v, --verbose        Verbose mode for more message\n"
            , prog);
}

int main (int argc, char ** argv)
{
    int num_jobs = 1;
    char packet_file[256] = "./packet.txt";
    char regex_file[256] = "./regex.txt";
    char result_file[256] = "./result.txt";
    char binaryFile[256] = "./csre.xclbin";
    int err = 0;
    int cmd;
    bool check = true;

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            { "help",         no_argument,       NULL, 'h' },
            { "packet",       required_argument, NULL, 'p' },
            { "regex_file",   required_argument, NULL, 'q' },
            { "result_file",  required_argument, NULL, 'o' },
            { "kernel",       required_argument, NULL, 'k' },
            { "verbose",      required_argument, NULL, 'v' },
            { "num_jobs",     required_argument, NULL, 'j' },
            { "disable_check", required_argument, NULL, 'd' },
            { 0,              no_argument,       NULL, 0   },
        };
        cmd = getopt_long (argc, argv, "p:q:o:k:i:j:hdv",
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

        case 'j':
            num_jobs = strtol (optarg, (char **)NULL, 0);
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

    unsigned max_cu_index = num_jobs - 1;
    unsigned index = 0;
    std::string halLogfile;

    try {
        xclDeviceHandle handle;
        uint64_t cu_base_addr = 0;
        int first_mem = -1;
        uuid_t xclbinId;

        if (initXRT (binaryFile, index, halLogfile.c_str(), handle, max_cu_index, cu_base_addr, first_mem, xclbinId)) {
            return 1;
        }

        if (first_mem < 0) {
            return 1;
        }

        printf ("--------> CyanSemi Regular Expression Acceleration Demo\n");

        printf ("----> Compile regular expression pattern\n");
        printf ("--> Get regex from file: %s\n", regex_file);
        // Compile the regex and fill them to the user buffer
        size_t regex_buffer_size = 0;
        std::vector<void *> regex_buffer (num_jobs);

        for (int i = 0; i < num_jobs; i++) {
            err += csre_compile_file (regex_file, &regex_buffer[i], &regex_buffer_size);
            printf ("--> Regex buffer size: %zu @ %p\n", regex_buffer_size, regex_buffer[i]);
        }

        printf ("----> Parse packet file\n");
        printf ("--> Get packet from file: %s\n", packet_file);
        // Parse the packet file and fill them to the user buffer
        void ** job_packet_buffer = (void **) malloc (num_jobs * sizeof (void *));
        size_t * job_packet_sizes = (size_t *) malloc (num_jobs * sizeof (size_t));
        err += csre_parse_packet (packet_file, job_packet_buffer, job_packet_sizes, num_jobs);
        uint64_t packet_buffer_size = 0;

        // Create Buffer Object for packet based on the user pointer
        for (int i = 0; i < num_jobs; i++) {
            printf ("--> Job[%d] packet buffer size: %zu @ %p\n", i, job_packet_sizes[i], job_packet_buffer[i]);
            packet_buffer_size += job_packet_sizes[i];
        }

        // Create Buffer Object for result based on the user pointer
        printf ("----> Prepare result buffer\n");
        size_t result_size = csre_align (OUTPUT_RESULT_WIDTH_AXI) * MAX_PACKET_COUNT;

        std::vector<unsigned> bo_packet_handle (num_jobs);
        std::vector<unsigned> bo_regex_handle (num_jobs);
        std::vector<unsigned> bo_result_handle (num_jobs);
        std::vector<unsigned> bo_exec_handle (num_jobs);
        std::vector<void*> bo_exec_data (num_jobs);

        for (int i = 0; i < num_jobs; i++) {
            bo_regex_handle[i] = xclAllocUserPtrBO (handle, regex_buffer[i], regex_buffer_size, first_mem + (2 * i));
            bo_packet_handle[i] = xclAllocUserPtrBO (handle, job_packet_buffer[i], job_packet_sizes[i], first_mem + (2 * i));
            bo_result_handle[i] = xclAllocBO (handle, result_size, 0, first_mem + (2 * i + 1));
            bo_exec_handle[i] = xclAllocBO (handle, 4096, 0, (1 << 31));
            bo_exec_data[i] = xclMapBO (handle, bo_exec_handle[i], true);
        }

        // Run the software to get the golden reference
        if (check) {
            printf ("----> Run software for reference result\n");
            regex_ref_push_packet (packet_file);
            regex_ref_push_pattern (regex_file);
            regex_ref_run_match();
        }

        std::vector<std::thread> workers;
        std::vector<std::thread> copyers;
        std::vector<std::thread> resulters;

        printf ("----> Run FPGA\n");
        high_resolution_clock::time_point t_start = high_resolution_clock::now();

        for (int i = 0; i < num_jobs; i++) {
            copyers.push_back (std::thread ([&xclbinId, i, handle, bo_packet_handle, bo_regex_handle, bo_result_handle,
                        regex_buffer_size, job_packet_sizes, result_size, bo_exec_data]() -> int {

                if (xclOpenContext (handle, xclbinId, i, true)) {
                    throw std::runtime_error ("Cannot create context");
                }

                if (xclSyncBO (handle, bo_regex_handle[i], XCL_BO_SYNC_BO_TO_DEVICE, regex_buffer_size, 0)) {
                    printf ("ERROR: syncing regex buffer object\n");
                    return 1;
                }

                xclBOProperties p;
                uint64_t regex_bufferdevAddr = !xclGetBOProperties (handle, bo_regex_handle[i], &p) ? p.paddr : -1;

                if ((regex_bufferdevAddr == (uint64_t) (-1))) {
                    printf ("ERROR: invalid physical address for regex buffer\n");
                    return 1;
                }

                if (xclSyncBO (handle, bo_packet_handle[i], XCL_BO_SYNC_BO_TO_DEVICE, job_packet_sizes[i], 0)) {
                    printf ("ERROR: syncing packet buffer object on job %i\n", i);
                    return 1;
                }

                uint64_t packet_bufferdevAddr = !xclGetBOProperties (handle, bo_packet_handle[i], &p) ? p.paddr : -1;
                uint64_t result_bufferdevAddr = !xclGetBOProperties (handle, bo_result_handle[i], &p) ? p.paddr : -1;

                if ((result_bufferdevAddr == (uint64_t) (-1)) || (packet_bufferdevAddr == (uint64_t) (-1))) {
                    printf ("ERROR: invalid physical address for packet (%lx) and/or result (%lx) buffer on job %i\n",
                            packet_bufferdevAddr, result_bufferdevAddr, i);
                    return 1;
                }

                //construct the exec buffer cmd to start the kernel.
                {
                    auto ecmd = reinterpret_cast<ert_start_kernel_cmd *> (bo_exec_data[i]);
                    auto rsz = ADDR_RESULT_TOTAL_SIZE_H / 4 + 1; // regmap array size
                    std::memset (ecmd, 0, (sizeof * ecmd) + rsz);
                    ecmd->state = ERT_CMD_STATE_NEW;
                    ecmd->opcode = ERT_START_CU;
                    ecmd->count = 1 + rsz;
                    ecmd->cu_mask = (0x1 << i);

                    ecmd->data[ADDR_STD_CONTROL] = 0x0; // ap_start

                    ecmd->data[ADDR_PACKET_SRC_ADDR_L / 4] = packet_bufferdevAddr;
                    ecmd->data[ADDR_PACKET_SRC_ADDR_H / 4] = (packet_bufferdevAddr >> 32) & 0xFFFFFFFF; // input

                    ecmd->data[ADDR_REGEX_SRC_ADDR_L / 4] = regex_bufferdevAddr;
                    ecmd->data[ADDR_REGEX_SRC_ADDR_H / 4] = (regex_bufferdevAddr >> 32) & 0xFFFFFFFF; // input

                    ecmd->data[ADDR_RESULT_TGT_ADDR_L / 4] = result_bufferdevAddr;
                    ecmd->data[ADDR_RESULT_TGT_ADDR_H / 4] = (result_bufferdevAddr >> 32) & 0xFFFFFFFF; // output

                    ecmd->data[ADDR_PACKET_TOTAL_SIZE_L / 4] = job_packet_sizes[i];
                    ecmd->data[ADDR_PACKET_TOTAL_SIZE_H / 4] = (job_packet_sizes[i] >> 32) & 0xFFFFFFFF;

                    ecmd->data[ADDR_REGEX_TOTAL_SIZE / 4] = regex_buffer_size;

                    ecmd->data[ADDR_RESULT_TOTAL_SIZE_L / 4] = result_size;
                    ecmd->data[ADDR_RESULT_TOTAL_SIZE_H / 4] = (result_size >> 32) & 0xFFFFFFFF;
                }

                xclCloseContext (handle, xclbinId, i);

                return 0;
            }));
        }

        for (int i = 0; i < num_jobs; i++) {
            copyers[i].join();
        }
        high_resolution_clock::time_point t_copy_end = high_resolution_clock::now();
        auto copy_duration = duration_cast<microseconds> (t_copy_end - t_start).count();

        for (int i = 0; i < num_jobs; i++) {
            workers.push_back (std::thread ([&xclbinId, i, handle, bo_exec_handle]() -> int {

                if (xclOpenContext (handle, xclbinId, i, true)) {
                    throw std::runtime_error ("Cannot create context");
                }

                //Send the "start kernel" command.
                if (xclExecBuf (handle, bo_exec_handle[i])) {
                    printf ("ERROR: Unable to issue xclExecBuf[%d] : start_kernel\n", i);
                    printf ("FAILED TEST\n");
                    printf ("Write failed\n");
                    return 1;
                }

                //Wait on the command finish
                while (xclExecWait (handle, 1000) == 0) {
                    std::cout << "reentering wait...\n";
                };

                xclCloseContext (handle, xclbinId, i);

                return 0;
            }));
        }

        for (int i = 0; i < num_jobs; i++) {
            workers[i].join();
        }
        high_resolution_clock::time_point t_exec_end = high_resolution_clock::now();
        auto exec_duration = duration_cast<microseconds> (t_exec_end - t_copy_end).count();

        for (int i = 0; i < num_jobs; i++) {
            resulters.push_back (std::thread ([&xclbinId, i, handle, bo_result_handle, result_size]() -> int {

                if (xclOpenContext (handle, xclbinId, i, true)) {
                    throw std::runtime_error ("Cannot create context");
                }

                //Get the output;
                if (xclSyncBO (handle, bo_result_handle[i], XCL_BO_SYNC_BO_FROM_DEVICE, result_size, 0)) {
                    printf ("ERROR: syncing result buffer object on job %i\n", i);
                    return -1;
                }

                xclCloseContext (handle, xclbinId, i);

                return 0;
            }));
        }

        for (int i = 0; i < num_jobs; i++) {
            resulters[i].join();
        }
        high_resolution_clock::time_point t_result_end = high_resolution_clock::now();
        auto result_duration = duration_cast<microseconds> (t_result_end - t_exec_end).count();
        auto overall_duration = duration_cast<microseconds> (t_result_end - t_start).count();

        printf ("Copy time and perf:");
        print_time (copy_duration, (packet_buffer_size + (regex_buffer_size * num_jobs)));
        printf ("Exec time and perf:");
        print_time (exec_duration, packet_buffer_size);
        printf ("Result time and perf:");
        print_time (result_duration, result_size);
        printf ("Overall time and perf:");
        print_time (overall_duration, packet_buffer_size);

        int total_processed_results = 0;
        FILE * out_fp = fopen (result_file, "w");

        for (int i = 0; i < num_jobs; i++) {
            void * result_buffer = xclMapBO (handle, bo_result_handle[i], false);
            printf ("--> Job[%d] - Result buffer size: %zu @ %p\n", i, result_size, result_buffer);

            if (verbose_level > 0) {
                __hexdump (stdout, (void *) result_buffer, 256);
            }

            int processed_results = 0;
            err += process_results (result_buffer, check, &processed_results, out_fp);

            total_processed_results += processed_results;
            munmap (result_buffer, result_size);
        }

        fclose (out_fp);

        if (check) {
            err += compare_num_matched_pkt (total_processed_results);
        }

        //Clean up stuff
        for (int i = 0; i < num_jobs; i++) {
            munmap (bo_exec_data[i], 4096);
            xclFreeBO (handle, bo_packet_handle[i]);
            xclFreeBO (handle, bo_exec_handle[i]);
            xclFreeBO (handle, bo_regex_handle[i]);
            xclFreeBO (handle, bo_result_handle[i]);
            free_mem (job_packet_buffer[i]);
            free_mem (regex_buffer[i]);
        }

        free_mem (job_packet_buffer);
        free_mem (job_packet_sizes);
    } catch (std::exception const & e) {
        std::cout << "Exception: " << e.what() << "\n";
        std::cout << "FAILED TEST\n";
        return -1;
    }

    if (err) {
        std::cout << "TEST FAILED!" << std::endl;
        return err;
    } else {
        std::cout << "TEST PASSED!" << std::endl;
    }

    return err;

}
