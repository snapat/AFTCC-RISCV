module inst_mem (
    input  logic [31:0] romAxiReadAddress,
    output logic [31:0] romAxiReadData
);
    // 1024 words = 4KB (Small enough for LUTRAM to avoid block RAM timing issues)
    logic [31:0] romArray [0:1023];

    initial begin
        $readmemh("main.hex", romArray);
    end

    // Combinational Read (Instant)
    assign romAxiReadData = romArray[romAxiReadAddress[11:2]];
endmodule