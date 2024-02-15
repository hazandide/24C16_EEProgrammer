/*
 * virtualcomport.h
 */

#ifndef INC_VIRTUALCOMPORT_H_
#define INC_VIRTUALCOMPORT_H_

#include <stdint.h>

#define VCP_BUFF_CMD_SRCH			32U
#define VCP_BUFF_AFTER_EQSIGN 16U
#define VCP_COMMAND_COUNT 		3U

#define FALSE 0
#define TRUE  1

typedef enum
{
	CWrite = 0,
	CErase,
	CRead,
	CNone
} VCP_NumberTypeDef;

typedef struct {
	const char* vcp_command;
	uint8_t size;
} VCP_CommandTypeDef;

typedef struct
{
	uint8_t 	DataReceived;
	uint8_t 	SendingData;
	uint16_t 	CurrVcpRxSize;
} VCP_FlagsTypeDef;

typedef struct
{
	VCP_NumberTypeDef CmdName;
	uint16_t					StartAddr;
	uint16_t 					StopAddr;
	uint8_t						StartofDataInRxBuff;
} VCP_AnswerTypeDef;


VCP_AnswerTypeDef VCP_SearchCommand(char* RxBuff, uint16_t Size, VCP_CommandTypeDef *VcpCommands);

#endif /* INC_VIRTUALCOMPORT_H_ */
