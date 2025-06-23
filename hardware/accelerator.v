module accelerator (
    input clk,

    input mem_valid,
    output reg mem_ready,
    input [31:0] mem_addr,
    input [31:0] mem_wdata,
    input [3:0] mem_wstrb,
    output reg [31:0] mem_rdata
);

    reg [31:0] memory;

    always @(posedge clk) begin
        if (mem_valid) begin
            mem_ready <= 1'b1;
            if (mem_wstrb == 4'b0) begin
                // reading data
                mem_rdata <= memory + 1;
                mem_ready <= 1;
            end else begin
                // writing data
                memory <= mem_wdata;
                mem_ready <= 1;
            end
        end
    end

endmodule