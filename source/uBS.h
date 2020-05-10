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

// FRAM on SPI related
#define CMD_25AA_WRSR     0x01  // Write status register
#define CMD_25AA_WRITE    0x02
#define CMD_25AA_READ     0x03
#define CMD_25AA_WRDI     0x04  // Write Disable
#define CMD_25AA_RDSR     0x05  // Read Status Register
#define CMD_25AA_WREN     0x06  // Write Enable
#define CMD_25AA_RDID     0x9F  // Read FRAM ID
#define STATUS_25AA_WEL   0b00000010  // write enable latch (1 == write enable)

void uBSFormat(void);
void uBSInit(void);
int8_t uBSWrite(void* name, uint8_t nameSize, void *data, uint16_t dataSize);
int8_t uBSRead(void* name, uint8_t nameSize, void *data, uint16_t *dataSize);
int8_t uBSSeekAll(uint32_t* address, void* name, uint8_t nameSize);
int8_t uBSRename(void* oldName, void* newName, uint8_t nameSize);

#endif /* UBS_H_ */
