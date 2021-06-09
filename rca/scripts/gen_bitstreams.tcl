#Script to generate bitstreams from routed dcps


set routed_dcps [glob /home/anv17/FYP/fyp-rca/rca/dcps/full_configs/*.dcp]
set bitstream_dir /home/anv17/FYP/fyp-rca/rca/bitstreams/

foreach cfg $routed_dcps {
	open_checkpoint $cfg
	set bs_name [string map {"_routed.dcp" ""} [scan $cfg "/home/anv17/FYP/fyp-rca/rca/dcps/full_configs/%s"]]
	set bs_path $bitstream_dir$bs_name
	write_bitstream $bs_path -force
	close_design
}

#Generate Greybox bitstreams
open_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/greybox_routed.dcp
set bs_name greybox
set bs_path $bitstream_dir$bs_name
write_bitstream $bs_path -force
close_design

#Organise partial bitstreams into correct folders

set pbitstreams [glob /home/anv17/FYP/fyp-rca/rca/bitstreams/*_partial.bit]

foreach bs $pbitstreams {
	scan $bs "/home/anv17/FYP/fyp-rca/rca/bitstreams/%\[a-z\]_pblock_ou_%u_%u_partial.bit" ou row col
	set new_path [format "/home/anv17/FYP/fyp-rca/rca/bitstreams/rps/rp_%u_%u/%s.bit" $row $col $ou] 
	exec mv $bs $new_path
}

set rp_dirs [glob /home/anv17/FYP/fyp-rca/rca/bitstreams/rps/*]
foreach rp_dir $rp_dirs {
	cd $rp_dir
	set bifs [glob *.bif]
	foreach bif $bifs {
		exec bootgen -image $bif -arch zynq -process_bitstream bin -w on
	} 
}

cd $bitstream_dir
exec bootgen -image greybox.bif -arch zynq -process_bitstream bin -w on
