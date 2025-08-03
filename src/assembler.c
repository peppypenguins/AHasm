#include "assembler.h"

#define MAX(x, y) ((x) > (y) ? (x) : (y))

void firstPass(ht* table, FILE** input);
void secondPass(ht* table, FILE** input, FILE** output);
int findOrig(FILE** input, char* origStart);
char* selectOpFunc(char* opCode, char* pArg1, char* pArg2, char* pArg3, char* pArg4,
FILE** output, ht* table, int* offset, int location);
char* add(char* pArg1, char* pArg2, char* pArg3);
char* and(char* pArg1, char* pArg2, char* pArg3);
char* or(char* pArg1, char* pArg2, char* pArg3);
char* xor(char* pArg1, char* pArg2, char* pArg3);
char* ldb(char* pArg1, char* pArg2, char* pArg3);
char* ldw(char* pArg1, char* pArg2, char* pArg3);
char* ldi(char* pArg1, char* pArg2, ht* table, int location);
char* ldib(char* pArg1, char* pArg2, ht* table, int location);
char* lea(char* pArg1, char* pArg2, ht* table, int location);
char* stb(char* pArg1, char* pArg2, char* pArg3);
char* stw(char* pArg1, char* pArg2, char* pArg3);
char* sti(char* pArg1, char* pArg2,ht* table, int location);
char* stib(char* pArg1, char* pArg2,ht* table, int location);
char* br(uint8_t brID, char* pArg1, ht* table, int location);
char* jmp(char* pArg1);
char* jsr(char* pArg1, ht* table, int location);
char* jsrr(char* pArg1);
char* ret();
char* rti();
char* mul(char* pArg1, char* pArg2, char* pArg3);
char* divide(char* pArg1, char* pArg2, char* pArg3);
char* trap(char* pArg1);
char* lshf(char* pArg1, char* pArg2, char* pArg3);
char* rshfl(char* pArg1, char* pArg2, char* pArg3);
char* rshfa(char* pArg1, char* pArg2, char* pArg3);
char* mov(char* pArg1, char* pArg2);
char* rot(char* pArg1, char* pArg2, char* pArg3);
char* push(char* pArg1);
char* pushb(char* pArg1);
char* pop(char* pArg1);
char* popb(char* pArg1);
char* macc(char* pArg1, char* pArg2, char* pArg3, char* pArg4);
char* extdb(char* pArg1, char* pArg2, char* pArg3);
char* extdw(char* pArg1, char* pArg2, char* pArg3);
char* blkw(char* pArg1, FILE** output, int* pOffset);
char* fill(char* pArg1);
char* stringz(char* pArg1, FILE** output, int* pOffset);

ht* label_table = NULL;

//Inner function for checking the given files are valid
void checkFiles(const char* inputFile, const char* outputFile, FILE** input, FILE** output){
    if (*input == NULL){
        // try without the true source
        char tempStr[255];
        tempStr[0] = '\0';
        strcat(tempStr, "asmFiles/");
        strcat(tempStr,inputFile);
        *input = fopen(tempStr, "r");
        if (*input == NULL){
            printf("Cannot find file name %s, terminating...", inputFile);
            exit(4);
        }
    }

    if (*output == NULL){
        // try without the true source
        char tempStr[255];
        tempStr[0] = '\0';
        strcat(tempStr, "asmFiles/");
        strcat(tempStr, outputFile);
        *output = fopen(tempStr, "r+");
        if (*output == NULL){
            printf("Cannot find file name %s, terminating...", outputFile);
            exit(4);
        }
    }
}


/*
Checks is a given label is valid.
*/
void checkLabel(char* label_str){
    if (strlen(label_str) > 20){
        printf("Invalid label %s, terminating...", label_str);
        ht_destroy(label_table);
        exit(4);
    }
    if (label_str[0] == 'x' || isdigit(label_str[0])){
        printf("Invalid label %s, terminating...", label_str);
        ht_destroy(label_table);
        exit(4);
    }
    if (strcmp(label_str, "in") == 0 || strcmp(label_str, "out") == 0){
        printf("Invalid label %s, terminating...", label_str);
        ht_destroy(label_table);
        exit(4);
    }
    if (strcmp(label_str, "getc") == 0 || strcmp(label_str, "puts") == 0){
        printf("Invalid label %s, terminating...", label_str);
        ht_destroy(label_table);
        exit(4);
    }

    for (int i = 0; i < strlen(label_str); ++i){
        if (isalnum(label_str[i]) == 0){
        printf("Invalid label %s, terminating...", label_str);
        ht_destroy(label_table);
        exit(4);
        }
    }
}

