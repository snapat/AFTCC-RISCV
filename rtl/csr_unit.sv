module csr_unit (
    input  logic        clock,
    input  logic        resetActiveLow,
    
    // Commands from Controller
    input  logic        csrWriteEnable, // High when taking a Trap
    input  logic [31:0] pcFromCore,     // The PC to save (Current PC)
    
    // Output to PC Mux
    output logic [31:0] mepcValue       // The saved PC (Return Address)
);

    logic [31:0] mepc; // Machine Exception Program Counter

    always_ff @(posedge clock or negedge resetActiveLow) begin
        if (!resetActiveLow) begin
            mepc <= 32'b0;
        end else if (csrWriteEnable) begin
            // When taking a trap, save the current PC here
            mepc <= pcFromCore;
        end
    end

    // Continuous Read
    assign mepcValue = mepc;

endmodule