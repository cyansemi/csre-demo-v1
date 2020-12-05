create_ip -name ila -vendor xilinx.com -library ip -version 6.2 -module_name ila_0
set_property -dict [list CONFIG.C_NUM_OF_PROBES {30} \
                         CONFIG.C_PROBE9_WIDTH {64} \
                         CONFIG.C_PROBE16_WIDTH {64} \
                         CONFIG.C_PROBE23_WIDTH {64} \
                         CONFIG.C_PROBE10_WIDTH {8} \
                         CONFIG.C_PROBE17_WIDTH {8} \
                         CONFIG.C_PROBE24_WIDTH {8} \
                         CONFIG.C_PROBE13_TYPE {DATA}  \
                         CONFIG.C_PROBE20_TYPE {DATA}  \
                         CONFIG.C_PROBE27_TYPE {DATA}  \
                         CONFIG.C_PROBE13_WIDTH {128} \
                         CONFIG.C_PROBE20_WIDTH {128} \
                         CONFIG.C_PROBE27_WIDTH {128} \
                         CONFIG.C_EN_STRG_QUAL {1} \
                         CONFIG.C_INPUT_PIPE_STAGES {2} \
                         CONFIG.C_ADV_TRIGGER {true} \
                         CONFIG.ALL_PROBE_SAME_MU_CNT {4} \
                   ] [get_ips ila_0]
generate_target {instantiation_template} [get_files ila_0.xci]
set_property generate_synth_checkpoint false [get_files ila_0.xci]
