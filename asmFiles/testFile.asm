        .ORIG 0x3000
        BRn LABEL
LABEL   ADD R1, R2, R2
        AND R2, R2, #0
        LEA R3, LABEL
        HALT
        .BLKW #3
        .STRINGZ Hello
        .END