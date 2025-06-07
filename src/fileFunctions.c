#include "fileFunctions.h"
#include <limits.h>

extern ht* label_table; 

/*
Basic funcion used to get the file path from the user, just prompts
and waits for input 2 times. One for input file, one for ouput file.
 */
void obtainFilePath(char* inputFile, char* outputFile, uint16_t maxSize){
    printf("Enter the input file path:");
    
    if (fgets(inputFile, maxSize,stdin) != NULL){
        size_t len = strlen(inputFile);
        if (len > 0 && inputFile[len-1] == '\n'){
            inputFile[len-1] = '\0';
        }
    }
    printf("Enter the output file path:");
    
    if (fgets(outputFile, maxSize,stdin) != NULL){
        size_t len = strlen(outputFile);
        if (len > 0 && outputFile[len-1] == '\n'){
            outputFile[len-1] = '\0';
        }
    }
}

/*
Simply checks if the given is a valid opcode. if so return truw, otherwise return false
 */
bool isOpcode(const char* inputString){
    char *opCodes[NUM_OPCODES] = {"add", "and", "or", "xor", "ldb", "ldw", "ldi", "ldib", "lea",
    "stb", "stw", "sti", "stib", "br", "brn", "brnz", "brnp", "brnzp", "brzp", "brz", "brp", "jmp",
    "jsr", "jsrr", "ret", "rti", "mul", "div", "trap", "lshf", "rshfl", "rshfa", "mov", "rot",
    "push", "pushb", "pop", "popb", "macc", "extdb", "extdw", "halt", ".fill", ".blkw", ".stringz", ".end", ".orig"};

    for (int i = 0; i < NUM_OPCODES; i++){
        if (strcmp(inputString, opCodes[i]) == 0){
            return true;
        }
    }

    return false;
}

/*
simple function that checks which case the opcode was. Returns that index in the array.
 */
int findOpcode(const char* inputString){
  char *opCodes[NUM_OPCODES] = {"add", "and", "or", "xor", "ldb", "ldw", "ldi", "ldib", "lea",
    "stb", "stw", "sti", "stib", "br", "brn", "brnz", "brnp", "brnzp", "brzp", "brz", "brp", "jmp",
    "jsr", "jsrr", "ret", "rti", "mul", "div", "trap", "lshf", "rshfl", "rshfa", "mov", "rot",
    "push", "pushb", "pop", "popb", "macc", "extdb", "extdw", "halt",".fill", ".blkw", ".stringz", ".end", ".orig"};
  int i = 0;
    for (i; i < NUM_OPCODES; ++i){
        if (strcmp(inputString, opCodes[i]) == 0){
            break;
        }
    }

    return i;
}

/*
Converts a user given string representing a number, either in format:
#3 or x3 into an integer value. If formatted incorrectly terminates the 
program
*/
int toNum( char* pStr )
{
   char * t_ptr;
   char * orig_pStr;
   int t_length,k;
   int lNum, lNeg = 0;
   long int lNumLong;

   orig_pStr = pStr;
   if (*pStr == '0'){
    pStr++;
   }
   if( *pStr == '#' )				/* decimal */
   { 
     pStr++;
     if( *pStr == '-' )				/* dec is negative */
     {
       lNeg = 1;
       pStr++;
     }
     t_ptr = pStr;
     t_length = strlen(t_ptr);
     for(k=0;k < t_length;k++)
     {
       if (!isdigit(*t_ptr))
       {
	 printf("Error: invalid decimal operand, %s\n",orig_pStr);
   ht_destroy(label_table);
	 exit(4);
       }
       t_ptr++;
     }
     lNum = atoi(pStr);
     if (lNeg)
       lNum = -lNum;
 
     return lNum;
   }
   else if( *pStr == 'x' )	/* hex     */
   {
     pStr++;
     if( *pStr == '-' )				/* hex is negative */
     {
       lNeg = 1;
       pStr++;
     }
     t_ptr = pStr;
     t_length = strlen(t_ptr);
     for(k=0;k < t_length;k++)
     {
       if (!isxdigit(*t_ptr))
       {
	 printf("Error: invalid hex operand, %s\n",orig_pStr);
   ht_destroy(label_table);
	 exit(4);
       }
       t_ptr++;
     }
     lNumLong = strtol(pStr, NULL, 16);    /* convert hex string into integer */
     lNum = (lNumLong > INT_MAX)? INT_MAX : lNumLong;
     if( lNeg )
       lNum = -lNum;
     return lNum;
   }
   else
   {
	printf( "Error: invalid operand, %s\n", orig_pStr);
  ht_destroy(label_table);
	exit(4);  /* This has been changed from error code 3 to error code 4, see clarification 12 */
   }
}

/*
Function that reads a line of text from a file and 
parses the line for the data held inside.
*/
int readAndParse( FILE* pInfile, char* pLine, char** pLabel, char
	** pOpcode, char** pArg1, char** pArg2, char** pArg3, char** pArg4
	)
	{
	   char * lRet, * lPtr;
	   int i;
	   if( !fgets( pLine, MAX_LINE_LENGTH, pInfile ) )
		return( DONE );
	   for( i = 0; i < strlen( pLine ); i++ )
		pLine[i] = tolower( pLine[i] );
	   
           /* convert entire line to lowercase */
	   *pLabel = *pOpcode = *pArg1 = *pArg2 = *pArg3 = *pArg4 = pLine + strlen(pLine);

	   /* ignore the comments */
	   lPtr = pLine;

	   while( *lPtr != ';' && *lPtr != '\0' &&
	   *lPtr != '\n' ) 
		lPtr++;

	   *lPtr = '\0';
	   if( !(lPtr = strtok( pLine, "\t\n ," ) ) ) 
		return( EMPTY_LINE );

	   if( isOpcode( lPtr ) == false && lPtr[0] != '.' ) /* found a label */
	   {
		*pLabel = lPtr;
		if( !( lPtr = strtok( NULL, "\t\n ," ) ) ) return( OK );
	   }
	   
           *pOpcode = lPtr;

	   if( !( lPtr = strtok( NULL, "\t\n ," ) ) ) return( OK );
	   
           *pArg1 = lPtr;
	   
           if( !( lPtr = strtok( NULL, "\t\n ," ) ) ) return( OK );

	   *pArg2 = lPtr;
	   if( !( lPtr = strtok( NULL, "\t\n ," ) ) ) return( OK );

	   *pArg3 = lPtr;

	   if( !( lPtr = strtok( NULL, "\t\n ," ) ) ) return( OK );

	   *pArg4 = lPtr;

	   return( OK );
	}


/*
Simple function that converts an integer into its hex
character equivalent
*/
char toHexString(uint8_t input){

  switch (input){
    case 0: return '0';
      break; 
    case 1: return '1';
      break;
    case 2: return '2';
      break;
    case 3: return '3';
      break;
    case 4: return '4';
      break;
    case 5: return '5';
      break;
    case 6: return '6';
      break;
    case 7: return '7';
      break;
    case 8: return '8';
      break;
    case 9: return '9';
      break;
    case 10: return 'a';
      break;
    case 11: return 'b';
      break;
    case 12: return 'c';
      break;
    case 13: return 'd';
      break;
    case 14: return 'e';
      break;
    case 15: return 'f';
      break;
    default: return 'g';
    break;
  }

  return '0';
}
