#Script to implement (opt, place, route) FPGA configurations of RMs in RPs
#One configuration per RM will be made with every RP filled by the RM the configuration is associated with


#Get RP list

open_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/top_rcf.dcp
set rps [get_cells -hierarchical ou]

#Create PBlocks and add cells to PBlocks
foreach rp $rps {
	scan $rp "design_1_i/taiga_wrapper_xilinx_0/inst/cpu/rca/pr_grid/pr_slots_row\[%u\].pr_slots_col\[%u\].grid_slot/ou" row col
	set pblock_name [format "rp_%u_%u" $row $col]
	create_pblock $pblock_name
	add_cells_to_pblock [get_pblocks $pblock_name] $rp
	
        write_checkpoint -cell $rp $file_name
}

#Implement Passthrough RM first (also to generate a static routed locked design)

foreach rp $rps {
	read_checkpoint -cell $rp /home/anv17/FYP/fyp-rca/rca/dcps/rm_ooc_synth_dcps/pr_module_pt_synth.dcp

}

opt_design
place_design
route_design

write_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/full_configs/pt_routed.dcp


foreach rp $rps {
	scan $rp "design_1_i/taiga_wrapper_xilinx_0/inst/cpu/rca/pr_grid/pr_slots_row\[%u\].pr_slots_col\[%u\].grid_slot/ou" row col
	set file_name [format "/home/anv17/FYP/fyp-rca/rca/dcps/slot_dcps/rp_%u_%u/pt_routed.dcp" $row $col]
        write_checkpoint -cell $rp $file_name
}

foreach rp $rps {
	update_design -cell $rp -black_box
}

lock_design -level routing

write_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/static_routed.dcp


