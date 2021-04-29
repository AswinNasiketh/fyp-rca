#*****************************************************************************************
# Vivado (TM) v2018.3 (64-bit)
#
# tmp_edit_project.tcl: Tcl script for re-creating project 'tmp_edit_project'
#
# Generated by Vivado on Thu Dec 20 14:43:20 PST 2018
# IP Build 2404404 on Fri Dec  7 01:43:56 MST 2018
#
# This file contains the Vivado Tcl commands for re-creating the project to the state*
# when this script was generated. In order to re-create the project, please source this
# file in the Vivado Tcl Shell.
#
# * Note that the runs in the created project will be configured the same way as the
#   original project, however they will not be launched automatically. To regenerate the
#   run results please launch the synthesis/implementation runs as needed.
# 
#*****************************************************************************************

# Set the reference directory for source file relative paths (by default the value is script directory path)
#set origin_dir [ file dirname [ file normalize [ info script ] ] ]
set origin_dir [file dirname [info script]]
puts $origin_dir

# Use origin directory path location variable, if specified in the tcl shell
if { [info exists ::origin_dir_loc] } {
  set origin_dir $::origin_dir_loc
}

# Set the project name
set _xil_proj_name_ "taiga_wrapper_IP"

# Use project name variable, if specified in the tcl shell
if { [info exists ::user_project_name] } {
  set _xil_proj_name_ $::user_project_name
}

variable script_file
set script_file "taiga_wrapper_IP.tcl"

# Help information for this script
proc print_help {} {
  variable script_file
  puts "\nDescription:"
  puts "Recreate a Vivado project from this script. The created project will be"
  puts "functionally equivalent to the original project for which this script was"
  puts "generated. The script contains commands for creating a project, filesets,"
  puts "runs, adding/importing sources and setting properties on various objects.\n"
  puts "Syntax:"
  puts "$script_file"
  puts "$script_file -tclargs \[--origin_dir <path>\]"
  puts "$script_file -tclargs \[--project_name <name>\]"
  puts "$script_file -tclargs \[--help\]\n"
  puts "Usage:"
  puts "Name                   Description"
  puts "-------------------------------------------------------------------------"
  puts "\[--origin_dir <path>\]  Determine source file paths wrt this path. Default"
  puts "                       origin_dir path value is \".\", otherwise, the value"
  puts "                       that was set with the \"-paths_relative_to\" switch"
  puts "                       when this script was generated.\n"
  puts "\[--project_name <name>\] Create project with the specified name. Default"
  puts "                       name is the name of the project from where this"
  puts "                       script was generated.\n"
  puts "\[--help\]               Print help information for this script"
  puts "-------------------------------------------------------------------------\n"
  exit 0
}

if { $::argc > 0 } {
  for {set i 0} {$i < $::argc} {incr i} {
    set option [string trim [lindex $::argv $i]]
    switch -regexp -- $option {
      "--origin_dir"   { incr i; set origin_dir [lindex $::argv $i] }
      "--project_name" { incr i; set _xil_proj_name_ [lindex $::argv $i] }
      "--help"         { print_help }
      default {
        if { [regexp {^-} $option] } {
          puts "ERROR: Unknown option '$option' specified, please type '$script_file -tclargs --help' for usage info.\n"
          return 1
        }
      }
    }
  }
}

# Set the directory path for the original project from where this script was exported
#This is where the IP project gets stored ?
set orig_proj_dir "[file normalize "$origin_dir/"]"

# Create project
create_project ${_xil_proj_name_} $origin_dir/${_xil_proj_name_} -part xc7z020clg484-1

# Set the directory path for the new project
set proj_dir [get_property directory [current_project]]

# Reconstruct message rules
# None

# Set project properties
set obj [current_project]
set_property -name "board_part" -value "em.avnet.com:zed:part0:1.4" -objects $obj
set_property -name "default_lib" -value "xil_defaultlib" -objects $obj
set_property -name "ip_cache_permissions" -value "read write" -objects $obj
set_property -name "ip_output_repo" -value "$proj_dir/${_xil_proj_name_}.cache/ip" -objects $obj
set_property -name "sim.ip.auto_export_scripts" -value "1" -objects $obj
set_property -name "simulator_language" -value "Mixed" -objects $obj
set_property -name "target_language" -value "Verilog" -objects $obj