int add_label_increment(char* pOpcode, char* pArg1){
    int opcode = findOpcode(pOpcode);
    if (opcode == BLKW){
        int blkwrd_cnt = toNum(pArg1);
        return blkwrd_cnt * 2;
    } else if (opcode == STRINGZ){
        size_t str_len = strlen(pArg1);
        return (str_len + 1) & 0xFFFE;
    } else {
        return 2;
    }
}

/*
The main function of this file, this handles the actually assembly process
*/
void assemble(const char* inputFile,const char* outputFile){
    FILE *input = fopen(inputFile, "r");
    FILE *output = fopen(outputFile, "r+");

    checkFiles(inputFile, outputFile, &input, &output);

    label_table = ht_create();
    firstPass(label_table, &input);
    secondPass(label_table, &input, &output);
    ht_destroy(label_table);
    fclose(input);
    fclose(output);
}

/*
The first pass of the assembly process. This pass is used for collecting the labels into
a hash table and associating the labels with their address in memory. These will be used
in the second pass of the assembly process
*/
void firstPass(ht* table, FILE** input){

    int lret, offset = 0;
    char lLine[MAX_LINE_LENGTH+1];
    char *pLabel, *pOpcode, *pArg1, *pArg2, *pArg3, *pArg4;

    int orig = findOrig(input, NULL);
    do {
        lret = readAndParse(*input,lLine, &pLabel, &pOpcode, &pArg1, &pArg2, &pArg3, &pArg4);
        if (lret != DONE && lret != EMPTY_LINE){
            if (pLabel != NULL && pLabel[0] != '\0'){
                checkLabel(pLabel);
                if (ht_get(table,pLabel) == NULL){
                    int* value = (int*)malloc(sizeof(int) * 1);
                    *value = orig + offset;
                    ht_set(table,pLabel, value);
                } else {
                    printf("Multiple label instances (%s), terminating...", pLabel);
                    ht_destroy(label_table);
                    exit(4);
                }
            }
            offset += add_label_increment(pOpcode, pArg1);
        }
    } while(lret != DONE);
    rewind(*input);
}

/*
This function is used for finding the .orig opcode in the given file as this marks the start of 
the assembly process. If there is no .orig found then the program should terminate at that point
*/
int findOrig(FILE** input, char* origStart){
    int lret, itr = 0;
    char lLine[MAX_LINE_LENGTH + 1];
    char *pLabel, *pOpcode, *pArg1, *pArg2, *pArg3, *pArg4;

    do {
        lret = readAndParse(*input,lLine, &pLabel, &pOpcode, &pArg1, &pArg2, &pArg3, &pArg4);
        if (lret != DONE && lret != EMPTY_LINE){
            if (strcmp(pOpcode,".orig") == 0){
                if (origStart != NULL){
                    (void*)strcpy(origStart,pArg1);
                }

                return toNum(pArg1);
            }
        }

    } while(lret != DONE);

    printf("Did not find start of program, terminating...");
    ht_destroy(label_table);
    exit(4);
return -1;
}

/*
This handles the second pass of the assembly process. This is the
 pass where the majority of the work is done. Each line in the file corresponds to a
 single assembly instruction. First the specific opcode is determined and depending
 on the opcode we call a function corresponding to it that will return a string 
 that is the machine code string of that assembly instruction, this will be appended to
 the output file.
*/
void secondPass(ht* table, FILE** input, FILE** output){
char origStr[6];
int orig = findOrig(input,origStr);
fprintf(*output,"%s\n",origStr);

    int lret, offset = 0;
    char lLine[MAX_LINE_LENGTH+1];
    char *pLabel, *pOpcode, *pArg1, *pArg2, *pArg3, *pArg4;
    do {
        offset += 2;
        lret = readAndParse(*input,lLine, &pLabel, &pOpcode, &pArg1, &pArg2, &pArg3, &pArg4);
        if (lret != DONE && lret != EMPTY_LINE){
            char* outString = selectOpFunc(pOpcode, pArg1, pArg2, pArg3, pArg4, output, table, &offset, orig+offset);
            if (outString == NULL){
                if (strcmp(pOpcode, ".end") == 0){
                break;
                }
            } else {
                fprintf(*output,"%s\n", outString);
                free(outString);
            }
        }
    } while(lret != DONE);
}

