
if { $::argc != 6 } {
    puts "ERROR: Program \"$::argv0\" requires 5 arguments!\n"
    puts "Usage: $::argv0 <xoname> <krnl_name> <target> <xpfm_path> <device> <krnl_id>\n"
    exit
}

set xoname    [lindex $::argv 0]
set krnl_name [lindex $::argv 1]
set target    [lindex $::argv 2]
set xpfm_path [lindex $::argv 3]
set device    [lindex $::argv 4]
set krnl_id   [lindex $::argv 5]

set suffix "${krnl_name}_${target}_${device}_${krnl_id}"

puts "suffix: $suffix"

source -notrace ./scripts/package_kernel.tcl

if {[file exists "${xoname}"]} {
    file delete -force "${xoname}"
}

package_xo -xo_path ${xoname} -kernel_name ${krnl_name} -ip_directory ./packaged_kernel/ip_${suffix} -kernel_xml ./src/kernel_${krnl_id}.xml
