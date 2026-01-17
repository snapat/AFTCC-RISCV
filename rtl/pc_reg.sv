module pc_reg(
    input  logic        clock,
    input  logic        resetActiveLow,
    input  logic        enable,      
    input  logic [31:0] nextProgramCounter,
    output logic [31:0] programCounter
);
    always_ff @(posedge clock or negedge resetActiveLow) begin
        if (!resetActiveLow) 
            programCounter <= 32'b0;
        else if (enable)  
            programCounter <= nextProgramCounter;
    end
endmodule