/*
This file selects the specific opcode that we are working on and calls
its corresponding function to get the complete asm instruction
*/
char* selectOpFunc(char* opCode, char* pArg1,char* pArg2, char* pArg3, char* pArg4,
FILE** output, ht* table, int* offset, int location){

    switch(findOpcode(opCode)){
        case ADD: return add(pArg1, pArg2, pArg3);
            break;
        case AND: return and(pArg1, pArg2, pArg3);
            break;
        case OR: return or(pArg1, pArg2, pArg3);
            break;
        case XOR: return xor(pArg1, pArg2, pArg3);
            break;
        case LDB: return ldb(pArg1, pArg2, pArg3);
            break;
        case LDW: return ldw(pArg1, pArg2, pArg3);
            break;
        case LDI: return ldi(pArg1, pArg2, table,location);
            break;
        case LDIB: return ldib(pArg1, pArg2, table, location);
            break;
        case LEA: return lea(pArg1, pArg2, table, location);
            break;
        case STB: return stb(pArg1, pArg2, pArg3);
            break;
        case STW: return stw(pArg1, pArg2, pArg3);
            break;
        case STI: return sti(pArg1, pArg2, table, location);
            break;
        case STIB: return stib(pArg1, pArg2, table, location);
            break;
        case BR: return br(0, pArg1, table, location);
            break;
        case BRN: return br(4, pArg1, table, location);
            break;
        case BRNZ: return br(6, pArg1, table, location);
            break;
        case BRNP: return br(5, pArg1, table, location);
            break;
        case BRNZP: return br(7, pArg1, table, location);
            break;
        case BRZP: return br(3, pArg1, table, location);
            break;
        case BRZ: return br(2, pArg1, table, location);
            break;
        case BRP: return br(1, pArg1, table, location);
            break;
        case JMP: return jmp(pArg1);
            break;
        case JSR: return jsr(pArg1, table, location);
            break;
        case JSRR: return jsrr(pArg1);
            break;
        case RET: return ret();
            break;
        case RTI: return rti();
            break;
        case MUL: return mul(pArg1, pArg2, pArg3);
            break;
        case DIV: return divide(pArg1, pArg2, pArg3);
            break;
        case TRAP: return trap(pArg1);
            break;
        case LSHF: return lshf(pArg1, pArg2, pArg3);
            break;
        case RSHFL: return rshfl(pArg1, pArg2, pArg3);
            break;
        case RSHFA: return rshfa(pArg1, pArg2, pArg3);
            break;
        case MOV: return mov(pArg1, pArg2);
            break;
        case ROT: return rot(pArg1, pArg2, pArg3);
            break;
        case PUSH: return push(pArg1);
            break;
        case PUSHB: return pushb(pArg1);
            break;
        case POP: return pop(pArg1);
            break;
        case POPB: return popb(pArg1);
            break;
        case MACC: return macc(pArg1, pArg2, pArg3, pArg4);
            break;
        case EXTB: return extdb(pArg1, pArg2, pArg3);
            break;
        case EXTW: return extdw(pArg1, pArg2, pArg3);
            break;
        case HALT:char* tempStr = "x25";
                    return trap(tempStr);
            break;
        case FILL: return fill(pArg1);
            break;
        case BLKW: return blkw(pArg1, output, offset);
            break;
        case STRINGZ: return stringz(pArg1, output, offset);
            break;
        case END: return NULL;
            break;
        case NUM_OPCODES: printf("invalid opcode %s, terminating,,,", opCode);
                        ht_destroy(label_table);
                        exit(2);
            break;
        default: printf("invalid opcode %s, terminating...", opCode);
                ht_destroy(label_table);
                exit(2);
            break;
    }
}

/*
This function checks if a register argument is formatted properly. If not we terminate
as this is an ill formed input file
*/
void checkRegValid(char* pArg){
    if (pArg[0] != 'r'){
        printf("Invalid Register Argument %s, must be in the format 'r1', terminating...", pArg);
        ht_destroy(label_table);
        exit(3);
    }
    if (!isdigit(pArg[1])){
        printf("Invalid Register Argument %s, no digit in argument, terminating...", pArg);
        ht_destroy(label_table);
        exit(2);
    }
    if(pArg[1] - '0' > 7){
        printf("Invalid Register Argumnet %s, digit must be less than 8, terminating...", pArg);
        ht_destroy(label_table);
        exit(4);
    }
}

