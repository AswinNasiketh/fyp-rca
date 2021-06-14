#script to do place and route on single RM config


set rm_dcp "/home/anv17/FYP/fyp-rca/rca/dcps/rm_ooc_synth_dcps/sra_ou_synth.dcp"

open_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/static_routed.dcp

set rps [get_cells -hierarchical ou]

foreach rp $rps {
	read_checkpoint -cell $rp $rm_dcp
}

opt_design
place_design
phys_opt_design
route_design

set rm_name [string map {"_ou_synth.dcp" ""} [scan $rm_dcp "/home/anv17/FYP/fyp-rca/rca/dcps/rm_ooc_synth_dcps/%s"]]
write_checkpoint [format "/home/anv17/FYP/fyp-rca/rca/dcps/full_configs/%s_routed.dcp" $rm_name] -force
