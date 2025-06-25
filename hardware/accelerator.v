/*
Matrix multiplication accelerator. Handles the multiplication
of a row vector A and a matrix B.

Parameters:
- ADDR_WRITE, ADDR_READ, ADDR_END: The memory mapped regions.
- N: The chunk size (length of A and dimensions of B).
- INPUT_WIDTH: The width of an input number, in bits.
- RESULT_WIDTH: The width of an output result, in bits.

Memory map:
- ADDR_WRITE ~ ADDR_WRITE+4*R: The elements of vector A.
- ADDR_WRITE+4*R ~ ADDR_WRITE+4*(R+R*S): The elements of matrix B, in column-major order.
- ADDR_READ ~ ADDR_READ+8*S: The resulting vector.
*/

/* verilator lint_off UNOPTFLAT */

module accelerator #(
    parameter ADDR_WRITE = 'h1100000,
    parameter ADDR_READ = 'h1300000,
    parameter ADDR_DEBUG_READ = 'h1400000,
    parameter ADDR_END = 'h1500000,
    parameter R = 4, // ACC_CHUNK_ROWS
    parameter S = 4, // ACC_CHUNK_COLS
    parameter INPUT_WIDTH = 8,
    parameter RESULT_WIDTH = 16
) (
    input clk,

    input mem_valid,
    output reg mem_ready,
    input [31:0] mem_addr,
    input [31:0] mem_wdata,
    input [3:0] mem_wstrb,
    output reg [31:0] mem_rdata
);

    // storage

    // memory[INPUT_WIDTH*(i+1)-1:INPUT_WIDTH*i]: i-th element of array A
    // memory[INPUT_WIDTH*(R+c*R+r+1)-1:INPUT_WIDTH*(R+c*R+r)]: (r,c) of matrix B
    reg [INPUT_WIDTH*(R+R*S)-1:0] memory;
    wire [RESULT_WIDTH*S-1:0] result;

    // calculation

    wire [RESULT_WIDTH*(S*(R+1))-1:0] eachcol_results;

    genvar r, c;
    generate
        for (c = 0; c < S; c++) begin
            assign eachcol_results[RESULT_WIDTH*(c*(R+1)+1)-1:RESULT_WIDTH*c*(R+1)] = 0;
            for (r = 0; r < R; r++) begin
                assign eachcol_results[RESULT_WIDTH*(c*(R+1)+r+2)-1:RESULT_WIDTH*(c*(R+1)+r+1)] = 
                    memory[INPUT_WIDTH*(r+1)-1:INPUT_WIDTH*r] * // element of A
                    memory[INPUT_WIDTH*(R+c*R+r+1)-1:INPUT_WIDTH*(R+c*R+r)] + // element of B
                    eachcol_results[RESULT_WIDTH*(c*(R+1)+r+1)-1:RESULT_WIDTH*(c*(R+1)+r)]; // previous result
            end
            assign result[RESULT_WIDTH*(c+1)-1:RESULT_WIDTH*c] = eachcol_results[RESULT_WIDTH*(c*(R+1)+R+1)-1:RESULT_WIDTH*(c*(R+1)+R)];
        end
    endgenerate

    // memory interface

    wire [31:0] olddata_mask;
    wire [31:0] newdata_mask;

    assign newdata_mask = {{8{mem_wstrb[0]}}, {8{mem_wstrb[1]}}, {8{mem_wstrb[2]}}, {8{mem_wstrb[3]}}};
    assign olddata_mask = ~newdata_mask;

    always @(posedge clk) begin
        if (mem_valid) begin
            if (mem_wstrb == 0) begin
                // reading data
                if (mem_addr >= ADDR_WRITE && mem_addr < ADDR_READ) begin
                    mem_rdata <= memory[(mem_addr&'hFFFFF)*8 +: 32];
                    mem_ready <= 1;
                end else if (mem_addr == ADDR_DEBUG_READ + 0) begin
                    mem_rdata <= R;
                    mem_ready <= 1;
                end else if (mem_addr == ADDR_DEBUG_READ + 4) begin
                    mem_rdata <= S;
                    mem_ready <= 1;
                end else if (mem_addr == ADDR_DEBUG_READ + 8) begin
                    mem_rdata <= INPUT_WIDTH;
                    mem_ready <= 1;
                end else if (mem_addr == ADDR_DEBUG_READ + 12) begin
                    mem_rdata <= RESULT_WIDTH;
                    mem_ready <= 1;
                end else if (mem_addr >= ADDR_READ && mem_addr < ADDR_END) begin
                    mem_rdata <= result[(mem_addr&'hFFFFF)*8 +: 32];
                    mem_ready <= 1;
                end else begin
                    mem_ready <= 0;
                end
            end else begin
                // writing data
                if (mem_addr >= ADDR_WRITE && mem_addr < ADDR_READ) begin
                    memory[(mem_addr&'hFFFFF)*8 +: 32] <= (newdata_mask & mem_wdata) | (olddata_mask & memory[(mem_addr&'hFFFFF)*8 +: 32]);
                    mem_ready <= 1;
                end else begin
                    mem_ready <= 0;
                end
            end
        end else begin
            mem_ready <= 0;
        end
    end

endmodule