/*
 this function checks if a constant is within the correct range.
 Say that assembly instruction can have only an imm4 value but the
 number 100 is input, then this is invalid and the program will terminate
 */
void checkConstantValid(int constantValue, int maxValue, int minValue){
if (constantValue > maxValue){
    printf("Constant value greater than accepted %d, terminating...", constantValue);
    ht_destroy(label_table);
    exit(4);
}
if (constantValue < minValue){
    printf("Constant value less than accepted %d, terminating...", constantValue);
    ht_destroy(label_table);
    exit(4);
}
}

/*
The add function, this function handles the add opcode
*/
char* add(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x0000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);
    
    if (pArg3[0] == 'r'){ // reg version
        uint8_t dig2 = (pArg1[1] - '0') + 8;
        uint8_t dig3 = ((pArg2[1] - '0') << 1);
        uint8_t dig4 = ((pArg3[1] - '0'));

        strResult[3] = toHexString(dig2);
        strResult[4] = toHexString(dig3);
        strResult[5] = toHexString(dig4);
    } else { // imm version
        uint8_t dig2 = (pArg1[1] - '0') + 8;
        uint8_t dig3 = ((pArg2[1] - '0') << 1) + 1;
        uint8_t dig4 = toNum(pArg3);
        checkConstantValid(dig4, 7, -8);

        strResult[3] = toHexString(dig2);
        strResult[4] = toHexString(dig3);
        strResult[5] = toHexString((dig4) & 0xF);
    }

return strResult;
}

/*
The and function, this function handles the and opcode
*/
char* and(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x2000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);

    if (pArg3[0] == 'r'){
        uint8_t dig2 = (pArg1[1] - '0') + 0x08;
        uint8_t dig3 = ((pArg2[1] - '0') << 1);
        uint8_t dig4 = ((pArg3[1] - '0'));

        strResult[3] = toHexString(dig2);
        strResult[4] = toHexString(dig3);
        strResult[5] = toHexString(dig4);
    } else {
        uint8_t dig2 = (pArg1[1] - '0') + 0x08;
        uint8_t dig3 = ((pArg2[1] - '0') << 1) + 1;
        uint8_t dig4 = toNum(pArg3);
        checkConstantValid(dig4, 7, -8);

        strResult[3] = toHexString(dig2);
        strResult[4] = toHexString(dig3);
        strResult[5] = toHexString((dig4) & 0xF);
    }
return strResult;
}

/*
The or function, this function handles the or opcode
*/
char* or(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x5000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);

    if (pArg3[0] == 'r'){
        uint8_t dig2 = (pArg1[1] - '0');
        uint8_t dig3 = ((pArg2[1] - '0') << 1);
        uint8_t dig4 = ((pArg3[1] - '0'));

        strResult[3] = toHexString(dig2);
        strResult[4] = toHexString(dig3);
        strResult[5] = toHexString(dig4);
    } else {
        uint8_t dig2 = (pArg1[1] - '0');
        uint8_t dig3 = ((pArg2[1] - '0') << 1) + 1;
        uint8_t dig4 = toNum(pArg3);
        checkConstantValid(dig4, 7, -8);

        strResult[3] = toHexString(dig2);
        strResult[4] = toHexString(dig3);
        strResult[5] = toHexString((dig4) & 0xF);
    }

return strResult;
}

/*
The xor function, this function handles the xor opcode
*/
char* xor(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x4000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);

    if (pArg3[0] == 'r'){
        uint8_t dig2 = (pArg1[1] - '0') + 8;
        uint8_t dig3 = ((pArg2[1] - '0') << 1);
        uint8_t dig4 = ((pArg3[1] - '0'));

        strResult[3] = toHexString(dig2);
        strResult[4] = toHexString(dig3);
        strResult[5] = toHexString(dig4);
    } else {
        uint8_t dig2 = (pArg1[1] - '0') + 8;
        uint8_t dig3 = ((pArg2[1] - '0') << 1) + 1;
        uint8_t dig4 = toNum(pArg3);
        checkConstantValid(dig4, 7, -8);

        strResult[3] = toHexString(dig2);
        strResult[4] = toHexString(dig3);
        strResult[5] = toHexString((dig4) & 0xF);
    }

