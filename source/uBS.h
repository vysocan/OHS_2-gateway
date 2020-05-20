/*
 * uBS.h
 *
 *  Created on: 27. 11. 2019
 *      Author: vysocan
 */

#ifndef UBS_H_
#define UBS_H_

#include "hal.h"
#include "chprintf.h"

/*
 * uBS data block definitions
 */
// Editable
#define UBS_BLOCK_SIZE        32
#define UBS_BLOCK_COUNT       256
#define UBS_NAME_SIZE         16
#define UBS_START_ADDRESS     16384
// Fixed
#define UBS_END_ADDRESS       (UBS_START_ADDRESS + (UBS_BLOCK_SIZE * UBS_BLOCK_COUNT))
#define UBS_HEADER_SIZE       1
#define UBS_POINTER_SIZE      2
#define UBS_HEADER_BLOCK_SIZE (UBS_HEADER_SIZE + UBS_POINTER_SIZE)
// Helpers
#define UBS_HEADER_ALLOW_SIZE 127 //((UBS_HEADER_SIZE * 255) >> 1)
#define UBS_DATA_SIZE         (UBS_BLOCK_SIZE - UBS_HEADER_BLOCK_SIZE)
#define UBS_FIRST_DATA_SIZE   (UBS_DATA_SIZE - UBS_NAME_SIZE)
#define UBS_CMD_BUF_SIZE      (1 + UBS_POINTER_SIZE)
// Statistics
#define UBS_SPACE_MAX         ((UBS_BLOCK_COUNT * UBS_DATA_SIZE) - UBS_NAME_SIZE)

// FRAM on SPI related
#define CMD_25AA_WRSR     0x01  // Write status register
#define CMD_25AA_WRITE    0x02
#define CMD_25AA_READ     0x03
#define CMD_25AA_WRDI     0x04  // Write Disable
#define CMD_25AA_RDSR     0x05  // Read Status Register
#define CMD_25AA_WREN     0x06  // Write Enable
#define CMD_25AA_RDID     0x9F  // Read FRAM ID
#define STATUS_25AA_WEL   0b00000010  // write enable latch (1 == write enable)

extern uint16_t uBSFreeSpace;
extern uint16_t uBSFreeBlocks;

void uBSFormat(void);
void uBSInit(void);
int8_t uBSErase(void* name, uint8_t nameSize);
int8_t uBSWrite(void* name, uint8_t nameSize, void *data, uint16_t dataSize);
int8_t uBSRead(void* name, uint8_t nameSize, void *data, uint16_t *dataSize);
int8_t uBSSeekAll(uint32_t* address, void* name, uint8_t nameSize);
int8_t uBSRename(void* oldName, void* newName, uint8_t nameSize);

#endif /* UBS_H_ */
