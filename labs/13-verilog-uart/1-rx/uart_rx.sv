module uart_rx
    (
        input logic clk, rst,
        input logic rx,            // serial data
        input logic tick,          // baud rate oversampled tick
        output logic rx_done_tick, // pulse one tick when done
        output logic [7:0] dout    // output data
    );

    /* verilator public_module */

    parameter idle = 2'b00;
    parameter start = 2'b01;
    parameter data = 2'b10;
    parameter stop = 2'b11;

    reg[1:0] state_reg = idle;
    reg[1:0] state_next;

    logic [3:0] counter = 4'd0;
    logic [2:0] bit_num = 3'd0;
    logic read_next = 0;
    logic val = 0;

    always_ff @(posedge clk) begin
        rx_done_tick <= 0;
        if (tick) begin
            state_reg <= state_next;
            counter <= counter + 1;
            read_next <= counter == 4'd7;
            if (read_next && state_reg == data) begin
                dout[bit_num] <= val;
                bit_num <= bit_num + 1;
            end
            if (read_next && state_reg == stop)
                rx_done_tick <= 1;
        end
    end

    always_comb begin
        state_next = state_reg;
        val = 0;
        if (read_next) begin
            case (state_reg)
                idle: begin
                    if (rx == 0)
                        state_next = data;
                end 
                start: begin
                    state_next = data;
                end
                data: begin
                    val = rx;
                    if (bit_num == 3'd7)
                        state_next = stop;
                end
                stop: begin
                    state_next = idle;
                end
            endcase
        end
    end

endmodule