return strResult;
}

/*
The ldb function, this function handles the ldb opcode
*/
char* ldb(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x1000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);

    uint8_t dig4 = toNum(pArg3);
    checkConstantValid(dig4, 31, -32);
    uint8_t dig2 = (pArg1[1] - '0');
    uint8_t dig3 = ((pArg2[1] - '0') << 1) + (((uint8_t)(dig4)) >> 4);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(dig4 & 0x0F);

    return strResult;
}

/*
The ldw function, this function handles the ldw opcode
*/
char* ldw(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x3000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);

    uint8_t dig4 = toNum(pArg3);
    checkConstantValid(dig4, 31, -32);
    uint8_t dig2 = (pArg1[1] - '0');
    uint8_t dig3 = ((pArg2[1] - '0') << 1) + (((uint8_t)(dig4)) >> 4);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(dig4 & 0x0F);

return strResult;
}

/*
The ldi function, this function handles the ldi opcode
*/
char* ldi(char* pArg1, char* pArg2, ht* table, int location){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x8000");

    checkRegValid(pArg1);
    uint8_t dig2 = pArg1[1] - '0';
    int16_t* labelVal = ((int16_t*)ht_get(table, pArg2));
    if (labelVal == NULL){
        printf("Label %s not found, terminating...", pArg2);
        ht_destroy(table);
        exit(3);
    }
    
    uint16_t offset = (*labelVal - (location)) / 2;
    checkConstantValid(offset,127, -128);
    uint8_t dig3 = (uint8_t)((offset >> 4) & 0xF);
    uint8_t dig4 = (uint8_t)(offset & 0x0F);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(dig4);

return strResult;
}

/*
The ldi function, this function handles the ldib opcode
*/
char* ldib(char* pArg1, char* pArg2, ht* table, int location){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0xD800");

    checkRegValid(pArg1);
    uint8_t dig2 = pArg1[1] - '0';
    int16_t* labelVal = ((int16_t*)ht_get(table, pArg2));
    if (labelVal == NULL){
        printf("Label %s not found, terminating...", pArg2);
        ht_destroy(table);
        exit(3);
    }
    
    uint16_t offset = (*labelVal - (location)) / 2;
    checkConstantValid(offset,127, -128);
    uint8_t dig3 = (uint8_t)((offset >> 4) & 0xF);
    uint8_t dig4 = (uint8_t)(offset & 0x0F);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(dig4);

return strResult;
}

/*
The lea function, this function handles the lea opcode
*/
char* lea(char* pArg1, char* pArg2, ht* table, int location){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x7000");
    checkRegValid(pArg1);
    uint8_t dig2 = pArg1[1] - '0';
    int16_t* labelVal = ((int16_t*)ht_get(table, pArg2));
    if (labelVal == NULL){
        printf("Label %s not found, terminating...", pArg2);
        ht_destroy(table);
        exit(3);
    }

    int16_t offset = (*labelVal - (location)) / 2;
    checkConstantValid(offset,127, -128);
    uint8_t dig3 = (uint8_t)((offset >> 4) & 0xF);
    uint8_t dig4 = (uint8_t)(offset & 0x0F);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(dig4);

return strResult;
}

/*
The stb function, this function handles the stb opcode
*/
char* stb(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x1000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);

    uint8_t dig4 = toNum(pArg3);
    uint8_t dig2 = (pArg1[1] - '0') + 8;
    uint8_t dig3 = ((pArg2[1] - '0') << 1) + (((uint8_t)(dig4)) >> 4);
    checkConstantValid(dig4, 7, -8);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(dig4 & 0x0F);


return strResult;
}

/*
The stw function, this function handles the stw opcode
*/
char* stw(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x3000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);

    uint8_t dig4 = toNum(pArg3);
    uint8_t dig2 = (pArg1[1] - '0') + 8;
    int8_t dig3 = ((pArg2[1] - '0') << 1) + (((uint8_t)(dig4)) >> 4);
    checkConstantValid(dig4, 7, -8);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(dig4 & 0x0F);


return strResult;
}

