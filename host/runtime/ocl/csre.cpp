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

#include "xcl2/xcl2.hpp"
#include "ref/regex_ref.h"
#include "recomp/regex_config.h"
#include "utils/xclbin_utils.h"
#include "api/libcsre.h"

#include "boost/chrono.hpp"

using namespace boost::chrono;

class CSRE
{
public:
    CSRE() :
        m_number_of_kernels (0),
        m_regex_buffer_size (0),
        m_packet_buffer_size (0),
        m_result_buffer_size (0)
    {};

    ~CSRE() {};

    int init_hardware (const char * binaryFile, int * number_of_kernels)
    {
        cl_int err;
        //OPENCL HOST CODE AREA START
        //Create Program and Kernel
        auto devices = xcl::get_xil_devices();
        auto device = devices[0];

        // read_binary_file() is a utility API which will load the binaryFile
        // and will return the pointer to file buffer.
        auto fileBuf = xcl::read_binary_file (std::string (binaryFile));
        cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
        int valid_device = 0;

        csre_config_t csre_config;
        csre_config.CSRE_MAX_CHAR_NUM       = xclbinGetKeyValue (binaryFile, "CSRE_MAX_CHAR_NUM");
        csre_config.CSRE_MAX_STATE_NUM      = xclbinGetKeyValue (binaryFile, "CSRE_MAX_STATE_NUM");
        csre_config.CSRE_MAX_CHAR_PER_TOKEN = xclbinGetKeyValue (binaryFile, "CSRE_MAX_CHAR_PER_TOKEN");
        csre_config.CSRE_MAX_TOKEN_NUM      = xclbinGetKeyValue (binaryFile, "CSRE_MAX_TOKEN_NUM");
        csre_config.CSRE_MAX_COUNT_NUM      = xclbinGetKeyValue (binaryFile, "CSRE_MAX_COUNT_NUM");

        if (update_csre_regex_configs (csre_config)) {
            std::cout << "Failed to update csre configuration!\n";
            exit (EXIT_FAILURE);
        }

        for (unsigned int i = 0; i < devices.size(); i++) {
            auto device = devices[i];
            // Creating Context and Command Queue for selected Device
            OCL_CHECK (err, m_context = cl::Context (device, NULL, NULL, NULL, &err));
            OCL_CHECK (err,
                       m_queue = cl::CommandQueue (
                                     m_context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err));

            std::cout << "--> Trying to program device[" << i
                      << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
            cl::Program program (m_context, {device}, bins, NULL, &err);

            m_number_of_kernels = program.getInfo<CL_PROGRAM_NUM_KERNELS> (&err);
            m_kernels.resize (m_number_of_kernels);
            std::cout << "--> Number kernels: " << std::dec << m_number_of_kernels << std::endl;

            if (err != CL_SUCCESS) {
                std::cout << "Failed to program device[" << i
                          << "] with xclbin file!\n";
            } else {
                std::cout << "--> Device[" << i << "]: program successful!\n";

                for (int i = 0; i < m_number_of_kernels; i++) {
                    // Kernel ID starts from 1, e.g., csre_1, csre_2, etc.
                    std::string krnl_name = "csre_" + std::to_string (i + 1);
                    printf ("--> Job[%d]: Prepare OCL context for kernel %s\n", i, krnl_name.c_str());

                    OCL_CHECK (err,
                               m_kernels[i] =
                                   cl::Kernel (program, krnl_name.c_str(), &err));
                }

                valid_device++;
                break; // we break because we found a valid device
            }

        }

        if (valid_device == 0) {
            std::cout << "Failed to program any device found, exit!\n";
            exit (EXIT_FAILURE);
        }

        if (m_number_of_kernels == 0) {
            std::cout << "Failed to find kernels, exit!\n";
            exit (EXIT_FAILURE);
        }

        *number_of_kernels = m_number_of_kernels;

        return 0;
    }

