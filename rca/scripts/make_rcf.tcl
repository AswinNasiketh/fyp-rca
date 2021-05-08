#TCL Script to update all PR Grid slot cells to black boxes and set their RECONFIGURABLE property


open_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/top_bb.dcp

set rps [get_cells -hierarchical ou]

foreach rp $rps {
	set_property HD.RECONFIGURABLE true $rp
#	update_design -cell $rp -black_box
}

write_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/top_rcf.dcp -force
close_design
