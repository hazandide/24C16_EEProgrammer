/*
 * ee24c16.c
 */

#include "ee24c16.h"
#include <string.h>

#ifdef EE_USE_WP
/**
 * Control WP pin state of EEPROM
 * @param val. if equals to zero disable write protection
 */
void EE_WpControl(uint8_t val)
{
	HAL_GPIO_WritePin(EE_WP_PORT, EE_WP_PIN, (val > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET));
}
#endif /* EE_USE_WP */

/**
 * Function to check arguments if they are appropriate for operations
 * @param hi2c 					I2C handle
 * @param MemStartAddr	Defines start address of memory for writing i.e. offset
 * 											Defines start address of memory for copying in Read mode
 * @param pData					Pointer to array which will be copied to memory in Write mode
 * 											Opposite for Read mode
 * 											~~~ size of this Array must be bigger than 'Size' argument ~~~
 * @param Size					How many bytes will copied from pData Arr to memory
 * 											Opposite for Read mode
 * @param EraseMode			If equals to EE_ERASEWRITE, ignore Pdata and erase given addresses of memory
 * 											Not used in Read Mode
 * @return 							Status of Operation
 */
static EE_StatusTypeDef EE_CheckArguments(I2C_HandleTypeDef *hi2c, uint16_t MemStartAddr, uint8_t *pData, uint16_t Size, uint8_t EraseMode)
{
	EE_StatusTypeDef retVal = EE_OK;

#ifdef EE_CHECK_PARAMETERS
	if (hi2c == NULL || (!EraseMode && pData == NULL))
	{
		retVal = EE_ERR_NULLPTR;
	}
	else if (MemStartAddr + Size > EE_TOTAL_SIZE)
	{
		retVal = EE_ERR_EXCEED_TOTAL_SIZE;
	}
	else if (Size == 0)
	{
		retVal = EE_ERR_NULLSIZE;
	}
	else if (HAL_I2C_IsDeviceReady(hi2c, EE_DEF_ADDR, 4, 100) != HAL_OK)
	{
		retVal = EE_ERR_DEVICE_I2C;
	}
#ifdef EE_USE_WP
	else if (HAL_GPIO_ReadPin(EE_WP_PORT, EE_WP_PIN))
	{
		retVal = EE_ERR_WP_MISCONFIG;
	}
#endif /* EE_USE_WP */
#endif /* EE_CHECK_PARAMETERS */

	return retVal;
}

/**
 *	Function to Write Data to EEPROM
 * @param hi2c 					I2C handle
 * @param MemStartAddr	Defines start address of memory for writing i.e. offset
 * @param pData					Pointer to array which will be copied to memory in Write mode
 * @param Size					How many bytes will copied from pData Arr to memory
 * @param EraseMode			If equals to EE_ERASEWRITE, ignore Pdata and erase given addresses of memory
 * @param Timeout				Total maximum time allowed for all Write operation
 * @return							Status of Operation
 */
EE_StatusTypeDef EE_Write(I2C_HandleTypeDef *hi2c, uint16_t MemStartAddr, uint8_t *pData, uint16_t Size, uint8_t EraseMode, uint32_t Timeout)
{
	/* Check Arguments */
	EE_StatusTypeDef retVal = EE_CheckArguments(hi2c, MemStartAddr, pData, Size, EraseMode);
	if (retVal)
	{
		return retVal;
	}

	const uint8_t 	StartSector 	= MemStartAddr / EE_SECTOR_SIZE;
	const uint8_t 	StopSector 		= (MemStartAddr + Size) / EE_SECTOR_SIZE;
	uint8_t					DevAddress		= ((EE_DEF_ADDR & ~(1 << 0)) | (StartSector << 1));
	uint8_t 				BurstSize 		= EE_PAGE_SIZE;
	uint8_t* 				pArr 					= pData;
	uint16_t 			 	MemCounter 		= MemStartAddr;
	const uint16_t 	MemStopAddr		= MemStartAddr + Size;
	const uint32_t 	TickStart 		= HAL_GetTick();

	/* If EraseMode Ignore pData, create Erase array and assign to pArr*/
	if (EraseMode)
	{
		uint8_t EraseArr[EE_PAGE_SIZE];
		memset(EraseArr, 0xFF, EE_PAGE_SIZE);
		pArr = EraseArr;
	}

	/* Write Sector By Sector */
	for (uint8_t i = StartSector; i <= StopSector; )
	{
		/* For last page */
		if ((MemStopAddr - MemCounter) < 16)
		{
			BurstSize = MemStopAddr - MemCounter;
			if (BurstSize == 0)
			{
				break;
			}

			/* There is data left.
			 * After last page write operation, Overflowed Sector counter will be checked
			 * and this will break for loop
			 */
			i = StopSector + 1;
		}
		else
		{
			/* For finding first page offset. After first page offset this will give
			 * EE_PAGE_SIZE (16) and pages will be filled completely
			 */
			BurstSize = (EE_PAGE_SIZE - (MemCounter % EE_PAGE_SIZE));
		}

		if (HAL_I2C_Mem_Write(hi2c, DevAddress, (MemCounter % EE_SECTOR_SIZE), I2C_MEMADD_SIZE_8BIT, pArr, BurstSize, Timeout) != HAL_OK)
		{
			retVal = EE_ERR_HAL_I2C;
			break;
		}

		/* Increase MemCounter by BurstSize */
		MemCounter += BurstSize;

		/* If write mode is normalWrite not Erase then increase data array pointer too */
		if (!EraseMode)
		{
			pArr += BurstSize;
		}

		/* Wait for page write operation in blocking mode */
		HAL_Delay(EE_WRITE_WAIT);

		/* In this device A0 A1 A2 pins are NC.
		 * By setting the first, second and third bits of Device Address value in I2C line,
		 * We reach different sectors of memory.
		 * Each sector is 16 pages long. Each page is 16 bytes long.
		 * There are total 8 sectors (000 -> 111).
		 */
		if ((MemCounter % EE_SECTOR_SIZE) == 0)
		{
			DevAddress &= ~(0xF << 0);
			DevAddress |= (++i << 1);
		}

		/* Check for timeout */
		if (HAL_GetTick() - TickStart > Timeout)
		{
			retVal = EE_ERR_TIMEOUT;
			break;
		}
	}

	return retVal;
}

/**
 * Function to Read memory and copy Size argument amount of data to pStore Arr
 * @param hi2c
 * @param MemStartAddr
 * @param pStore
 * @param Size
 * @param Timeout
 * @return
 */
EE_StatusTypeDef EE_Read(I2C_HandleTypeDef *hi2c, uint16_t MemStartAddr, uint8_t *pStore, uint16_t Size, uint32_t Timeout)
{
	EE_StatusTypeDef retVal = EE_CheckArguments(hi2c, MemStartAddr, pStore, Size, 0);

	if (retVal)
	{
		return retVal;
	}

	const uint8_t StartSector = MemStartAddr / EE_SECTOR_SIZE;
	uint8_t	DevAddress = ((EE_DEF_ADDR | (1 << 0)) | (StartSector << 1));

	/* Just Giving the start values. Device and HAL API will handle it
	 * Because there is no time constraint like in Write mode
	 */
	if (HAL_I2C_Mem_Read(hi2c, DevAddress, (MemStartAddr % EE_SECTOR_SIZE), I2C_MEMADD_SIZE_8BIT, pStore, Size, Timeout) != HAL_OK)
	{
		retVal = EE_ERR_HAL_I2C;
	}

	return retVal;
}
