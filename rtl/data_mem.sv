module data_mem (
    input  logic        clock,
    input  logic [31:0] ramAxiWriteAddress, 
    input  logic [31:0] ramAxiWriteData,
    input  logic        ramAxiWriteValid, 
    input  logic [31:0] ramAxiReadAddress,
    output logic [31:0] ramAxiReadData
);
    logic [31:0] ramArray [0:1023];

    // Synchronous Write Logic
    always_ff @(posedge clock) begin
        if (ramAxiWriteValid) begin
            ramArray[ramAxiWriteAddress[11:2]] <= ramAxiWriteData;
        end
    end

    // Combinational Read Logic (Instant for Single-Cycle)
    assign ramAxiReadData = ramArray[ramAxiReadAddress[11:2]];

endmodule