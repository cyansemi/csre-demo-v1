#!/usr/bin/env python
# -*- coding: utf-8 -*-



import sys
import os
import time
import random
import subprocess
import shlex
from optparse import OptionParser
from os.path import join as pathjoin

usage =\
'''
%prog <options>
[Implementation strategies list]
Performance_Explore Performance_ExplorePostRoutePhysOpt Performance_WLBlockPlacement Performance_WLBlockPlacementFanoutOpt Performance_EarlyBlockPlacement
Performance_NetDelay_high Performance_NetDelay_low Performance_Retiming Performance_ExtraTimingOpt Performance_RefinePlacement Performance_SpreadSLLs
Performance_BalanceSLLs Congestion_SpreadLogic_high Congestion_SpreadLogic_medium Congestion_SpreadLogic_low Congestion_SpreadLogic_Explore
Congestion_SSI_SpreadLogic_high Congestion_SSI_SpreadLogic_low Area_Explore Area_ExploreSequential Area_ExploreWithRemap Power_DefaultOpt Power_ExploreArea
'''

parser = OptionParser(usage=usage)
parser.add_option("-d", "--dry_run",
                  action="store_true", dest="dry_run", default=False,
                  help="Only generate the run scripts, no job submission, default: %default")
parser.add_option("-i", "--input_repo", dest="input_repo", default='../../',
                  help="Path to csre-demo root. No default value.", metavar="DIRECTORY")
parser.add_option("-p", "--pattern_buf_l1", dest="pattern_buf_l1", default="4",
                  help="Common separated list for number of pattern buffers, default: %default", metavar="STRING")
parser.add_option("-k", "--number_of_kernel", dest="number_of_kernel", default="2,4",
                  help="Common separated list for number of kernels, default: %default", metavar="STRING")
parser.add_option("-f", "--clock_freq", dest="clock_freq", default="180,200,250",
                  help="Common separated list for clock freqs, default: %default", metavar="STRING")
parser.add_option("-c", "--max_char_num", dest="max_char_num", default="32,64",
                  help="Common separated list for max number of characters, default: %default", metavar="STRING")
parser.add_option("-s", "--max_state_num", dest="max_state_num", default="8,16,32",
                  help="Common separated list for max number of states, default: %default", metavar="STRING")
parser.add_option("-v", "--vivado_impl_strategy", dest="vivado_impl_strategy", default="Performance_Explore",
                  help="Common separated list for vivado implementation strategies, default: %default", metavar="STRING")
(options, leftovers) = parser.parse_args()

options.input_repo = os.path.abspath(options.input_repo)

run_script =\
'''#!/bin/bash
output_dir=%s
mkdir -p $output_dir
cd $output_dir
# Resolve the local tcl appstore issue when multiple vivado runs at the same time
export XILINX_TCLSTORE_USERAREA=$output_dir
touch .RUNNING
cmake -DCSRE_NUM_PATTERN_REG_L1="%d" -DCSRE_MAX_CHAR_NUM="%d" -DCSRE_MAX_STATE_NUM="%d" -DCSRE_MAX_TOKEN_NUM="%d" -DCSRE_MAX_CHAR_PER_TOKEN="%d" -DKERNEL_NUM="%d" -DKERNEL_FREQ="%d" -DVIVADO_IMPL_STRATEGY="%s" %s

make image
if [ $? != 0 ]; then
    touch .FAILED
else
    touch .PASSED
fi
rm .RUNNING

make install
'''

# 8 cpus and 32GB mem per run
run_cmd = "sbatch -p fpga_impl -n 8 --mem 32768 %s"
run_name_template = "k%d_f%d_char%d_state%d_pattbuf%d_%s_%s_%s"
run_dir = "."

def run_and_wait(cmd, work_dir, log):
    with open(log, "w+") as f:
        proc = subprocess.Popen(shlex.split(cmd),\
                                cwd=work_dir,\
                                shell=False,\
                                stdout=f, stderr=f)
        proc.communicate()
        return proc.returncode

def mkdirs(path):
    try:
        os.makedirs(path)
    except OSError as exc:
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise

for kernel_num in options.number_of_kernel.split(','):
    for freq in options.clock_freq.split(','):
        for patt_buf_l1  in options.pattern_buf_l1.split(','):
            for max_char in options.max_char_num.split(','):
                for max_state in options.max_state_num.split(','):
                    for strategy in options.vivado_impl_strategy.split(','):
                        timestamp = time.strftime("%Y%m%d-%a-%H%M%S", time.localtime())
                        seed = str(random.randint(0, 0xffffffff))
                        run_name = run_name_template % (int(kernel_num), int(freq), int(max_char), int(max_state), int(patt_buf_l1), timestamp, seed, strategy)
                        output_dir = pathjoin(run_dir, run_name)
                        mkdirs(output_dir)
                        run_file = pathjoin(output_dir, run_name + ".sh")
                        log_file = pathjoin(output_dir, run_name + ".log")
                        with open(run_file, "w") as f:
                            f.write(run_script % (output_dir, int(patt_buf_l1), int(max_char), int(max_state), 16, 16, int(kernel_num), int(freq), strategy, options.input_repo))
                        if not options.dry_run:
                            run_and_wait((run_cmd % run_file), ".", log_file)

