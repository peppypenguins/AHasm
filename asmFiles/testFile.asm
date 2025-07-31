         .ORIG 0x3000
         BRn LABEL
LABEL    ADD R1, R2, R2
         AND R2, R2, #0
         LEA R3, LABEL
	 LEA R4, LDLABEL
	 LEA R5, LDLABEL2
	 LDB R5, R4, #0
	 LDW R6, R4, #0
	 LDI R7, LDLABEL
	 STB R7, R4, #3
	 BR LABEL
         HALT
	 .BLKW #3		
LDLABEL  .STRINGZ Hello
LDLABEL2 .END