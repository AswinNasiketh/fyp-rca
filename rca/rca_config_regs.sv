import taiga_config::*;
import riscv_types::*;
import taiga_types::*;
import rca_config::*;

module rca_config_regs (
    input clk,
    input rst,

    //Reg file to store which of the CPU regs to read from and write to
    //Read interface for decode stage
    input [$clog2(NUM_RCAS)-1:0] rca_sel_decode,
    input rca_use_fb_instr_decode,
    
    output logic [4:0] [NUM_READ_PORTS-1:0] rca_cpu_src_reg_addrs_decode,
    output logic [4:0] [NUM_WRITE_PORTS-1:0] rca_cpu_dest_reg_addrs_decode,

    input [$clog2(NUM_RCAS)-1:0] rca_sel_issue,
    input [$clog2(NUM_RCAS)-1:0] rca_sel_grid_wb,
    input [$clog2(NUM_RCAS)-1:0] rca_sel_buf,

    //Write interface
    input cpu_fb_reg_addr_wr_en,
    input cpu_nfb_reg_addr_wr_en,
    input [$clog2(NUM_READ_PORTS)-1:0] cpu_port_sel,
    input cpu_src_dest_port, //0 for src_addr reg, 1 for dest_addr_reg
    input [4:0] cpu_reg_addr,

    //Reg file to store grid crossbar configurations
    //Read interface
    output [$clog2(GRID_MUX_INPUTS)-1:0] grid_mux_sel_out [NUM_GRID_MUXES*2],
    
    //Write interface
    input grid_mux_wr_en,
    input [$clog2(NUM_GRID_MUXES*2)-1:0] grid_mux_wr_addr,
    input [$clog2(GRID_MUX_INPUTS)-1:0] new_grid_mux_sel,

    //Reg file to store IO Unit crossbar configurations
    //Read interface
    input [$clog2(NUM_IO_UNITS*2)-1:0] io_mux_addr,
    output [$clog2(IO_UNIT_MUX_INPUTS)-1:0] curr_io_mux_sels [NUM_IO_UNITS*2],
    
    //Write interface - uses address from read interface
    input io_mux_wr_en,
    input [$clog2(IO_UNIT_MUX_INPUTS)-1:0] new_io_mux_sel,

    //Reg file to store RCA Result Unit crossbar configurations - uses rca_sel_grid_wb for reading
    //Read interface  
    input [$clog2(NUM_WRITE_PORTS)-1:0] rca_result_mux_addr,
    output logic [$clog2(NUM_IO_UNITS+1)-1:0] curr_fb_rca_result_mux_sel [NUM_WRITE_PORTS],
    output logic [$clog2(NUM_IO_UNITS+1)-1:0] curr_nfb_rca_result_mux_sel [NUM_WRITE_PORTS],
    
    //Write interface - uses address from read interface and rca_sel_issue for writing
    input rca_fb_result_mux_wr_en,
    input rca_nfb_result_mux_wr_en,
    input [$clog2(NUM_IO_UNITS+1)-1:0] new_rca_result_mux_sel,

    //Reg file to store which IO (input) unit is associated with which accelerator - for data_valid signal generation
    //Uses rca_sel signal from above
    output logic [NUM_IO_UNITS-1:0] curr_rca_io_inp_map,

    input rca_io_inp_map_wr_en,
    input [NUM_IO_UNITS-1:0] new_rca_io_inp_map,

    //Reg file to store custom constant which can be used as input instead of a CPU register
    output logic [XLEN-1:0] input_constants_out [NUM_IO_UNITS],

    input rca_input_constants_wr_en,
    input [$clog2(NUM_IO_UNITS)-1:0] io_unit_addr,
    input [XLEN-1:0] new_input_constant,

    //Reg file to store IO unit LS masks - uses rca_sel already defined
    input rca_io_ls_mask_wr_en,
    input rca_io_ls_mask_fb_wr_en,
    input [NUM_IO_UNITS-1:0] new_io_ls_mask,
    output logic [NUM_IO_UNITS-1:0] curr_io_ls_mask_fb,
    output logic [NUM_IO_UNITS-1:0] curr_io_ls_mask_nfb
);

    logic [4:0] [NUM_READ_PORTS-1:0] cpu_src_reg_addrs [NUM_RCAS]; 
    logic [4:0] [NUM_WRITE_PORTS-1:0] cpu_dest_fb_reg_addrs [NUM_RCAS];    
    logic [4:0] [NUM_WRITE_PORTS-1:0] cpu_dest_nfb_reg_addrs [NUM_RCAS];    

    logic [$clog2(GRID_MUX_INPUTS)-1:0] grid_mux_sels [NUM_GRID_MUXES*2];

    logic [$clog2(IO_UNIT_MUX_INPUTS)-1:0] io_unit_mux_sels [NUM_IO_UNITS*2];

    typedef logic [$clog2(NUM_IO_UNITS+1)-1:0] rca_result_mux_sel_t [NUM_WRITE_PORTS];
    rca_result_mux_sel_t rca_result_mux_sels_fb [NUM_RCAS];
    rca_result_mux_sel_t rca_result_mux_sels_nfb [NUM_RCAS];

    logic [NUM_IO_UNITS-1:0] rca_io_inp_map [NUM_RCAS];

    logic [NUM_IO_UNITS-1:0] io_ls_mask_fb [NUM_RCAS];
    logic [NUM_IO_UNITS-1:0] io_ls_mask_nfb [NUM_RCAS];

    //Reg file to store which of the CPU regs to read from and write to
    initial begin
        cpu_src_reg_addrs = '{default: '0};
        cpu_dest_fb_reg_addrs = '{default: '0};
        cpu_dest_nfb_reg_addrs = '{default: '0};
    end

    always_ff @(posedge clk) begin
        if (rst) begin
            cpu_src_reg_addrs = '{default: '0};
            cpu_dest_fb_reg_addrs = '{default: '0};
            cpu_dest_nfb_reg_addrs = '{default: '0};
        end
        else if (cpu_fb_reg_addr_wr_en) begin
            if (cpu_src_dest_port == 0) cpu_src_reg_addrs[rca_sel_issue][cpu_port_sel] <= cpu_reg_addr;
            else cpu_dest_fb_reg_addrs[rca_sel_issue][cpu_port_sel] <= cpu_reg_addr;
        end
        else if (cpu_nfb_reg_addr_wr_en) begin
            cpu_dest_nfb_reg_addrs[rca_sel_issue][cpu_port_sel] <= cpu_reg_addr;
        end
    end

    always_comb begin
        if (rca_use_fb_instr_decode) begin
            rca_cpu_src_reg_addrs_decode = cpu_src_reg_addrs[rca_sel_decode];
            rca_cpu_dest_reg_addrs_decode = cpu_dest_fb_reg_addrs[rca_sel_decode];
        end
        else begin
            rca_cpu_src_reg_addrs_decode = 0; //for non feedback use instrs, don't supply source regs
            rca_cpu_dest_reg_addrs_decode = cpu_dest_nfb_reg_addrs[rca_sel_decode];
        end
    end    

    //Reg file to store grid crossbar configuration
    initial grid_mux_sels = '{default: '0};

    always_ff @(posedge clk) begin
        if (rst) grid_mux_sels <= '{default: '0};        
        else if (grid_mux_wr_en) grid_mux_sels[grid_mux_wr_addr] <= new_grid_mux_sel;
    end

    assign grid_mux_sel_out = grid_mux_sels;

    // Reg file to store io unit crossbar configuration (same as above)
    initial io_unit_mux_sels = '{default: '0};

    always_ff @(posedge clk) begin
        if (rst) io_unit_mux_sels <= '{default: '0};        
        else if (io_mux_wr_en) io_unit_mux_sels[io_mux_addr] <= new_io_mux_sel;
    end

    assign curr_io_mux_sels = io_unit_mux_sels;

    //Reg file to store rca result crossbar configuration for feedback results
    initial begin
        for (int i = 0; i < NUM_RCAS; i++) begin
            for (int j = 0; j < NUM_WRITE_PORTS; j++)
                rca_result_mux_sels_fb[i][j] = UNUSED_WRITE_PORT_ADDR;
        end
    end

    always_ff @(posedge clk) begin
        if (rst) begin
            for (int i = 0; i < NUM_RCAS; i++) begin
                for (int j = 0; j < NUM_WRITE_PORTS; j++)
                    rca_result_mux_sels_fb[i][j] = UNUSED_WRITE_PORT_ADDR;
            end
        end     
        else if (rca_fb_result_mux_wr_en) rca_result_mux_sels_fb[rca_sel_issue][rca_result_mux_addr] <= new_rca_result_mux_sel;
    end

    always_comb begin
        for (int i = 0; i < NUM_WRITE_PORTS; i++)
            curr_fb_rca_result_mux_sel[i] = rca_result_mux_sels_fb[rca_sel_grid_wb][i];
    end

    //Reg file to store rca result crossbar configuration for non-feedback results
    initial begin
        for (int i = 0; i < NUM_RCAS; i++) begin
            for (int j = 0; j < NUM_WRITE_PORTS; j++)
                rca_result_mux_sels_nfb[i][j] = UNUSED_WRITE_PORT_ADDR;
        end
    end

    always_ff @(posedge clk) begin
        if (rst) begin
            for (int i = 0; i < NUM_RCAS; i++) begin
                for (int j = 0; j < NUM_WRITE_PORTS; j++)
                    rca_result_mux_sels_nfb[i][j] = UNUSED_WRITE_PORT_ADDR;
            end
        end     
        else if (rca_nfb_result_mux_wr_en) rca_result_mux_sels_nfb[rca_sel_issue][rca_result_mux_addr] <= new_rca_result_mux_sel;
    end

    always_comb begin
        for (int i = 0; i < NUM_WRITE_PORTS; i++)
            curr_nfb_rca_result_mux_sel[i] = rca_result_mux_sels_nfb[rca_sel_grid_wb][i];
    end

    //Reg file to store which IO (input) unit is associated with which accelerator - for passing through data valid
    initial rca_io_inp_map = '{default: '0};

    always_ff @(posedge clk) begin
        if (rst) rca_io_inp_map = '{default: '0};
        else if (rca_io_inp_map_wr_en) rca_io_inp_map[rca_sel_issue] <= new_rca_io_inp_map;
    end

    always_comb curr_rca_io_inp_map = rca_io_inp_map[rca_sel_buf];

    initial input_constants_out = '{default: '0};

    always_ff @(posedge clk) begin
        if (rst) input_constants_out = '{default: '0};
        else if(rca_input_constants_wr_en) input_constants_out[io_unit_addr] <= new_input_constant;
    end

    //Reg files to store IO unit LS masks
    initial io_ls_mask_fb = '{default: '0};
    initial io_ls_mask_nfb = '{default: '0};

    always_ff @(posedge clk) begin
        if(rst) begin
            io_ls_mask_fb <= '{default: '0};
            io_ls_mask_nfb <= '{default: '0};
        end
        else if(rca_io_ls_mask_wr_en) begin
            if(rca_io_ls_mask_fb_wr_en)
                io_ls_mask_fb[rca_sel_issue] <= new_io_ls_mask;
            else
                io_ls_mask_nfb[rca_sel_issue] <= new_io_ls_mask;
        end
    end

    always_comb curr_io_ls_mask_nfb = io_ls_mask_nfb[rca_sel_grid_wb];
    always_comb curr_io_ls_mask_fb = io_ls_mask_fb[rca_sel_grid_wb];

    //Assertions 
    write_nfb_src_reg_addr:
        assert property (@(posedge clk) disable iff (rst) !(cpu_nfb_reg_addr_wr_en & cpu_src_dest_port == 0))
        else $error("Write of src reg address for non feedback register set occurred");

    write_nfb_fb_reg_addr:
        assert property (@(posedge clk) disable iff (rst) !(cpu_nfb_reg_addr_wr_en & cpu_fb_reg_addr_wr_en))
        else $error("Trying to write feedback and non feedback register addresses simultaneously");
endmodule