module uart_tx
    (
        input logic clk, rst,
        input logic tx_start,      // pulse one tick when transmission should begin
        input logic tick,          // baud rate oversampled tick
        input logic [7:0] din,     // data to send (user must keep data valid
                                   // at least until tx_done_tick is asserted)
        output logic tx_done_tick, // pulse one tick when done
        output logic tx            // serial data
    );

    /* verilator public_module */

    parameter idle = 2'b00;
    parameter start = 2'b01;
    parameter data = 2'b10;
    parameter stop = 2'b11;

    reg[1:0] state_reg = idle;
    reg[1:0] state_next;

    logic [3:0] counter = 4'd0;
    logic [3:0] bit_num = 4'd0;
    logic write_next = 0;

    always_ff @(posedge clk) begin
        tx_done_tick <= 0;
        if (tick) begin
            state_reg = state_next;
            counter <= counter + 1;
            write_next <= counter == 4'd0;
            if (write_next) begin
                if (state_reg == start)
                    tx <= 0;
                if (state_reg == data) begin
                    tx <= din[bit_num[2:0]];
                    bit_num <= bit_num + 1;
                end 
                if (state_reg == stop) begin
                    tx <= 1;
                    tx_done_tick <= 1;
                    bit_num <= 0;
                    state_reg = idle;
                end
            end
        end
        if (state_reg == idle)
            tx <= 1;
        if (rst) begin
            counter <= 0;
            bit_num <= 0;
            state_reg = idle;
            tx <= 1;
        end
    end

    always_comb begin
        state_next = state_reg;
        if (write_next) begin
            case (state_reg)
                idle: begin
                    if (tx_start)
                        state_next = start;
                end 
                start: begin
                    state_next = data;
                end
                data: begin
                    if (bit_num == 4'd8)
                        state_next = stop;
                end
                stop: begin
                    state_next = idle;
                end
            endcase
        end
    end

endmodule
