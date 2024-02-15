/*
 * virtualcomport.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ee24c16.h"
#include "virtualcomport.h"

/**
 * Function to parse command and its parameter from terminal data
 * Terminal input format for writing, erasing, reading operations:
 *
 * CWrite={StartAddr} {Data}
 * CErase={StartAddr}-{StopAddr}
 * CRead={StartAddr}-{StopAddr}
 *
 * CErase=0-2048
 * will erase all EEPROM memory i.e. from 0th to 2047th
 *
 * CRead=0-1
 * will read just 0th bit in EEPROM memory.
 *
 * CRead=2047-2048
 * will read last byte in EEPROM memory
 *
 * CWrite=2047 b
 * will write "b" to last byte in memory.
 *
 * CWrite=2048 b
 * will throw EXCEED_TOTAL_SIZE error.
 *
 * CWrite=100 My Very Long Data ... DataEnd
 * will write "My Very Long Data ... DataEnd" to EEPROM starting from 100th byte
 *
 * CRead=100-200
 * will read EEPROM 100th - 199th addresses.
 *
 * @param RxBuff
 * @param Size
 * @param VcpCommands
 * @return
 */
VCP_AnswerTypeDef VCP_SearchCommand(char* RxBuff, uint16_t Size, VCP_CommandTypeDef *VcpCommands)
{
	VCP_AnswerTypeDef retVal = {CNone};

	char TempChar = '\0';
	char* CommandPos 	= NULL;

	/* Create buffers statically */
	char CmdBuff[VCP_BUFF_CMD_SRCH] = {0};
	char BuffAfterEqualSign[VCP_BUFF_AFTER_EQSIGN] = {0};

	/* Fill buffer for analyzing */
	memcpy(CmdBuff, RxBuff, ((Size < VCP_BUFF_CMD_SRCH) ? Size : (VCP_BUFF_CMD_SRCH - 1)));
	char* CmdDataEnd = (char*) (CmdBuff + strlen(CmdBuff));

	uint8_t i = 0;
	uint8_t j = 0;

	int32_t  	EEStartMem = 0;
	int32_t 	EEStopMem  = 0;

	for (; i < VCP_COMMAND_COUNT; i++)
	{
		j = 0;

		/* Check if there is command in the CmdBuff one by one
		 * If there store its position
		 */
		CommandPos = strstr(CmdBuff, VcpCommands[i].vcp_command);

		if (!CommandPos)
		{
			continue;
		}

		EEStartMem = 0;
		EEStopMem = 0;
		memset(BuffAfterEqualSign, 0, VCP_BUFF_AFTER_EQSIGN);

		/* CommandPos + VcpCommands[i].size will give "=" letter in CmdBuff
		 * From there copy numbers and "-" character to BuffAfterEqualSign with respecting buffer boundaries
		 * If there is other character just break and stop copying to BuffAfterEqualSign
		 * j variable will shift address to right one by one in CmdBuff
		 */
		uint8_t HyphenFlag = FALSE;
		while (((CommandPos + VcpCommands[i].size + j) < (CmdDataEnd)) && (j < (VCP_BUFF_AFTER_EQSIGN - 1)))
		{
			TempChar = *(CommandPos + VcpCommands[i].size + j);

			if ((TempChar >= 48 && TempChar <= 57) || TempChar == '-')
			{

				if (TempChar == '-')
				{
					/* In command format there is just 1 "-" character */
					if (HyphenFlag)
						break;
					else
						HyphenFlag = TRUE;
				}

				/* Fill array */
				BuffAfterEqualSign[j++] = TempChar;
			}
			else
			{
				break;
			}

		}

		/* Some numbers copied to BuffAfterEqualSign */
		if (j > 0)
		{
			uint8_t k = FALSE;
			char *remaining = NULL;
			char *remaining2 = NULL;

			EEStartMem = strtol(BuffAfterEqualSign, &remaining, 10);

			if (i == CErase || i == CRead)
			{
				/* In erase and read mode, there is stop address requirement
				 * increment remaining part to exclude "-" character because we do not want negative position
				 * "-" is for separating addresses
				 */
				EEStopMem = strtol(++remaining, &remaining2, 10);

				/* Control values */
				k = (EEStartMem > EEStopMem) 													? k+1 : k;
				k = ((EEStopMem > EE_TOTAL_SIZE || EEStopMem < 0)) 		? k+1 : k;
			}

			k = ((EEStartMem > EE_TOTAL_SIZE || EEStartMem < 0)) 	? k+1 : k;

			/* If values are in range, push values */
			if (!k)
			{
				retVal.CmdName = i;
				retVal.StartAddr = EEStartMem;
				retVal.StopAddr = EEStopMem;

				/* VcpAnswer.StartofDataInRxBuff is Offset value to exclude command and its parameter in VCP_RX_BUFF terminal data
				 * It is only used in write command because command and data are send together
				 *
				 * +1 comes from format. There is Separation character between {StartAddr} and {Data}
				 * CWrite={StartAddr} {Data}
				 */
				retVal.StartofDataInRxBuff = (VcpCommands[i].size + j + 1);

				/* We have 1 command and correct parameters, stop searching for command
				 * For chaining commands we can convert main block code to a function and call from here
				 * But there is no need for chaining commands
				 */
				break;
			}
		}
	}
	return retVal;
}

