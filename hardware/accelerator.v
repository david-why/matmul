/*
Matrix multiplication accelerator for uint32. Handles the multiplication
of a row vector of A and a matrix B.

Parameters:
- ADDR_WRITE, ADDR_READ, ADDR_END: The memory mapped regions.
- N: The chunk size (length of A and dimensions of B).
- INPUT_WIDTH: The width of an input number, in bits.
- RESULT_WIDTH: The width of an output result, in bits.

Memory map:
- ADDR_WRITE ~ ADDR_WRITE+4*N: The elements of vector A.
- ADDR_WRITE+4*N ~ ADDR_WRITE+4*(N+N*N): The elements of matrix B, in column-major order.
- ADDR_READ ~ ADDR_READ+8*N: The resulting vector.
*/

module accelerator #(
    parameter ADDR_WRITE = 'h1100000,
    parameter ADDR_READ = 'h1300000,
    parameter ADDR_END = 'h1500000,
    parameter N = 4,
    parameter INPUT_WIDTH = 32,
    parameter RESULT_WIDTH = 64
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
    // memory[INPUT_WIDTH*(N+c*N+r+1)-1:INPUT_WIDTH*(N+c*N+r)]: (r,c) of matrix B
    reg [INPUT_WIDTH*(N+N*N)-1:0] memory;
    wire [RESULT_WIDTH*N-1:0] result;

    // calculation

    wire [RESULT_WIDTH*(N*(N+1))-1:0] eachcol_results;

    genvar r, c;
    generate
        for (c = 0; c < N; c++) begin
            // wire [RESULT_WIDTH*(N+1)-1:0] col_results;
            assign eachcol_results[RESULT_WIDTH*(c*(N+1)+1)-1:RESULT_WIDTH*c*(N+1)] = 0;
            for (r = 0; r < N; r++) begin
                assign eachcol_results[RESULT_WIDTH*(c*(N+1)+r+2)-1:RESULT_WIDTH*(c*(N+1)+r+1)] = 
                    memory[INPUT_WIDTH*(r+1)-1] * // element of A
                    memory[INPUT_WIDTH*(N+c*N+r+1)-1:INPUT_WIDTH*(N+c*N+r)] + // element of B
                    eachcol_results[RESULT_WIDTH*(c*(N+1)+r+1)-1:RESULT_WIDTH*(c*(N+1)+r)]; // previous result
            end
            assign result[RESULT_WIDTH*(c+1)-1:RESULT_WIDTH*c] = eachcol_results[RESULT_WIDTH*(c*(N+1)+N+1)-1:RESULT_WIDTH*(c*(N+1)+N)];
        end
    endgenerate

    // memory interface

    always @(posedge clk) begin
        if (mem_valid) begin
            if (mem_wstrb == 0) begin
                // reading data
                if (mem_addr >= ADDR_READ && mem_addr < ADDR_END) begin
                    mem_rdata <= result[(mem_addr&'hFFFFF)*8 +: 32];
                    mem_ready <= 1;
                end else if (mem_addr >= ADDR_WRITE && mem_addr < ADDR_READ) begin
                    mem_rdata <= memory[(mem_addr&'hFFFFF)*8 +: 32];
                    mem_ready <= 1;
                end else begin
                    mem_ready <= 0;
                end
            end else begin
                // writing data
                if (mem_addr >= ADDR_WRITE && mem_addr < ADDR_READ) begin
                    memory[(mem_addr&'hFFFFF)*8 +: 32] <= mem_wdata;
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