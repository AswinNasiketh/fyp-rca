import taiga_config::*;
import riscv_types::*;
import taiga_types::*;
import rca_config::*;

module rca_config_regs (
    input clk,
    input rst,

    //Reg file to store which of the CPU regs to read from and write to
    //Read interface
    input [$clog2(NUM_RCAS)-1:0] rca_sel,
    input rca_use_fb_instr,
    
    output logic [4:0] [NUM_READ_PORTS-1:0] rca_cpu_src_reg_addrs,
    output logic [4:0] [NUM_WRITE_PORTS-1:0] rca_cpu_dest_reg_addrs,

    //Write interface
    input cpu_fb_reg_addr_wr_en,
    input cpu_nfb_reg_addr_wr_en,
    input [$clog2(NUM_READ_PORTS)-1:0]  cpu_port_sel,
    input cpu_src_dest_port, //0 for src_addr reg, 1 for dest_addr_reg
    input [4:0] cpu_reg_addr,

    //Reg file to store grid crossbar configurations
    //Read interface
    input [$clog2(NUM_GRID_MUXES)-1:0] grid_mux_addr,
    output [$clog2(GRID_MUX_INPUTS)-1:0] curr_grid_mux_sel,
    
    //Write interface - uses address from read interface
    input grid_mux_wr_en,
    input [$clog2(GRID_MUX_INPUTS)-1:0] new_grid_mux_sel,

    //Reg file to store IO Unit crossbar configurations
    //Read interface
    input [$clog2(GRID_NUM_ROWS)-1:0] io_mux_addr,
    output [$clog2(IO_UNIT_MUX_INPUTS)-1:0] curr_io_mux_sels [GRID_NUM_ROWS],
    
    //Write interface - uses address from read interface
    input io_mux_wr_en,
    input [$clog2(IO_UNIT_MUX_INPUTS)-1:0] new_io_mux_sel,

    //Reg file to store RCA Result Unit crossbar configurations - uses rca_sel from first reg file interface
    //Read interface
    input [$clog2(NUM_WRITE_PORTS)-1:0] rca_result_mux_addr,
    output [$clog2(GRID_NUM_ROWS)-1:0] curr_rca_result_mux_sel [NUM_WRITE_PORTS],
    
    //Write interface - uses address from read interface
    input rca_result_mux_wr_en,
    input [$clog2(GRID_NUM_ROWS)-1:0] new_rca_result_mux_sel,

    //Reg file to store which IO (input) unit is associated with which accelerator - for data_valid signal generation
    //Uses rca_sel signal from above
    output [GRID_NUM_ROWS-1:0] curr_rca_io_inp_map,

    input rca_io_inp_map_wr_en,
    input [GRID_NUM_ROWS-1:0] new_rca_io_inp_map
);

    logic [4:0] [NUM_READ_PORTS-1:0] cpu_src_reg_addrs [NUM_RCAS]; 
    logic [4:0] [NUM_WRITE_PORTS-1:0] cpu_dest_fb_reg_addrs [NUM_RCAS];    
    logic [4:0] [NUM_WRITE_PORTS-1:0] cpu_dest_nfb_reg_addrs [NUM_RCAS];    

    logic [$clog2(GRID_MUX_INPUTS)-1:0] grid_mux_sels [NUM_GRID_MUXES];

    logic [$clog2(IO_UNIT_MUX_INPUTS)-1:0] io_unit_mux_sels [GRID_NUM_ROWS];

    typedef logic [$clog2(GRID_NUM_ROWS)-1:0] rca_result_mux_sel_t [NUM_WRITE_PORTS];
    rca_result_mux_sel_t rca_result_mux_sels [NUM_RCAS];

    logic [GRID_NUM_ROWS-1:0] rca_io_inp_map [NUM_RCAS];

    // Implementation - Reg file to store which of the CPU regs to read from and write to
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
            if (cpu_src_dest_port == 0) cpu_src_reg_addrs[rca_sel][cpu_port_sel] <= cpu_reg_addr;
            else cpu_dest_fb_reg_addrs[rca_sel][cpu_port_sel] <= cpu_reg_addr;
        end
        else if (cpu_nfb_reg_addr_wr_en) begin
            cpu_dest_nfb_reg_addrs[rca_sel][cpu_port_sel] <= cpu_reg_addr;
        end
    end

    always_comb begin
        if (rca_use_fb_instr) begin
            rca_cpu_src_reg_addrs = cpu_src_reg_addrs[rca_sel];
            rca_cpu_dest_reg_addrs = cpu_dest_fb_reg_addrs[rca_sel];
        end
        else begin
            rca_cpu_src_reg_addrs = 0; //for non feedback use instrs, don't supply source regs
            rca_cpu_dest_reg_addrs = cpu_dest_nfb_reg_addrs[rca_sel];
        end
    end    

    //Implementation - Reg file to store grid crossbar configuration
    initial grid_mux_sels = '{default: '0};

    always_ff @(posedge clk) begin
        if (rst) grid_mux_sels <= '{default: '0};        
        else if (grid_mux_wr_en) grid_mux_sels[grid_mux_addr] <= new_grid_mux_sel;
    end

    assign curr_grid_mux_sel = grid_mux_sels[grid_mux_addr];

    //Implementation - Reg file to store io unit crossbar configuration (same as above)
    initial io_unit_mux_sels = '{default: '0};

    always_ff @(posedge clk) begin
        if (rst) io_unit_mux_sels <= '{default: '0};        
        else if (io_mux_wr_en) io_unit_mux_sels[io_mux_addr] <= new_io_mux_sel;
    end

    assign curr_io_mux_sels = io_unit_mux_sels;

    //Implementation - Reg file to store rca result crossbar configuration
    initial begin
        for (int i = 0; i < NUM_RCAS; i++) begin
            for (int j = 0; j < NUM_WRITE_PORTS; j++)
                rca_result_mux_sels[i][j] = 0;
        end
    end

    always_ff @(posedge clk) begin
        if (rst) begin
            for (int i = 0; i < NUM_RCAS; i++) begin
                for (int j = 0; j < NUM_WRITE_PORTS; j++)
                    rca_result_mux_sels[i][j] = 0;
            end
        end     
        else if (rca_result_mux_wr_en) rca_result_mux_sels[rca_sel][rca_result_mux_addr] <= new_rca_result_mux_sel;
    end

    always_comb begin
        for (int i = 0; i < NUM_WRITE_PORTS; i++)
            curr_rca_result_mux_sel[i] = rca_result_mux_sels[rca_sel][i];
    end

    //Implementation - Reg file to store which IO (input) unit is associated with which accelerator
    initial rca_io_inp_map = '{default: '0};

    always_ff @(posedge clk) begin
        if (rst) rca_io_inp_map = '{default: '0};
        else if (rca_io_inp_map_wr_en) rca_io_inp_map[rca_sel] <= new_rca_io_inp_map;
    end

    always_comb curr_rca_io_inp_map <= rca_io_inp_map[rca_sel];


    //Assertions 
    write_nfb_src_reg_addr:
        assert property (@(posedge clk) disable iff (rst) !(cpu_nfb_reg_addr_wr_en & cpu_src_dest_port == 0))
        else $error("Write of src reg address for non feedback register set occurred");

    write_nfb_fb_reg_addr:
        assert property (@(posedge clk) disable iff (rst) !(cpu_nfb_reg_addr_wr_en & cpu_fb_reg_addr_wr_en))
        else $error("Trying to write feedback and non feedback register addresses simultaneously");
endmodule