    int scan (void ** regex_buffers, size_t regex_buffer_size, void ** job_packet_buffers, size_t * job_packet_sizes,
              void ** result_buffers, size_t result_buffer_size)
    {
        cl_int err;

        // TODO: how to make sure the size of the input arrays are within the boundary of m_number_of_kernels

        m_regex_buffer_size = regex_buffer_size;
        m_result_buffer_size = result_buffer_size;
        m_packet_buffer_size = 0;

        for (int i = 0; i < m_number_of_kernels; i++) {
            printf ("--> Job[%d]: packet buffer size: %zu @ %p\n", i, job_packet_sizes[i], job_packet_buffers[i]);
            m_packet_buffer_size += job_packet_sizes[i];
        }

        std::vector<cl_mem_ext_ptr_t> ext_regex_buffers (m_number_of_kernels);
        std::vector<cl_mem_ext_ptr_t> ext_packet_buffers (m_number_of_kernels);
        std::vector<cl_mem_ext_ptr_t> ext_result_buffers (m_number_of_kernels);

        std::vector<cl::Buffer> ocl_buf_regex (m_number_of_kernels);
        std::vector<cl::Buffer> ocl_buf_packet (m_number_of_kernels);
        std::vector<cl::Buffer> ocl_buf_result (m_number_of_kernels);

        int step = (m_number_of_kernels <= 2 ? 16 : 
                    m_number_of_kernels <= 4 ? 8 :
                    m_number_of_kernels <= 8 ? 4 :
                    m_number_of_kernels <= 16? 2 : 999); //Doesn't support more than 16 kernels.

        for (int i = 0; i < m_number_of_kernels; i++) {
            // Set the HBM index
            ext_packet_buffers[i] = {XCL_MEM_TOPOLOGY | unsigned (i * step), job_packet_buffers[i], 0};
            ext_regex_buffers[i]  = {XCL_MEM_TOPOLOGY | unsigned (i * step + 1), regex_buffers[i], 0};
            ext_result_buffers[i] = {XCL_MEM_TOPOLOGY | unsigned (i * step + 1), result_buffers[i], 0};

            //Allocate Buffer in Global Memory
            OCL_CHECK (err,
                       ocl_buf_regex[i] = cl::Buffer (m_context,
                                          CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                          regex_buffer_size,
                                          &ext_regex_buffers[i],
                                          &err));

            OCL_CHECK (err,
                       ocl_buf_packet[i] = cl::Buffer (m_context,
                                           CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                           job_packet_sizes[i],
                                           &ext_packet_buffers[i],
                                           &err));
            OCL_CHECK (err,
                       ocl_buf_result[i] = cl::Buffer (m_context,
                                           CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                           result_buffer_size,
                                           &ext_result_buffers[i],
                                           &err));

            OCL_CHECK (err, err = m_kernels[i].setArg (0, ocl_buf_packet[i]));
            OCL_CHECK (err, err = m_kernels[i].setArg (1, ocl_buf_regex[i]));
            OCL_CHECK (err, err = m_kernels[i].setArg (2, ocl_buf_result[i]));
            OCL_CHECK (err, err = m_kernels[i].setArg (3, (ulong)job_packet_sizes[i]));
            OCL_CHECK (err, err = m_kernels[i].setArg (4, (uint)regex_buffer_size));
            OCL_CHECK (err, err = m_kernels[i].setArg (5, (ulong)result_buffer_size));
        }

        std::vector<cl::Event> data_from_host_to_device (m_number_of_kernels);
        std::vector<std::vector<cl::Event> > kernel_exec_dep_list (m_number_of_kernels);
        std::vector<cl::Event> kernel_exec (m_number_of_kernels);
        std::vector<std::vector<cl::Event> > result_dep_list (m_number_of_kernels);

        t_start = high_resolution_clock::now();

        for (int i = 0; i < m_number_of_kernels; i++) {

            OCL_CHECK (err,
                       err = m_queue.enqueueMigrateMemObjects ({ocl_buf_regex[i], ocl_buf_packet[i]},
                               //0));
                               0, NULL, &data_from_host_to_device[i]));

            kernel_exec_dep_list[i].push_back (data_from_host_to_device[i]);
        }

        OCL_CHECK (err, err = m_queue.flush());
        OCL_CHECK (err, err = m_queue.finish());

        t_copy_end = high_resolution_clock::now();

        for (int i = 0; i < m_number_of_kernels; i++) {

            OCL_CHECK (err, err = m_queue.enqueueTask (m_kernels[i], &kernel_exec_dep_list[i], &kernel_exec[i]));

            result_dep_list[i].push_back (kernel_exec[i]);
        }

        OCL_CHECK (err, err = m_queue.flush());
        OCL_CHECK (err, err = m_queue.finish());

        t_exec_end = high_resolution_clock::now();

        for (int i = 0; i < m_number_of_kernels; i++) {
            OCL_CHECK (err,
                       err = m_queue.enqueueMigrateMemObjects ({ocl_buf_result[i]},
                               CL_MIGRATE_MEM_OBJECT_HOST, &result_dep_list[i], NULL));
        }

        OCL_CHECK (err, err = m_queue.flush());
        OCL_CHECK (err, err = m_queue.finish());

        t_result_end = high_resolution_clock::now();
        return err;
    }

