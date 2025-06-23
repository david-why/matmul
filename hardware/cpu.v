`include "picorv32.v"
`include "accelerator.v"

module cpu (
	input clk, resetn,
	output trap,

	output reg        mem_valid,
	output reg        mem_instr,
	input             mem_ready,

	output reg [31:0] mem_addr,
	output reg [31:0] mem_wdata,
	output reg [ 3:0] mem_wstrb,
	input      [31:0] mem_rdata
);

    // Instantiate the PicoRV32 core
    picorv32 #(
        .ENABLE_MUL(1),
        .ENABLE_DIV(1),
        .PROGADDR_RESET(32'h00800000)
    ) core (
        .clk(clk),
        .resetn(resetn),
        .trap(trap),

        // Memory interface
        .mem_valid(mem_valid),
        .mem_instr(mem_instr),
        .mem_ready(mem_ready),

        .mem_addr(mem_addr),
        .mem_wdata(mem_wdata),
        .mem_wstrb(mem_wstrb),
        .mem_rdata(mem_rdata),


        .mem_la_read(),
        .mem_la_write(),
        .mem_la_addr(),
        .mem_la_wdata(),
        .mem_la_wstrb(),

        .pcpi_valid(),
        .pcpi_insn(),
        .pcpi_rs1(),
        .pcpi_rs2(),
        .pcpi_wr(),
        .pcpi_rd(),
        .pcpi_wait(),
        .pcpi_ready(),

        .irq(),
        .eoi(),

        .trace_valid(),
        .trace_data()
    );

endmodule
