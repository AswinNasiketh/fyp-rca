set bitstream_dir /home/anv17/FYP/fyp-rca/rca/bitstreams/
set bif_ext ".bif"
set rm_files [glob /home/anv17/FYP/fyp-rca/rca/pr_modules/*.sv]

#form module names

set rm_names {}

foreach file_name $rm_files {
	set rm_name [string map {".sv" ""} [scan $file_name "/home/anv17/FYP/fyp-rca/rca/pr_modules/%s"]] 
	lappend rm_names $rm_name
}

lappend rm_names "pt"
lappend rm_names "greybox"

set rp_dirs [glob /home/anv17/FYP/fyp-rca/rca/bitstreams/rps/*]

foreach name $rm_names {
	set bif_file_path $bitstream_dir$name$bif_ext
	
	set bif_contents "all:
	{
        	$name.bit /* Bitstream file name */
	}"
	
	exec echo $bif_contents > $bif_file_path
	
	foreach rp_dir $rp_dirs {
		set copy_path [append rp_dir "/."]
		exec cp $bif_file_path $copy_path
	}
}

