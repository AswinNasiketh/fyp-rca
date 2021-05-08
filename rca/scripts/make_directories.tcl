#Script to make directories for each RP

#DCPS
#for {set row 0} {$row < 5} {incr row} {
#	for {set col 0} {$col < 6} {incr col} {
#		set dir_name [format "/home/anv17/FYP/fyp-rca/rca/dcps/slot_dcps/rp_%u_%u" $row $col]
#		exec mkdir $dir_name
#	}
#}
#Bitstreams
for {set row 0} {$row < 5} {incr row} {
	for {set col 0} {$col < 6} {incr col} {
		set dir_name [format "/home/anv17/FYP/fyp-rca/rca/bitstreams/rps/rp_%u_%u" $row $col]
		exec mkdir $dir_name
	}
}