# Create 'sources_1' fileset (if not found)
if {[string equal [get_filesets -quiet sources_1] ""]} {
  create_fileset -srcset sources_1
}

#import all sources from taiga repo directory
#Zavier: Eric says we only want the wrapper, and whatever type/interface file we need at first.
#The reasoning is: less files ati ntial package, less worry 
#import_files -fileset [get_filesets sources_1] $origin_dir/core
#import_files -fileset [get_filesets sources_1] $origin_dir/l2_arbiter
#import_files -fileset [get_filesets sources_1] $origin_dir/local_memory

import_files -norecurse $origin_dir/../../core/xilinx/taiga_wrapper_xilinx.sv -force
import_files -norecurse $origin_dir/../../l2_arbiter/l2_external_interfaces.sv -force
import_files -norecurse $origin_dir/../../local_memory/local_memory_interface.sv -force
import_files -norecurse $origin_dir/../../rca/rca_cpu_interfaces.sv -force
import_files -norecurse $origin_dir/../../core/external_interfaces.sv -force
import_files -norecurse $origin_dir/../../rca/rca_config.sv -force
import_files -norecurse $origin_dir/../../core/taiga_config.sv -force
import_files -norecurse $origin_dir/../../l2_arbiter/l2_config_and_types.sv -force

# Set IP repository paths
set obj [get_filesets sources_1]
set_property "ip_repo_paths" "[file normalize "$origin_dir/taiga_wrapper_IP"]" $obj

# Rebuild user ip_repo's index before adding any source files
update_ip_catalog -rebuild

# Add/Import constrs file and set constrs file properties
#set file "[file normalize "$origin_dir/examples/zedboard/zedboard_master_XDC_RevC_D_v3.xdc"]"
#set file_imported [import_files -fileset constrs_1 [list $file]]

# Set 'sources_1' fileset file properties for remote files
# None

# Set 'sources_1' fileset file properties for local files

# Set 'sources_1' fileset properties
set obj [get_filesets sources_1]
set_property -name "top" -value "taiga_wrapper_xilinx" -objects $obj
set_property -name "top_auto_set" -value "0" -objects $obj
set_property -name "top_file" -value " ${origin_dir}/core/taiga_wrapper_xilinx.sv" -objects $obj


# Remove interface files for taiga 
puts "INFO: Project created:${_xil_proj_name_}"

#Removal of SystemVerilog interface files, so initial IP packaging can be done
#CUrrently Vivado 2018.1 complains if there is any SV interfaces during the intial packaging
#But if we were to re-add the SV interface files back into the IP and repackage it, SV will not complain
#export_ip_user_files -of_objects  [get_files $origin_dir/${_xil_proj_name_}/${_xil_proj_name_}.srcs/sources_1/imports/core/interfaces.sv] -no_script -reset -force -quiet
#remove_files  $origin_dir/${_xil_proj_name_}/${_xil_proj_name_}.srcs/sources_1/imports/core/interfaces.sv
#export_ip_user_files -of_objects  [get_files $origin_dir/${_xil_proj_name_}/${_xil_proj_name_}.srcs/sources_1/imports/l2_arbiter/l2_interfaces.sv] -no_script -reset -force -quiet
#remove_files $origin_dir/${_xil_proj_name_}/${_xil_proj_name_}.srcs/sources_1/imports/l2_arbiter/l2_interfaces.sv

############## Initial IP Packaging########################################
ipx::package_project -import_files -force -root_dir $proj_dir
update_compile_order -fileset sources_1
set_property core_revision 2 [ipx::current_core]
ipx::create_xgui_files [ipx::current_core]
ipx::update_checksums [ipx::current_core]
ipx::save_core [ipx::current_core]



