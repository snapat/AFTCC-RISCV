module soc_top(
    input  logic       clock,          
    input  logic       resetActiveLow, 
    output logic [7:0] debugLeds,      
    output logic       uartTransmit    
);

    // --- 0. CLOCK DIVIDER ---
    logic [2:0] clockDivider;
    logic       cpuClock;
    always_ff @(posedge clock) clockDivider <= clockDivider + 1;
    assign cpuClock = clockDivider[2];

    // --- INTERNAL SIGNALS ---
    logic [31:0] programCounter, nextProgramCounter, instruction, immediateValue;
    logic [31:0] readData1, readData2, aluResult, busReadData, ramReadData;
    logic [31:0] ramAddress, ramWriteData;
    logic [31:0] mepcValue;
    logic        registerWriteEnable, memoryWriteEnable, aluInputSource, resultSource, isBranch, zeroFlag;
    logic        ramWriteValid, csrWriteEnable, isTrap;
    logic [2:0]  aluControl;

    // --- PREEMPTION HARDWARE ---
    logic [31:0] timerCount;
    logic        timerInterrupt;
    localparam   TIMER_LIMIT = 50; 

    always_ff @(posedge cpuClock or negedge resetActiveLow) begin
        if (!resetActiveLow) begin
            timerCount <= 0;
            timerInterrupt <= 0;
        end else begin
            if (timerCount >= TIMER_LIMIT) begin
                timerCount <= 0;
                timerInterrupt <= 1; 
            end else begin
                timerCount <= timerCount + 1;
                timerInterrupt <= 0;
            end
        end
    end

    // --- 1. PC LOGIC ---
    assign nextProgramCounter = isTrap ? 32'h00000010 : 
                               (isBranch & zeroFlag) ? (programCounter + immediateValue) : 
                               (programCounter + 4);
    
    pc_reg u_pc (
        .clock(cpuClock), .resetActiveLow(resetActiveLow), .enable(1'b1), 
        .nextProgramCounter(nextProgramCounter), .programCounter(programCounter)
    );

    // --- 2. CORE COMPONENTS ---
    inst_mem u_rom (
        .romAxiReadAddress(programCounter), .romAxiReadData(instruction)
    );

    controller u_ctrl (
        .opcode(instruction[6:0]), .funct3(instruction[14:12]), .funct7(instruction[31:25]),
        .timerInterrupt(timerInterrupt), .registerWriteEnable(registerWriteEnable), 
        .aluInputSource(aluInputSource), .memoryWriteEnable(memoryWriteEnable), 
        .resultSource(resultSource), .isBranch(isBranch), 
        .aluControlSignal(aluControl), .csrWriteEnable(csrWriteEnable), .isTrap(isTrap)
    );

    imm_gen u_imm_gen (.instruction(instruction), .immediateValue(immediateValue));

    csr_unit u_csr (
        .clock(cpuClock), .resetActiveLow(resetActiveLow),
        .csrWriteEnable(csrWriteEnable), .pcFromCore(programCounter), .mepcValue(mepcValue)
    );

    regfile u_rf (
        .clock(cpuClock), .registerWriteEnable(registerWriteEnable),
        .readAddress0(instruction[19:15]), .readAddress1(instruction[24:20]), 
        .writeAddress(instruction[11:7]), 
        .writeData(resultSource ? busReadData : aluResult), 
        .readData0(readData1), .readData1(readData2)
    );

    alu u_alu (
        .inputA(readData1), .inputB(aluInputSource ? immediateValue : readData2),
        .aluControl(aluControl), .aluResult(aluResult), .zero(zeroFlag)
    );

    // --- 3. THE BUS (Full Pin Mapping) ---
    bus_interconnect u_bus (
        .clock(cpuClock), .resetActiveLow(resetActiveLow),
        
        // CPU MASTER
        .cpuAxiWriteAddress(aluResult), .cpuAxiWriteData(readData2), .cpuAxiWriteValid(memoryWriteEnable),
        .cpuAxiWriteReady(), .cpuAxiWriteValidData(1'b1), .cpuAxiWriteReadyData(),
        .cpuAxiReadAddress(aluResult), .cpuAxiReadValid(resultSource), .cpuAxiReadReady(),
        .cpuAxiReadData(busReadData), .cpuAxiReadValidData(), .cpuAxiReadReadyData(1'b1),

        // DMA MASTER (All pins mapped & tied)
        .dmaAxiWriteAddress(32'b0), .dmaAxiWriteValid(1'b0), .dmaAxiWriteReady(),
        .dmaAxiWriteData(32'b0), .dmaAxiWriteValidData(1'b0), .dmaAxiWriteReadyData(),
        .dmaAxiReadAddress(32'b0), .dmaAxiReadValid(1'b0), .dmaAxiReadReady(),
        .dmaAxiReadData(), .dmaAxiReadValidData(), .dmaAxiReadReadyData(1'b1),

        // SLAVE: ROM (All pins mapped & tied)
        .romAxiReadAddress(), .romAxiReadValid(), .romAxiReadReady(1'b1),
        .romAxiReadData(32'b0), .romAxiReadValidData(1'b1), .romAxiReadReadyData(),

        // SLAVE: RAM
        .ramAxiWriteAddress(ramAddress), .ramAxiWriteValid(ramWriteValid), .ramAxiWriteReady(1'b1),
        .ramAxiWriteData(ramWriteData), .ramAxiWriteValidData(), .ramAxiWriteReadyData(1'b1),
        .ramAxiReadAddress(ramAddress), .ramAxiReadValid(), .ramAxiReadReady(1'b1),
        .ramAxiReadData(ramReadData), .ramAxiReadValidData(1'b1), .ramAxiReadReadyData(),

        // SLAVE: IO (All pins mapped & tied)
        .ioAxiWriteAddress(), .ioAxiWriteValid(), .ioAxiWriteReady(1'b1),
        .ioAxiWriteData(), .ioAxiWriteValidData(), .ioAxiWriteReadyData(1'b1),
        .ioAxiReadAddress(), .ioAxiReadValid(), .ioAxiReadReady(1'b1),
        .ioAxiReadData(32'b0), .ioAxiReadValidData(1'b1), .ioAxiReadReadyData()
    );

    // --- 4. DATA MEMORY ---
    data_mem u_ram (
        .clock(cpuClock), .ramAxiWriteAddress(ramAddress), .ramAxiWriteData(ramWriteData),
        .ramAxiWriteValid(ramWriteValid), .ramAxiReadAddress(ramAddress), .ramAxiReadData(ramReadData)
    );

    assign debugLeds = programCounter[9:2];
    assign uartTransmit = 1'b1;

endmodule