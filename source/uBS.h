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
 * uBS feature definitions
 *
 * FREE_MAP   - Creates free block map. Free blocks are then searched in memory map
 *              rather then in FRAM.
 * MASTER_MAP - Creates master block map. Not yet implemented.
 */
#define UBS_USE_FREE_MAP      0 // Put 0 or 1
#define UBS_USE_MASTER_MAP    0 // Put 0 or 1
/*
 * uBS data block definitions
 */
// Editable
#define UBS_BLOCK_SIZE        128
#define UBS_BLOCK_COUNT       512
#define UBS_NAME_SIZE         16    // Block name size in bytes
#define UBS_ADDRESS_START     65536 // Offset of uBS space
// Fixed
#define UBS_ADDRESS_END       (UBS_ADDRESS_START + (UBS_BLOCK_SIZE * UBS_BLOCK_COUNT))
#define UBS_HEADER_SIZE       1 // Bytes
#define UBS_ADDRESS_SIZE      3 // Bytes
#define UBS_HEADER_BLOCK_SIZE (UBS_HEADER_SIZE + UBS_ADDRESS_SIZE)
// Helpers
#define UBS_HEADER_ALLOW_SIZE 127 //((UBS_HEADER_SIZE * 255) >> 1)
#define UBS_DATA_SIZE         (UBS_BLOCK_SIZE - UBS_HEADER_BLOCK_SIZE)
#define UBS_FIRST_DATA_SIZE   (UBS_DATA_SIZE - UBS_NAME_SIZE)
#define UBS_CMD_BUF_SIZE      (1 + UBS_ADDRESS_SIZE)
#define UBS_MAP_SIZE          (UBS_BLOCK_COUNT/8) + ((UBS_BLOCK_COUNT%8) ? 1 : 0)
// Statistics
#define UBS_SPACE_MAX         ((UBS_BLOCK_COUNT * UBS_DATA_SIZE) - UBS_NAME_SIZE)
/*
 * Sanity checks
 */
#if UBS_NAME_SIZE > UBS_DATA_SIZE
  #error Size of master block name larger then block data size!
#endif

#if UBS_DATA_SIZE > UBS_HEADER_ALLOW_SIZE
  #error Size of data is larger then UBS_HEADER_ALLOW_SIZE!
#endif

#if UBS_BLOCK_SIZE > (UBS_HEADER_SIZE * 256)
  #error Size of blok is larger then uint8_t size!
#endif
// Status replies
#define UBS_RSLT_OK        (1)
#define UBS_RSLT_NOK       (0)
#define UBS_RSLT_NOT_FOUND (-1)
#define UBS_RSLT_TOO_LARGE (-2)
#define UBS_RSLT_DAMAGED   (-127)

// FRAM on SPI related
#define CMD_25AA_WRSR     0x01  // Write status register
#define CMD_25AA_WRITE    0x02
#define CMD_25AA_READ     0x03
#define CMD_25AA_WRDI     0x04  // Write Disable
#define CMD_25AA_RDSR     0x05  // Read Status Register
#define CMD_25AA_WREN     0x06  // Write Enable
#define CMD_25AA_RDID     0x9F  // Read FRAM ID
#define STATUS_25AA_WEL   0b00000010  // write enable latch (1 == write enable)

extern uint32_t uBSFreeSpace;
extern uint16_t uBSFreeBlocks;

void uBSFormat(void);
int8_t uBSInit(void);
int8_t uBSErase(void* name, uint8_t nameSize);
int8_t uBSWrite(void* name, uint8_t nameSize, void *data, uint16_t dataSize);
int8_t uBSRead(void* name, uint8_t nameSize, void *data, uint16_t *dataSize);
int8_t uBSSeekAll(uint32_t* address, void* name, uint8_t nameSize);
int8_t uBSRename(void* oldName, uint8_t oldNameSize, void* newName, uint8_t newNameSize);

#endif /* UBS_H_ */