/*
The sti function, this function handles the sti opcode
*/
char* sti(char* pArg1, char* pArg2, ht* table, int location){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x8000");

    checkRegValid(pArg1);
    uint8_t dig2 = (pArg1[1] - '0') + 8;
    int16_t* labelVal = ((int16_t*)ht_get(table, pArg2));
    if (labelVal == NULL){
        printf("Label %s not found, terminating...", pArg2);
        ht_destroy(table);
        exit(3);
    }
    int16_t offset = (*labelVal - (location)) / 2;
    checkConstantValid(offset,127, -128);
    uint8_t dig3 = (uint8_t)((offset >> 4) & 0xF);
    uint8_t dig4 = (uint8_t)(offset & 0x0F);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(dig4);

return strResult;
}

/*
The stib function, this function handles the stib opcode
*/
char* stib(char* pArg1, char* pArg2, ht* table, int location){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0xE000");

    checkRegValid(pArg1);
    uint8_t dig2 = (pArg1[1] - '0') + 8;
    int16_t* labelVal = ((int16_t*)ht_get(table, pArg2));
    if (labelVal == NULL){
        printf("Label %s not found, terminating...", pArg2);
        ht_destroy(table);
        exit(3);
    }
    int16_t offset = (*labelVal - (location)) / 2;
    checkConstantValid(offset,127, -128);
    uint8_t dig3 = (uint8_t)((offset >> 4) & 0xF);
    uint8_t dig4 = (uint8_t)(offset & 0x0F);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(dig4);

return strResult;
}

/*
The br function, this function handles the br opcode
*/
char* br(uint8_t brID, char* pArg1, ht* table, int location){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x0000");
    int16_t* labelVal = ((int16_t*)ht_get(table, pArg1));
    if (labelVal == NULL){
        printf("Label %s not found, terminating...", pArg1);
        ht_destroy(table);
        exit(3);
    }
    int16_t offset = (*labelVal - (location)) / 2;
    checkConstantValid(offset,127, -128);
    uint8_t dig2 = brID;
    uint8_t dig3 = (uint8_t)((offset >> 4) & 0xF);
    uint8_t dig4 = (uint8_t)(offset & 0x0F);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(dig4);

return strResult;
}

/*
The jmp function, this function handles the jmp opcode
*/
char* jmp(char* pArg1){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x6000");
    checkRegValid(pArg1);
    uint8_t dig3 = (pArg1[1] - '0') << 1;
    strResult[4] = toHexString(dig3);
return strResult;
}


char* jsr(char* pArg1, ht* table, int location){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x2000");
    int16_t* labelVal = ((int16_t*)ht_get(table, pArg1));
    if (labelVal == NULL){
        printf("Label %s not found, terminating...", pArg1);
        ht_destroy(table);
        exit(3);
    }
    int16_t offset = (*labelVal - (location)) / 2;
    checkConstantValid(offset,1023, -1024);

    uint8_t dig2 = (offset >> 7) + 8;
    uint8_t dig3 = (offset >> 4) & 0xF;
    uint8_t dig4 = (offset & 0xF);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(dig4);


return strResult;
}

/*
The jsrr function, this function handles the jsrr opcode
*/
char* jsrr(char* pArg1){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x2000");
    checkRegValid(pArg1);
    uint8_t dig3 = (pArg1[1] - '0') << 1;
    strResult[4] = toHexString(dig3);
return strResult;
}


/*
The ret function, this function handles the ret opcode
*/
char* ret(){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x60E0");
return strResult;
}

/*
The rti function, this function handles the rti opcode
*/
char* rti(){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x4000");
return strResult;
}

/*
The mul function, this function handles the mul opcode
*/
char* mul(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x9000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);
    
    if (pArg3[0] == 'r'){ // reg version
        uint8_t dig2 = (pArg1[1] - '0');
        uint8_t dig3 = ((pArg2[1] - '0') << 1);
        uint8_t dig4 = ((pArg3[1] - '0'));

        strResult[3] = toHexString(dig2);
        strResult[4] = toHexString(dig3);
        strResult[5] = toHexString(dig4);
    } else { // imm version
        uint8_t dig2 = (pArg1[1] - '0');
        uint8_t dig3 = ((pArg2[1] - '0') << 1) + 1;
        int8_t dig4 = toNum(pArg3);
        checkConstantValid(dig4, 7, -8);

        strResult[3] = toHexString(dig2);
        strResult[4] = toHexString(dig3);
        strResult[5] = toHexString(dig4 & 0xF);
    }

