#Script to apply pblocks XDC and set the load the biggest RM Synthesised design into the OU black boxes

#Get RP list

open_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/top_rcf.dcp
set rps [get_cells -hierarchical ou]

#Load PBlocks and add cells to PBlocks
read_xdc /home/anv17/FYP/fyp-rca/rca/xdcs/pblocks.xdc

#Apply SRA RMs (biggest)
foreach rp $rps {
	read_checkpoint -cell $rp /home/anv17/FYP/fyp-rca/rca/dcps/rm_ooc_synth_dcps/sra_ou_synth.dcp

}


write_checkpoint /home/anv17/FYP/fyp-rca/rca/dcps/top_rcf_pblocked.dcp -force
close_design



