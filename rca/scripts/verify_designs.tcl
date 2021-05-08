#Script to run PR Verify
set config_dcps [glob /home/anv17/FYP/fyp-rca/rca/dcps/full_configs/*.dcp]


pr_verify -full_check -initial /home/anv17/FYP/fyp-rca/rca/dcps/greybox_routed.dcp -additional $config_dcps
