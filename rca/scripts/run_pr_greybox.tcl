#Script to just generate Greybox full bitstream

open_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/top_bb.dcp

set rps [get_cells -hierarchical ou]

foreach rp $rps {
	set_property HD.RECONFIGURABLE true $rp

}


#Load PBlocks and add cells to PBlocks
read_xdc /home/anv17/FYP/fyp-rca/rca/xdcs/pblocks.xdc
puts "Implementing Greybox modules"

set rps [get_cells -hierarchical ou]
foreach rp $rps {
	update_design -cell $rp -buffer_ports
}

place_design
route_design

write_checkpoint -force  /home/anv17/FYP/fyp-rca/rca/dcps/greybox_routed.dcp
set bitstream_dir /home/anv17/FYP/fyp-rca/rca/bitstreams/

set bs_name greybox
set bs_path $bitstream_dir$bs_name
write_bitstream $bs_path -force -bin_file -no_partial_bitfile
close_design

exec "cd /home/anv17/FYP/fyp-rca/rca/bitstreams"
exec "bootgen -image greybox.bif -arch zynq -process_bitstream bin -w on"