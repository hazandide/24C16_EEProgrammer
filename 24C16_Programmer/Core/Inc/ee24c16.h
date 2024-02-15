/*
 * ee24c16.h
 */

#ifndef INC_EE24C16_H_
#define INC_EE24C16_H_

#include <stdint.h>
#include "main.h"

#define EE_CHECK_PARAMETERS

#define EE_USE_WP
#ifdef EE_USE_WP
	#define EE_WP_PORT				GPIOA
	#define EE_WP_PIN					GPIO_PIN_5
#endif	/* EE_USE_WP */

#define EE_DEF_ADDR					0xA0
#define EE_PAGE_SIZE 				16U
#define EE_PAGE_COUNT 			128U
#define EE_SECTOR_COUNT 		8U
#define EE_PAGE_PER_SECTOR  (EE_PAGE_COUNT / EE_SECTOR_COUNT)
#define EE_SECTOR_SIZE			(EE_PAGE_PER_SECTOR * EE_PAGE_SIZE)
#define EE_TOTAL_SIZE				(EE_PAGE_SIZE * EE_PAGE_COUNT)

#define EE_WRITE_WAIT				5U
#define EE_DEF_TIMEOUT 			10000U

#define EE_NORMALWRITE			0U
#define EE_ERASEWRITE				1U

typedef enum
{
	EE_OK = 0,
	EE_ERR_NULLPTR,
	EE_ERR_EXCEED_TOTAL_SIZE,
	EE_ERR_NULLSIZE,
	EE_ERR_WP_MISCONFIG,
	EE_ERR_TIMEOUT,
	EE_ERR_HAL_I2C,
	EE_ERR_DEVICE_I2C,
} EE_StatusTypeDef;

#ifdef EE_USE_WP
void EE_WpControl(uint8_t val);
#endif /* EE_USE_WP */
EE_StatusTypeDef EE_Write(I2C_HandleTypeDef *hi2c, uint16_t MemStartAddr, uint8_t *pData, uint16_t Size, uint8_t EraseMode, uint32_t Timeout);
EE_StatusTypeDef EE_Read(I2C_HandleTypeDef *hi2c, uint16_t MemStartAddr, uint8_t *pStore, uint16_t Size, uint32_t Timeout);

#endif /* INC_EE24C16_H_ */
