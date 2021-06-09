#script to implement greybox configuration and one other RM
source /home/anv17/FYP/fyp-rca/rca/scripts/make_rcf.tcl
source /home/anv17/FYP/fyp-rca/rca/scripts/apply_pblocks_and_biggest_rm.tcl

set ooc_rm_dcp /home/anv17/FYP/fyp-rca/rca/dcps/rm_ooc_synth_dcps/add_ou_synth.dcp
set routed_rm_dcp /home/anv17/FYP/fyp-rca/rca/dcps/full_configs/add_routed.dcp

open_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/top_rcf_pblocked.dcp
set rps [get_cells -hierarchical ou]

#Implement sra RM(largest) first (also to generate a static routed locked design)
puts "Implementing SRA and static routed locked design"
opt_design
place_design
route_design

write_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/full_configs/sra_routed.dcp -force

foreach rp $rps {
	update_design -cell $rp -black_box
}

lock_design -level routing

write_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/static_routed.dcp -force

puts "Implementing User defined RM"
#Apply User defined RM 
foreach rp $rps {
	read_checkpoint -cell $rp $ooc_rm_dcp
}

#Implement Configuration

opt_design
place_design
route_design

write_checkpoint $routed_rm_dcp -force
close_design

#Finally a greybox module
puts "Implementing Greybox Config"

open_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/static_routed.dcp
set rps [get_cells -hierarchical ou]
	foreach rp $rps {
		update_design -cell $rp -buffer_ports
	}


place_design
route_design

write_checkpoint -force  /home/anv17/FYP/fyp-rca/rca/dcps/greybox_routed.dcp
close_design

puts "Running PR Verify"
pr_verify /home/anv17/FYP/fyp-rca/rca/dcps/greybox_routed.dcp $routed_rm_dcp

#Generate bitstream for greybox and RM configuration
puts "Generating bitstreams"
#RM Configuration bitstream
set bitstream_dir /home/anv17/FYP/fyp-rca/rca/bitstreams/
open_checkpoint $routed_rm_dcp
set bs_name [string map {"_routed.dcp" ""} [scan $routed_rm_dcp "/home/anv17/FYP/fyp-rca/rca/dcps/full_configs/%s"]]
set bs_path $bitstream_dir$bs_name
write_bitstream $bs_path -force
close_design

#Generate Greybox bitstreams
open_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/greybox_routed.dcp
set bs_name greybox
set bs_path $bitstream_dir$bs_name
write_bitstream $bs_path -force -no_partial_bitfile
close_design









