#Script to implement (opt, place, route) FPGA configurations of RMs in RPs
#One configuration per RM will be made with every RP filled by the RM the configuration is associated with


#Get RP list

open_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/top_rcf_pblocked.dcp
set rps [get_cells -hierarchical ou]

#Implement sra RM(largest) first (also to generate a static routed locked design)
opt_design
place_design
route_design

write_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/full_configs/sra_routed.dcp -force

foreach rp $rps {
	scan $rp "design_1_i/taiga_wrapper_xilinx_0/inst/cpu/rca/pr_grid/pr_slots_row\[%u\].pr_slots_col\[%u\].grid_slot/ou" row col
	set file_name [format "/home/anv17/FYP/fyp-rca/rca/dcps/slot_dcps/rp_%u_%u/sra_routed.dcp" $row $col]
        write_checkpoint -cell $rp $file_name -force
}

foreach rp $rps {
	update_design -cell $rp -black_box
}

lock_design -level routing

write_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/static_routed.dcp -force
close_design
#Static Routed Design Generated - now substitute each RM into static design and implement

set rm_dcps [glob /home/anv17/FYP/fyp-rca/rca/dcps/rm_ooc_synth_dcps/*.dcp]
#form module names

foreach rm $rm_dcps {
  puts "Implementing $rm"
  open_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/static_routed.dcp
  set rps [get_cells -hierarchical ou]

	foreach rp $rps {
		read_checkpoint -cell $rp $rm
	}

	opt_design
	place_design
	route_design

	set rm_name [string map {"_ou_synth.dcp" ""} [scan $rm "/home/anv17/FYP/fyp-rca/rca/dcps/rm_ooc_synth_dcps/%s"]]
	write_checkpoint [format "/home/anv17/FYP/fyp-rca/rca/dcps/full_configs/%s_routed.dcp" $rm_name] -force

	foreach rp $rps {
		scan $rp "design_1_i/taiga_wrapper_xilinx_0/inst/cpu/rca/pr_grid/pr_slots_row\[%u\].pr_slots_col\[%u\].grid_slot/ou" row col
		set file_name [format "/home/anv17/FYP/fyp-rca/rca/dcps/slot_dcps/rp_%u_%u/%s_routed.dcp" $row $col $rm_name ]
	     	write_checkpoint -cell $rp $file_name -force
	}

#	foreach rp $rps {
#		update_design -cell $rp -black_box
#	}
	close_design
#}	


#Finally a greybox module
puts "Implementing Greybox modules"

open_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/static_routed.dcp
set rps [get_cells -hierarchical ou]
	foreach rp $rps {
		update_design -cell $rp -buffer_ports
	}

place_design
route_design

write_checkpoint -force  /home/anv17/FYP/fyp-rca/rca/dcps/greybox_routed.dcp
close_design