return strResult;
}

/*
The divide function, this function handles the divide opcode
*/

char* divide(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x9000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);
    
    if (pArg3[0] == 'r'){ // reg version
        uint8_t dig2 = (pArg1[1] - '0') + 8;
        uint8_t dig3 = ((pArg2[1] - '0') << 1);
        uint8_t dig4 = ((pArg3[1] - '0'));

        strResult[3] = toHexString(dig2);
        strResult[4] = toHexString(dig3);
        strResult[5] = toHexString(dig4);
    } else { // imm version
        uint8_t dig2 = (pArg1[1] - '0') + 8;
        uint8_t dig3 = ((pArg2[1] - '0') << 1) + 1;
        int8_t dig4 = toNum(pArg3);
        checkConstantValid(dig4, 7, -8);

        strResult[3] = toHexString(dig2);
        strResult[4] = toHexString(dig3);
        strResult[5] = toHexString(dig4 & 0xF);
    }

return strResult;
}

/*
The trap function, this function handles the trap opcode
*/
char* trap(char* pArg1){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x7800");

    int8_t dig34 = toNum(pArg1);
    checkConstantValid(dig34,127, -128);
    uint8_t dig3 = dig34 >> 4;
    uint8_t dig4 = dig34 & 0xF; 

    strResult[3] = toHexString(dig3);
    strResult[4] = toHexString(dig4);
return strResult;
}

/*
The left shift function, this function handles the left shift opcode
*/
char* lshf(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x6000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);
    

        uint8_t dig2 = (pArg1[1] - '0') + 8;
        uint8_t dig3 = ((pArg2[1] - '0') << 1);
        int8_t dig4 = toNum(pArg3);
        checkConstantValid(dig4, 7, 0);

        strResult[3] = toHexString(dig2);
        strResult[4] = toHexString(dig3);
        strResult[5] = toHexString(dig4);

return strResult;
}



/*
The logical right shift function. This function handles the logical right shift opcode
*/
char* rshfl(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x6000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);
    

    uint8_t dig2 = (pArg1[1] - '0') + 8;
    uint8_t dig3 = ((pArg2[1] - '0') << 1);
    int8_t dig4 = toNum(pArg3);
    checkConstantValid(dig4, 7, 0);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(dig4+8);

return strResult;
}

/*
the arithmetic right shift function. This function handles the arithmetic right shift opcode
*/
char* rshfa(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0x6000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);
    

    uint8_t dig2 = (pArg1[1] - '0') + 8;
    uint8_t dig3 = ((pArg2[1] - '0') << 1)+1;
    int8_t dig4 = toNum(pArg3);
    checkConstantValid(dig4, 7, 0);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(dig4+8);

return strResult;
}

/*
The move function. This function handles the move opcode.
*/
char* mov(char* pArg1, char* pArg2){
   char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0xA000");
    checkRegValid(pArg1);
    checkRegValid(pArg2);

    if (pArg2[0] == 'r'){ // reg version
        uint8_t dig2 = (pArg1[1] - '0');
        uint8_t dig4 = (pArg2[1] - '0');
        strResult[3] = toHexString(dig2);
        strResult[5] = toHexString(dig4);
    } else { // imm version
        uint8_t dig2 = (pArg1[1] - '0');
        uint16_t imm = toNum(pArg2);
        checkConstantValid(imm, 63, -64);
        uint8_t dig3 = ((imm & 0x70) >> 4) + 1;
        uint8_t dig4 = (imm & 0x0F);

        strResult[3] = toHexString(dig2);
        strResult[4] = toHexString(dig3 & 0x0F);
        strResult[5] = toHexString(dig4 & 0x0F);
    }
return strResult;
}

/*
The rotate instruction. This function handles the rotate opcode
*/
char* rot(char* pArg1, char* pArg2, char* pArg3){
   char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0xA000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);
    uint8_t amt = toNum(pArg3);
    checkConstantValid(amt,31, 0);

    uint8_t dig2 = (pArg1[1] - '0') + 8;
    uint8_t dig3 = (pArg2[1] - '0') + (amt >> 4);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(amt & 0xF);


return strResult;
}

