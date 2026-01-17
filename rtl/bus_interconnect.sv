module bus_interconnect (
    input  logic        clock,
    input  logic        resetActiveLow,

    // CPU MASTER
    input  logic [31:0] cpuAxiWriteAddress, input  logic cpuAxiWriteValid, output logic cpuAxiWriteReady,
    input  logic [31:0] cpuAxiWriteData,    input  logic cpuAxiWriteValidData, output logic cpuAxiWriteReadyData,
    input  logic [31:0] cpuAxiReadAddress,  input  logic cpuAxiReadValid, output logic cpuAxiReadReady,
    output logic [31:0] cpuAxiReadData,     output logic cpuAxiReadValidData, input  logic cpuAxiReadReadyData,

    // DMA MASTER
    input  logic [31:0] dmaAxiWriteAddress, input  logic dmaAxiWriteValid, output logic dmaAxiWriteReady,
    input  logic [31:0] dmaAxiWriteData,    input  logic dmaAxiWriteValidData, output logic dmaAxiWriteReadyData,
    input  logic [31:0] dmaAxiReadAddress,  input  logic dmaAxiReadValid,  output logic dmaAxiReadReady,
    output logic [31:0] dmaAxiReadData,     output logic dmaAxiReadValidData, input  logic dmaAxiReadReadyData,

    // SLAVES
    output logic [31:0] romAxiReadAddress,  output logic romAxiReadValid,    input logic romAxiReadReady,
    input  logic [31:0] romAxiReadData,     input  logic romAxiReadValidData, output logic romAxiReadReadyData,

    output logic [31:0] ramAxiWriteAddress, output logic ramAxiWriteValid,     input logic ramAxiWriteReady,
    output logic [31:0] ramAxiWriteData,    output logic ramAxiWriteValidData, input logic ramAxiWriteReadyData,
    output logic [31:0] ramAxiReadAddress,  output logic ramAxiReadValid,      input logic ramAxiReadReady,
    input  logic [31:0] ramAxiReadData,     input  logic ramAxiReadValidData, output logic ramAxiReadReadyData,

    output logic [31:0] ioAxiWriteAddress,  output logic ioAxiWriteValid,      input logic ioAxiWriteReady,
    output logic [31:0] ioAxiWriteData,     output logic ioAxiWriteValidData, input logic ioAxiWriteReadyData,
    output logic [31:0] ioAxiReadAddress,   output logic ioAxiReadValid,      input logic ioAxiReadReady,
    input  logic [31:0] ioAxiReadData,      input  logic ioAxiReadValidData, output logic ioAxiReadReadyData
);

    logic activeMaster; 
    logic [31:0] currAddr_R, currAddr_W, currData_W;
    logic        currValid_R, currValid_W, currValidData_W;

    always_ff @(posedge clock or negedge resetActiveLow) begin
        if (!resetActiveLow) activeMaster <= 1'b0;
        else if (activeMaster == 0 && (dmaAxiReadValid || dmaAxiWriteValid)) activeMaster <= 1'b1;
        else if (activeMaster == 1 && (!dmaAxiReadValid && !dmaAxiWriteValid)) activeMaster <= 1'b0;
    end

    always_comb begin
        // --- 1. DEFAULT HANDSHAKE TIES (FIXES PORTSHORT) ---
        cpuAxiWriteReady = 1'b1; 
        cpuAxiWriteReadyData = 1'b1;
        cpuAxiReadReady  = 1'b1; 
        cpuAxiReadValidData  = 1'b1;
        dmaAxiWriteReady = 1'b1; 
        dmaAxiWriteReadyData = 1'b1;
        dmaAxiReadReady  = 1'b1; 
        dmaAxiReadValidData  = 1'b1;

        romAxiReadReadyData  = 1'b1;
        ramAxiWriteValidData = 1'b1; 
        ramAxiReadReadyData = 1'b1;
        ioAxiWriteValidData  = 1'b1; 
        ioAxiReadReadyData  = 1'b1;

        // --- 2. ADDRESS/DATA DEFAULTS ---
        ramAxiWriteValid = 0; ioAxiWriteValid = 0; romAxiReadValid = 0;
        ramAxiReadValid  = 0; ioAxiReadValid  = 0;
        cpuAxiReadData = 0;   dmaAxiReadData = 0;
        
        romAxiReadAddress = 0; ramAxiWriteAddress = 0; ramAxiWriteData = 0;
        ramAxiReadAddress = 0; ioAxiWriteAddress  = 0; ioAxiWriteData = 0;
        ioAxiReadAddress  = 0;

        // --- 3. MASTER MUX ---
        if (activeMaster == 0) begin
            currAddr_R = cpuAxiReadAddress;  currValid_R = cpuAxiReadValid;
            currAddr_W = cpuAxiWriteAddress; currValid_W = cpuAxiWriteValid;
            currData_W = cpuAxiWriteData;    currValidData_W = cpuAxiWriteValidData;
        end else begin
            currAddr_R = dmaAxiReadAddress;  currValid_R = dmaAxiReadValid;
            currAddr_W = dmaAxiWriteAddress; currValid_W = dmaAxiWriteValid;
            currData_W = dmaAxiWriteData;    currValidData_W = dmaAxiWriteValidData;
        end

        // --- 4. SLAVE DEMUX ---
        if (currValid_R) begin
            if (currAddr_R[30]) begin 
                ioAxiReadAddress = currAddr_R; ioAxiReadValid = 1;
                if (!activeMaster) cpuAxiReadData = ioAxiReadData; else dmaAxiReadData = ioAxiReadData;
            end else if (currAddr_R[29]) begin 
                ramAxiReadAddress = currAddr_R; ramAxiReadValid = 1;
                if (!activeMaster) cpuAxiReadData = ramAxiReadData; else dmaAxiReadData = ramAxiReadData;
            end
        end

        if (currValid_W) begin
            if (currAddr_W[30]) begin 
                ioAxiWriteAddress = currAddr_W; ioAxiWriteValid = 1;
                ioAxiWriteData = currData_W; 
            end else begin 
                ramAxiWriteAddress = currAddr_W; ramAxiWriteValid = 1;
                ramAxiWriteData = currData_W;
            end
        end
    end
endmodule