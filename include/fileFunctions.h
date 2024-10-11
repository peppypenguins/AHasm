
#ifndef FILEFUNCTIONS_H
#define FILEFUNCTIONS_H
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include "ht.h"
#define MAX_LINE_LENGTH 255


	enum
	{
	   DONE, OK, EMPTY_LINE
	};

	enum {
ADD,
AND,
OR,
XOR,
LDB,
LDW,
LDI,
LEA,
STB,
STW,
STI,
BR,
BRN,
BRNZ,
BRNP,
BRNZP,
BRZP,
BRZ,
BRP,
JMP,
JSR,
JSRR,
RET,
RTI,
MUL,
DIV,
TRAP,
LSHF,
RSHFL,
RSHFA,
MOV,
ROT,
PUSH,
POP,
MACC,
EXTB,
EXTW,
HALT,
FILL,
BLKW,
STRINGZ,
END,
ORIG,
NUM_OPCODES,
};


void obtainFilePath(char* inputFile, char* outputFile, uint16_t maxsize);

int readAndParse( FILE* pInfile, char* pLine, char** pLabel, char
	** pOpcode, char** pArg1, char** pArg2, char** pArg3, char** pArg4
	);

int toNum(char* pStr);

bool isOpcode(const char* inputString);

int findOpcode(const char* inputString);

char toHexString(uint8_t input);


#endif