/*
The push instruction. This function handles the stack push opcode.
*/
char* push(char* pArg1){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0xB000");

    checkRegValid(pArg1);
    uint8_t dig2 = (pArg1[1] - '0');
    strResult[3] = toHexString(dig2);

return strResult;
}

/*
The pushb instruction. This function handles the stack push byte opcode.
*/
char* pushb(char* pArg1){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0xB010");

    checkRegValid(pArg1);
    uint8_t dig2 = (pArg1[1] - '0');
    strResult[3] = toHexString(dig2);

return strResult;
}

/*
The pop function. This function handles the pop stack opcode
*/
char* pop(char* pArg1){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0xB000");

    checkRegValid(pArg1);
    uint8_t dig2 = (pArg1[1] - '0') + 8;
    strResult[3] = toHexString(dig2);

return strResult;
}

/*
The popb function. This function handles the pop byte stack opcode
*/
char* popb(char* pArg1){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0xB010");

    checkRegValid(pArg1);
    uint8_t dig2 = (pArg1[1] - '0') + 8;
    strResult[3] = toHexString(dig2);

return strResult;
}

/*
The multiply accumulate function. This functionh handles the macc opcode
*/
char* macc(char* pArg1, char* pArg2, char* pArg3, char* pArg4){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0xC000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);
    checkRegValid(pArg3);

    uint8_t arg1 = (pArg1[1] - '0');
    uint8_t arg2 = (pArg2[1] - '0');
    uint8_t arg3 = (pArg3[1] - '0');

    uint8_t dig2 = arg1;
    uint8_t dig3 = (arg2 << 1) + (arg3 >> 2);
    uint8_t dig4 = toNum(pArg4);
    checkConstantValid(dig4, 3, 0);

    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(dig4 + ((arg3 << 2)&0xF));

return strResult;
}

/*
The extend byte function. This function handles the extdb instruction
*/
char* extdb(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0xC000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);

    uint8_t imm = toNum(pArg3);
    checkConstantValid(imm, 15, 0);

    uint8_t dig2 = (pArg1[1] - '0') + 8;
    uint8_t dig3 = (pArg2[1] - '0') << 1;
    
    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(imm);

return strResult;
}

/*
The extend word function. This function handles the extdw opcode.
*/
char* extdw(char* pArg1, char* pArg2, char* pArg3){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, "0xD000");

    checkRegValid(pArg1);
    checkRegValid(pArg2);

    uint8_t imm = toNum(pArg3);
    checkConstantValid(imm, 7, 0);

    uint8_t dig2 = (pArg1[1] - '0');
    uint8_t dig3 = (pArg2[1] - '0') << 1;
    
    strResult[3] = toHexString(dig2);
    strResult[4] = toHexString(dig3);
    strResult[5] = toHexString(imm);

return strResult;
}

/*
The block word pseudo op function. This function handles the pseudo opcode .blkw.
*/
char* blkw(char* pArg1, FILE** output, int* pOffset){
uint16_t numWords = toNum(pArg1);

for (int i = 0; i < numWords; ++i){
    fprintf(*output, "%s\n", "0x0000");
    *pOffset += 2;
}

return NULL;
}

/**
 *The fill pseudo op function. This function handles the pseudo opcode .fill
 * 
 */
char* fill(char* pArg1){
    char* strResult = (char*)malloc((sizeof(char) * 7));
    strcpy(strResult, pArg1);
return strResult;
}

/*
The stringz pseudo op function. This function handles the pseudo opcode .stringz
*/
char* stringz(char* pArg1, FILE** output, int* pOffset){
    uint16_t itr = 0;

    while (pArg1[itr] != 0){
        char tmpStr[] = "0x0000";
        char firstChar = pArg1[itr];
        uint8_t dig4 = firstChar & 0xF;
        uint8_t dig3 = firstChar >> 4;
        tmpStr[4] = toHexString(dig3);
        tmpStr[5] = toHexString(dig4);
        ++itr;
        if (pArg1[itr] != 0){
            char secondChar = pArg1[itr];
            uint8_t dig2 = secondChar & 0xF;
            uint8_t dig1 = secondChar >> 4;
            tmpStr[3] = toHexString(dig2);
            tmpStr[2] = toHexString(dig1);
            ++itr;
        }
        fprintf(*output, "%s\n", tmpStr);
        *pOffset += 2;
        }
    return NULL;
}