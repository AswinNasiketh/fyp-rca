#Script to run PR Flow
source /home/anv17/FYP/fyp-rca/rca/scripts/make_rcf.tcl
source /home/anv17/FYP/fyp-rca/rca/scripts/apply_pblocks_and_biggest_rm.tcl
source /home/anv17/FYP/fyp-rca/rca/scripts/imp_rms.tcl
source /home/anv17/FYP/fyp-rca/rca/scripts/verify_designs.tcl
source  /home/anv17/FYP/fyp-rca/rca/scripts/gen_bitstreams.tcl

exec "cd /home/anv17/FYP/fyp-rca/rca/bitstreams"
exec "bootgen -image greybox.bif -arch zynq -process_bitstream bin -w on"
