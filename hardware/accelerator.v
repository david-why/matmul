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
            if (mem_wstrb == 0) begin
                // reading data
                if (mem_addr == 'h1003004) begin
                    mem_rdata <= memory + 1;
                    mem_ready <= 1;
                end
            end else begin
                // writing data
                if (mem_addr == 'h1003000) begin
                    memory <= mem_wdata;
                    mem_ready <= 1;
                end
            end
        end else begin
            mem_ready <= 0;
        end
    end

endmodule