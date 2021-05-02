#TCL Script to update all PR Grid slot cells to black boxes and set their RECONFIGURABLE property


open_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/top_no_bb.dcp

for {set row 0} {$row < 12} {incr row} {
	for {set col 0} {$col < 6} {incr col} {
		set slot_name [format "pr_slots_row\[%u\].pr_slots_col\[%u\].grid_slot" $row $col]
		set cell_name [get_cells -hierarchical $slot_name]
		set_property HD.RECONFIGURABLE true $cell_name
		update_design -cell $cell_name -black_box
	}
}

#lock_design -level routing
write_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/top_bb_route_locked.dcp
