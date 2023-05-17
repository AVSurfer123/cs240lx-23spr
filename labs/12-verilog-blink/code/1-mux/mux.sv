module mux
    (
        input logic a, b,
        input logic sel,
        output logic out
    );
    always_comb begin
        if (sel) 
            out = a;
        else 
            out = b; 
    end
endmodule
