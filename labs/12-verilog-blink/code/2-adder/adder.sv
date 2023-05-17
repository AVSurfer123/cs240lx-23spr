module adder
    (
        input logic [31:0] a, b,
        output logic [31:0] sum
    );
    logic [31:0] c;
    half_adder ha(a[0], b[0], c[0], sum[0]);
    generate
        genvar i;
        for (i = 1; i < 32; i = i + 1)
            full_adder fa(a[i], b[i], c[i-1], c[i], sum[i]);
    endgenerate
endmodule

module half_adder
    (
        input logic a, b,
        output logic c, s
    );
    assign s = a ^ b;
    assign c = a & b;
endmodule

module full_adder
    (
        input logic a, b, cin,
        output logic cout, s
    );
    assign s = a ^ b ^ cin;//(~a & (b ^ cin)) | (a & ~(b ^ cin));
    assign cout = (~a & b & cin) | (a & (b | cin));
endmodule