    void print_perf()
    {
        auto copy_duration = duration_cast<microseconds> (t_copy_end - t_start).count();
        auto exec_duration = duration_cast<microseconds> (t_exec_end - t_copy_end).count();
        auto result_duration = duration_cast<microseconds> (t_result_end - t_exec_end).count();
        auto overall_duration = duration_cast<microseconds> (t_result_end - t_start).count();

        printf ("----> [PERF] Copy time and perf:");
        print_time (copy_duration, (m_packet_buffer_size + (m_regex_buffer_size * m_number_of_kernels)));
        printf ("----> [PERF] Exec time and perf:");
        print_time (exec_duration, m_packet_buffer_size);
        printf ("----> [PERF] Result time and perf:");
        print_time (result_duration, m_result_buffer_size);
        printf ("----> [PERF] Overall time and perf:");
        print_time (overall_duration, m_packet_buffer_size);
    }

    int get_number_of_kernels()
    {
        return m_number_of_kernels;
    }

private:
    // OpenCL operation data structure
    cl::CommandQueue m_queue;
    cl::Context m_context;
    std::vector<cl::Kernel> m_kernels;

    // XCLBIN information
    int m_number_of_kernels;
    int m_max_char_num;
    int m_max_state_num;
    int m_max_token_len;
    int m_max_token_num;
    int m_packet_pipeline;
    int m_pattern_pipeline;

    // Runtime buffer size
    uint64_t m_regex_buffer_size;
    uint64_t m_packet_buffer_size;
    uint64_t m_result_buffer_size;

    // Performance counters
    high_resolution_clock::time_point t_start;
    high_resolution_clock::time_point t_copy_end;
    high_resolution_clock::time_point t_exec_end;
    high_resolution_clock::time_point t_result_end;
};

static CSRE * global_csre = NULL;

int csre_init_hardware (const char * binaryFile, int * number_of_kernels)
{
    global_csre = new CSRE();
    return global_csre->init_hardware (binaryFile, number_of_kernels);
}

int csre_scan (void ** regex_buffers, size_t regex_buffer_size, void ** job_packet_buffers, size_t * job_packet_sizes,
               void ** result_buffers, size_t result_buffer_size)
{
    return global_csre->scan (regex_buffers, regex_buffer_size, job_packet_buffers, job_packet_sizes,
                              result_buffers, result_buffer_size);
}

int csre_result (void ** result_buffers, const char * result_file, bool check)
{
    cl_int err = 0;
    // Print the performance of scan
    global_csre->print_perf();

    int total_processed_results = 0;
    FILE * out_fp = fopen (result_file, "w");

    printf ("----> Get the result\n");

    for (int i = 0; i < global_csre->get_number_of_kernels(); i++) {
        if (verbose_level > 0) {
            __hexdump (stdout, (void *) result_buffers[i], 128);
        }

        int processed_results = 0;
        int failed_results = process_results (result_buffers[i], check, &processed_results, out_fp);
        err += failed_results;
        total_processed_results += processed_results;
    }

    printf ("--> Totally %d failed results\n", err);

    fclose (out_fp);

    if (check) {
        if (compare_num_matched_pkt (total_processed_results)) {
            err += 1;
        }
    }

    printf ("--> Totally %d processed results\n", total_processed_results);
    printf ("--> Check the result in %s\n", result_file);

    return err;
}

int csre_close_hardware()
{
    if (global_csre) {
        delete global_csre;
    }

    return 0;
}
