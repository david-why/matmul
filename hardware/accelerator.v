module accelerator #(
    parameter ADDR_WRITE = 'h1003000,
    parameter ADDR_READ = 'h1004000,
    parameter N = 3
) (
    input clk,

    input mem_valid,
    output reg mem_ready,
    input [31:0] mem_addr,
    input [31:0] mem_wdata,
    input [3:0] mem_wstrb,
    output reg [31:0] mem_rdata,

    output [32*N-1:0] out_items
);

    reg [32*N-1:0] items = 0;

    wire [64*N-1:0] mul_result;
    wire [63:0] result;

    genvar i;
    assign mul_result[63:0] = {32'b0, items[31:0]};
    generate
        for (i = 1; i < N; i++) begin
            assign mul_result[64*i+63:64*i] = mul_result[64*i-1:64*i-64] * items[32*i+31:32*i];
        end
    endgenerate
    assign result = mul_result[64*N-1:64*N-64];

    always @(posedge clk) begin
        if (mem_valid) begin
            if (mem_wstrb == 0) begin
                // reading data
                if (mem_addr == ADDR_READ) begin
                    mem_rdata <= result[31:0];
                    mem_ready <= 1;
                end else if (mem_addr == ADDR_READ + 4) begin
                    mem_rdata <= result[63:32];
                    mem_ready <= 1;
                end
            end else begin
                // writing data
                if (mem_addr == ADDR_WRITE) begin
                    items[31:0] <= mem_wdata;
                    mem_ready <= 1;
                end else if (mem_addr == ADDR_WRITE + 4) begin
                    items[63:32] <= mem_wdata;
                    mem_ready <= 1;
                end else if (mem_addr == ADDR_WRITE + 8) begin
                    items[95:64] <= mem_wdata;
                    mem_ready <= 1;
                end
            end
        end else begin
            mem_ready <= 0;
        end
    end

    assign out_items = items;

endmodule