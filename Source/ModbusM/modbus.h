#ifndef _MODBUS_H_
#define _MODBUS_H_

#include "enum.h"

/*
*Macros
*/
#define		MAX_PARAMS			64
/*
*Structure
*/
/* Required fields for make a modbus query */
typedef struct
{
	unsigned char	nodeAddr;
	unsigned char	funcCode;
	unsigned short	startAddr;
	unsigned short	noOfRegis;
	BYTEE_ORDER		byteOrder; // if its 1 - normal or swift byte
	WORD_ORDER 		wordOrder; // if its 1 - normal or swift word
	REG_SIZE		regSize;   // if its 0 - 16bit ,1 - 32bit
	VALUE_DATA_TYPE	typeOfValue; // 0 - boolean bit , 1 - IEEE float(32bit), 2 - Uint(32bit), 3 - int(32bit), 4 - Uint(16bit), 5 - int(16bit)
}MODBUS_CONFIG;

/* For generic MODBUS meter */
typedef union
{
	short int				singned16BitValue;
	unsigned short int		unSingned16BitValue;
	long int				singned32BitValue;
	unsigned long int		unSingned32BitValue;
	float					floatValue;
	char					unused[SIZE_256];
}MODDATA;

typedef struct
{
	unsigned char			storedType;
	MODDATA					modData;
}MODBUS_DATA;

/*
*Function declarations
*/
int initModQuery(void);
unsigned short int crc( unsigned char *data, int start, int cnt);
int procModResponce(unsigned int paramIndx);

#endif
/* EOF */
