// Count up to M and then assert max_tick for one cycle and reset
module counter
    #(
        parameter M = 10
    )
    (
        input  logic clk, rst,
        output logic max_tick
    );
    logic [$clog2(M)-1:0] q;
    always_ff @(posedge clk) begin
        max_tick <= 0;
        if (rst)
            q <= 0;
        else if (q == M[$clog2(M)-1:0]) begin
            q <= 0;
            max_tick <= 1;
        end
        else
            q <= q + 1;
    end
endmodule