# To set the axi interface as aximm and port map all the signals over #
set_property abstraction_type_vlnv xilinx.com:interface:aximm_rtl:1.0 [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property bus_type_vlnv xilinx.com:interface:aximm:1.0 [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
ipx::remove_port_map arid [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
ipx::add_port_map WLAST [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_wlast [ipx::get_port_maps WLAST -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map BREADY [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_bready [ipx::get_port_maps BREADY -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map AWLEN [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_awlen [ipx::get_port_maps AWLEN -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map AWREADY [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_awready [ipx::get_port_maps AWREADY -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map ARBURST [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_arburst [ipx::get_port_maps ARBURST -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map RRESP [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_rresp [ipx::get_port_maps RRESP -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map RVALID [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_rvalid [ipx::get_port_maps RVALID -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map AWID [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_awid [ipx::get_port_maps AWID -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map RLAST [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_rlast [ipx::get_port_maps RLAST -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map ARID [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_arid [ipx::get_port_maps ARID -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map AWCACHE [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_awcache [ipx::get_port_maps AWCACHE -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map WREADY [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_wready [ipx::get_port_maps WREADY -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map WSTRB [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_wstrb [ipx::get_port_maps WSTRB -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map BRESP [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_bresp [ipx::get_port_maps BRESP -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map BID [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_bid [ipx::get_port_maps BID -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map ARLEN [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_arlen [ipx::get_port_maps ARLEN -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map RDATA [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_rdata [ipx::get_port_maps RDATA -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map BVALID [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_bvalid [ipx::get_port_maps BVALID -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map ARCACHE [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_arcache [ipx::get_port_maps ARCACHE -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map RREADY [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_rready [ipx::get_port_maps RREADY -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map AWVALID [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_awvalid [ipx::get_port_maps AWVALID -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map ARSIZE [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_arsize [ipx::get_port_maps ARSIZE -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map WDATA [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_wdata [ipx::get_port_maps WDATA -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map AWSIZE [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_awsize [ipx::get_port_maps AWSIZE -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map RID [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_rid [ipx::get_port_maps RID -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map ARADDR [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_araddr [ipx::get_port_maps ARADDR -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map AWADDR [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_awaddr [ipx::get_port_maps AWADDR -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map ARREADY [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_arready [ipx::get_port_maps ARREADY -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map WVALID [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_wvalid [ipx::get_port_maps WVALID -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map ARVALID [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_arvalid [ipx::get_port_maps ARVALID -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]
ipx::add_port_map AWBURST [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]
set_property physical_name m_axi_awburst [ipx::get_port_maps AWBURST -of_objects [ipx::get_bus_interfaces m_axi -of_objects [ipx::current_core]]]

#####Re-Adding of SV interfaces files
#set_property  ip_repo_paths  $origin_dir/${_xil_proj_name_} [current_project]
#current_project $_xil_proj_name_
#update_ip_catalog
#import_files -norecurse $origin_dir/l2_arbiter/l2_interfaces.sv -force
#import_files -norecurse $origin_dir/../../core/interfaces.sv -force

#####Re-Adding of project files
set_property  ip_repo_paths  $origin_dir/../../${_xil_proj_name_} [current_project]
current_project $_xil_proj_name_
update_ip_catalog
import_files -fileset [get_filesets sources_1] $origin_dir/../../core
import_files -fileset [get_filesets sources_1] $origin_dir/../../l2_arbiter
import_files -fileset [get_filesets sources_1] $origin_dir/../../local_memory
import_files -fileset [get_filesets sources_1] $origin_dir/../../rca

############## Re-packaging of core
update_compile_order -fileset sources_1
ipx::merge_project_changes files [ipx::current_core]
set_property core_revision 3 [ipx::current_core]
ipx::create_xgui_files [ipx::current_core]
ipx::update_checksums [ipx::current_core]
ipx::save_core [ipx::current_core]
current_project taiga_wrapper_IP
set_property "ip_repo_paths" "[file normalize "$origin_dir/taiga_wrapper_IP"]" $obj
update_ip_catalog -rebuild

