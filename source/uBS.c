/*
 * Copyright (c) Adam Baron <vysocan76@gmail.com>
 * All rights reserved.
 *
 * uBS - micro Block System for FRAM
 * Adam Baron 2020
 *
 *
 ************************************************************************************
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 ************************************************************************************
 *
 * This library provides a simple block access to SPI memory device like FRAM or
 * EEPROM.
 *
 * BLOCK:
 * 0123456789~~~01
 * HPP............
 * H - Header
 * P - Pointer to next block, 1 - 4 byte(s)
 * . - Data
 *
 * Header bits
 * 76543210
 * SSSSSSSM
 * M - Master block flag 1
 * S - Size up to 128 bytes, 0 empty block
 *
 * Master block
 *
 */

#include "uBS.h"
#include <string.h>

#ifndef UBS_DEBUG
#define UBS_DEBUG 0
#endif

#if UBS_DEBUG
#define DBG(...) {chprintf((BaseSequentialStream*)&SD3, __VA_ARGS__);}
#else
#define DBG(...)
#endif

/*
 * Macros
 */
#define UBS_GET_MASTER_FLAG(x)     ((x) & 0b1)
#define UBS_GET_BLOCK_DATA_SIZE(x) ((x >> 1U) & UBS_HEADER_ALLOW_SIZE)
#define UBS_SET_MASTER_FLAG(x)       x |= 1
#define UBS_SET_BLOCK_DATA_SIZE(x,y) x |= (y << 1U)
/*
 * Variables
 */
static uint8_t  uBSCmdBuf[UBS_CMD_BUF_SIZE];
uint32_t uBSFreeSpace = 0;
uint16_t uBSFreeBlocks = 0;
#if UBS_USE_FREE_MAP > 0
static uint8_t uBSFreeMap[UBS_MAP_SIZE];
#endif
#if UBS_USE_MASTER_MAP > 0
static uint8_t uBSMasterMap[UBS_MAP_SIZE];
#endif
/*
 * ReadBlock
 */
static int8_t uBSReadBlock(uint32_t address, uint8_t *data, uint8_t size) {
  osalDbgCheck(size <= UBS_BLOCK_SIZE);
  // Sanity check
  if ((address < UBS_ADDRESS_START) || (address > UBS_ADDRESS_END)) return UBS_RSLT_NOK;

  uBSCmdBuf[0] = CMD_25AA_READ;
  for (uint8_t i = 0; i < UBS_ADDRESS_SIZE; i++) {
    uBSCmdBuf[1 + i] = (address >> ((UBS_ADDRESS_SIZE - i - 1) * 8)) & 0xFF;
  }

  spiAcquireBus(&SPID1);
  spiSelect(&SPID1);
  spiSend(&SPID1, UBS_CMD_BUF_SIZE, uBSCmdBuf);
  spiReceive(&SPID1, size, data);
  spiUnselect(&SPID1);
  spiReleaseBus(&SPID1);

  return UBS_RSLT_OK;
}
/*
 * uBSWriteBlock
 */
static void uBSWriteBlock(uint32_t address, uint8_t* data, uint8_t size) {
  osalDbgCheck(size <= UBS_BLOCK_SIZE);

  spiAcquireBus(&SPID1);

  spiSelect(&SPID1);
  uBSCmdBuf[0] = CMD_25AA_WREN;
  spiSend(&SPID1, 1, uBSCmdBuf);
  spiUnselect(&SPID1);

  uBSCmdBuf[0] = CMD_25AA_WRITE;
  for (uint8_t i = 0; i < UBS_ADDRESS_SIZE; i++) {
    uBSCmdBuf[1 + i] = (address >> ((UBS_ADDRESS_SIZE - i - 1) * 8)) & 0xFF;
  }

  spiSelect(&SPID1);
  spiSend(&SPID1, UBS_CMD_BUF_SIZE, uBSCmdBuf);
  spiSend(&SPID1, size, data);
  spiUnselect(&SPID1);

  spiReleaseBus(&SPID1);
}
/*
 * uBSGetFreeBlock
 *
 * Use UBS_END_ADDRESS as dummy value
 */
static int8_t uBSGetFreeBlock(uint32_t* address) {
  uint8_t readBuf[UBS_HEADER_SIZE];
  uint32_t toSkip = *address;
  *address = UBS_ADDRESS_START;

  DBG("uBS free address skip: %u\r\n", toSkip);
  while (*address < UBS_ADDRESS_END) {
    if (*address != toSkip) {
      #if UBS_USE_FREE_MAP > 0
        // Get free bit and negate
        readBuf[0] = !((uBSFreeMap[((*address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) / 8] >>
            (((*address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) % 8)) & 0b1);
      #else
      if (uBSReadBlock(*address, &readBuf[0], UBS_HEADER_SIZE) != UBS_RSLT_OK)
        return UBS_RSLT_DAMAGED;
      #endif

      if (readBuf[0] == 0) {
        DBG("uBS free - found address: %u\r\n", *address);
        uBSFreeSpace -= UBS_DATA_SIZE;
        uBSFreeBlocks--;
        #if UBS_USE_FREE_MAP > 0
          // Clear free bit
          uBSFreeMap[((*address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) / 8] &=
              ~(1 << (((*address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) % 8));
        #endif
        return UBS_RSLT_OK;
      }
    }
    *address += UBS_BLOCK_SIZE;
  }
  DBG("uBS free - NOT found\r\n");
  return UBS_RSLT_NOK;
}
/*
 * uBSSeekName - Find block with blockName.
 */
static int8_t uBSSeekName(void* blockName, uint8_t nameSize, uint32_t* address) {
  uint8_t readBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE];

  *address = UBS_ADDRESS_START;
  do {
    if (uBSReadBlock(*address, &readBuf[0], UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE) != UBS_RSLT_OK)
      return UBS_RSLT_DAMAGED;
    if ((UBS_GET_MASTER_FLAG(readBuf[0])) &&
        (memcmp(&readBuf[UBS_HEADER_BLOCK_SIZE], blockName, nameSize) == 0)) {
      DBG("uBS Seek address: %u\r\n", *address);
      return UBS_RSLT_OK;
    }
    *address += UBS_BLOCK_SIZE;
  } while (*address < UBS_ADDRESS_END);

  return UBS_RSLT_NOT_FOUND;
}
/*
 * uBSSeekNameGetBlockSize - Find block with blockName and its size in block(s)
 *
 */
static int8_t uBSSeekNameGetBlockSize(void* blockName, uint8_t nameSize, uint32_t* address, uint16_t* blocks) {
  uint8_t readBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE];
  uint32_t nextBlock = 0;

  DBG("uBS SeekName: %s\r\n", blockName);

  *address = UBS_ADDRESS_START;
  *blocks = 0;
  do {
    if (uBSReadBlock(*address, &readBuf[0], UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE) != UBS_RSLT_OK)
      return UBS_RSLT_DAMAGED;
    if ((UBS_GET_MASTER_FLAG(readBuf[0])) &&
        (memcmp(&readBuf[UBS_HEADER_BLOCK_SIZE], blockName, nameSize) == 0)) {
      DBG("uBS SeekName found, ");
      *blocks = 1;
      for (uint8_t i = 0; i < UBS_ADDRESS_SIZE; i++) {
        nextBlock += (readBuf[1 + i] << ((UBS_ADDRESS_SIZE - i - 1) * 8));
      }
      while (nextBlock) {
        if (uBSReadBlock(nextBlock, &readBuf[0], UBS_HEADER_BLOCK_SIZE) != UBS_RSLT_OK)
          return UBS_RSLT_DAMAGED;
        (*blocks)++;
        nextBlock = 0;
        for (uint8_t i = 0; i < UBS_ADDRESS_SIZE; i++) {
          nextBlock += (readBuf[1 + i] << ((UBS_ADDRESS_SIZE - i - 1) * 8));
        }
      }
      DBG("address: %u, blocks: %u\r\n", *address, *blocks);
      return UBS_RSLT_OK;
    }
    *address += UBS_BLOCK_SIZE;
  } while (*address < UBS_ADDRESS_END);

  *blocks = 0;
  return UBS_RSLT_NOT_FOUND;
}
/*
 * uBSEraseBlock
 */
static int8_t uBSEraseBlock(uint32_t address, uint16_t eraseBlocks) {
  uint8_t writeBuf[UBS_BLOCK_SIZE] = {0};
  uint8_t readBuf[UBS_HEADER_BLOCK_SIZE];
  uint32_t nextBlock;

  for (uint16_t block = 0; block < eraseBlocks; block++) {
    if (uBSReadBlock(address, &readBuf[0], UBS_HEADER_BLOCK_SIZE) != UBS_RSLT_OK)
      return UBS_RSLT_DAMAGED;
    nextBlock = 0; // 0 to allow byte accumulation
    for (uint8_t i = 0; i < UBS_ADDRESS_SIZE; i++) {
      nextBlock += (readBuf[1 + i] << ((UBS_ADDRESS_SIZE - i - 1) * 8));
    }
    DBG("uBS Erase block address: %u\r\n", address);
    uBSWriteBlock(address, &writeBuf[0], UBS_BLOCK_SIZE);
    uBSFreeSpace += UBS_DATA_SIZE;
    uBSFreeBlocks++;
    #if UBS_USE_FREE_MAP > 0
      // Set free bit
      uBSFreeMap[((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) / 8] |=
           1 << (((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) % 8);
    #endif
    address = nextBlock;
  }

  return UBS_RSLT_OK;
}
/*
 * uBSFormat - erase all blocks
 */
void uBSFormat(void) {
  uint32_t address = UBS_ADDRESS_START;
  uint8_t writeBuf[UBS_BLOCK_SIZE] = {0};

  while (address < UBS_ADDRESS_END) {
    uBSWriteBlock(address, &writeBuf[0], UBS_BLOCK_SIZE);
    address += UBS_BLOCK_SIZE;
  }
  #if UBS_USE_FREE_MAP > 0
    // Set whole free map to 0b11111111
    memset(&uBSFreeMap[0], 255, UBS_MAP_SIZE);
  #endif
}
/*
 * uBSInit
 *
 * Calculate free space on start
 */
int8_t uBSInit(void) {
  uint32_t address = UBS_ADDRESS_START;
  uint8_t readBuf[UBS_HEADER_SIZE];

  uBSFreeSpace = 0;
  uBSFreeBlocks = 0;

  #if UBS_USE_FREE_MAP > 0
    // Clear whole free map
    memset(&uBSFreeMap[0], 0, UBS_MAP_SIZE);
  #endif
  #if UBS_USE_MASTER_MAP > 0
    // Clear whole master map
    memset(&uBSMasterMap[0], 0, UBS_MAP_SIZE);
  #endif

  while (address < UBS_ADDRESS_END) {
    if (uBSReadBlock(address, &readBuf[0], UBS_HEADER_SIZE) != UBS_RSLT_OK)
      return UBS_RSLT_DAMAGED;
    if (readBuf[0] == 0) {
      uBSFreeSpace += UBS_DATA_SIZE;
      uBSFreeBlocks++;
      #if UBS_USE_FREE_MAP > 0
        // Set free bit
        uBSFreeMap[((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) / 8] |=
            (1 << (((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) % 8));
      #endif
    }
    #if UBS_USE_MASTER_MAP > 0
      if (UBS_GET_MASTER_FLAG(readBuf[0])) {
        // Set free bit
        uBSMasterMap[((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) / 8] |=
              (1 << (((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) % 8));
      }
    #endif
    address += UBS_BLOCK_SIZE;
  }
  if (uBSFreeSpace > 0) uBSFreeSpace -= UBS_NAME_SIZE; // Remove one name size of master block

  DBG("uBS Init, free blocks: %u, free space:%u\r\n", uBSFreeBlocks, uBSFreeSpace);

  return UBS_RSLT_OK;
}
/*
 * uBSErase
 */
int8_t uBSErase(void* name, uint8_t nameSize) {
  uint32_t address;
  uint16_t blocks;

  if (name == NULL) return UBS_RSLT_NOK;
  if (nameSize > UBS_NAME_SIZE) return UBS_RSLT_NOK;
  if (nameSize == 0) return UBS_RSLT_NOK;

  if (uBSSeekNameGetBlockSize(name, nameSize, &address, &blocks) != UBS_RSLT_OK) return UBS_RSLT_NOT_FOUND;

  return uBSEraseBlock(address, blocks);
}
/*
 * uBSWrite
 */
int8_t uBSWrite(void* name, uint8_t nameSize, void *data, uint16_t dataSize) {
  uint16_t curSize, newBlocks, oldBlocks = 0;
  uint32_t address, nextBlock, eraseBlock;
  uint8_t  writeBuf[UBS_BLOCK_SIZE];
  uint8_t  readBuf[UBS_HEADER_BLOCK_SIZE];

  if (name == NULL) return UBS_RSLT_NOK;
  if (nameSize > UBS_NAME_SIZE) return UBS_RSLT_NOK;
  if (nameSize == 0) return UBS_RSLT_NOK;

  // Get new block size
  newBlocks = (dataSize + UBS_NAME_SIZE) / UBS_DATA_SIZE;
  if ((dataSize + UBS_NAME_SIZE) % UBS_DATA_SIZE) newBlocks++;

  if (uBSSeekNameGetBlockSize(name, nameSize, &address, &oldBlocks) != UBS_RSLT_OK) {
    address = UBS_ADDRESS_END;
    uBSGetFreeBlock(&address);
  }

  DBG("uBS Write address %u, newBlocks: %d, oldBlocks:%d\r\n", address, newBlocks, oldBlocks);
  if ((int16_t)(newBlocks - oldBlocks) > uBSFreeBlocks) return UBS_RSLT_TOO_LARGE;

  for (uint8_t block = 0; block < newBlocks; block++) {
    memset(&writeBuf[0], 0x0, UBS_BLOCK_SIZE);

    // Set header
    if (block == 0) {
      UBS_SET_MASTER_FLAG(writeBuf[0]);
      if (dataSize > UBS_FIRST_DATA_SIZE) curSize = UBS_FIRST_DATA_SIZE;
      else curSize = dataSize;
    } else {
      if (dataSize > UBS_DATA_SIZE) curSize = UBS_DATA_SIZE;
      else curSize = dataSize;
    }
    UBS_SET_BLOCK_DATA_SIZE(writeBuf[0], curSize);

    // Set pointer to next block
    nextBlock = 0;
    if (block < oldBlocks) {
      // Last old block no pointer
      if (block < (newBlocks - 1)) {
        if (block == (oldBlocks - 1)) {
          // Last old block need new pointer
          nextBlock = address;
          uBSGetFreeBlock(&nextBlock);
        } else {
          // Get old pointer
          if (uBSReadBlock(address, &readBuf[0], UBS_HEADER_BLOCK_SIZE) != UBS_RSLT_OK)
            return UBS_RSLT_DAMAGED;
          for (uint8_t i = 0; i < UBS_ADDRESS_SIZE; i++) {
            nextBlock += (readBuf[1 + i] << ((UBS_ADDRESS_SIZE - i - 1) * 8));
          }
        }
      } else {
        // Store old block pointer to erase
        if (uBSReadBlock(address, &readBuf[0], UBS_HEADER_BLOCK_SIZE) != UBS_RSLT_OK)
          return UBS_RSLT_DAMAGED;
        eraseBlock = 0;
        for (uint8_t i = 0; i < UBS_ADDRESS_SIZE; i++) {
          eraseBlock += (readBuf[1 + i] << ((UBS_ADDRESS_SIZE - i - 1) * 8));
        }
      }
    } else {
      // Last new block no pointer
      if (block < (newBlocks - 1)) {
        nextBlock = address;
        uBSGetFreeBlock(&nextBlock);
      }
    }
    for (uint8_t i = 0; i < UBS_ADDRESS_SIZE; i++) {
      writeBuf[1 + i] = (nextBlock >> ((UBS_ADDRESS_SIZE - i - 1) * 8)) & 0xFF;
    }
    DBG("|%u,%u,%u, size %u, next %u|-", writeBuf[0], writeBuf[1], writeBuf[2], curSize, nextBlock);

    // Copy name and/or data
    if (block == 0) {
      memcpy(&writeBuf[UBS_HEADER_BLOCK_SIZE], name, nameSize);
      memcpy(&writeBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE], data, curSize);
    } else {
      memcpy(&writeBuf[UBS_HEADER_BLOCK_SIZE], data, curSize);
    }
    //for (uint8_t i = 0; i < UBS_BLOCK_SIZE; i++) DBG("%u-", writeBuf[i]);
    DBG("uBS Write data: %u:%u\r\n", block, address);
    uBSWriteBlock(address, &writeBuf[0], UBS_BLOCK_SIZE);

    address = nextBlock;
    (data) += curSize;
    dataSize -= curSize;
  }

  // Erase old not used blocks
  // - not to use double buffer allocations
  // - if (newBlocks < oldBlocks) uBSEraseBlock(eraseBlock, oldBlocks - newBlocks);
  // Erase old not used blocks
  memset(&writeBuf[0], 0x0, UBS_BLOCK_SIZE);
  for (uint8_t block = newBlocks; block < oldBlocks; block++) {
    address = eraseBlock;
    if (uBSReadBlock(address, &readBuf[0], UBS_HEADER_BLOCK_SIZE) != UBS_RSLT_OK)
      return UBS_RSLT_DAMAGED;
    eraseBlock = 0; // 0 to allow byte accumulation
    for (uint8_t i = 0; i < UBS_ADDRESS_SIZE; i++) {
      eraseBlock += (readBuf[1 + i] << ((UBS_ADDRESS_SIZE - i - 1) * 8));
    }
    DBG("uBS Write erase: %u:%u\r\n", block, address);
    uBSWriteBlock(address, &writeBuf[0], UBS_BLOCK_SIZE);
    uBSFreeSpace += UBS_DATA_SIZE;
    uBSFreeBlocks++;
    #if UBS_USE_FREE_MAP > 0
      // Set free bit
      uBSFreeMap[((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) / 8] |=
           1 << (((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) % 8);
    #endif
  }

  return UBS_RSLT_OK;
}
/*
 * uBSRead
 */
int8_t uBSRead(void* name, uint8_t nameSize, void *data, uint16_t *dataSize) {
  uint16_t block = 0, curSize, readSize = 0;
  uint32_t address;
  uint8_t  readBuf[UBS_BLOCK_SIZE];

  if (name == NULL) return UBS_RSLT_NOK;
  if (nameSize > UBS_NAME_SIZE) return UBS_RSLT_NOK;
  if (nameSize == 0) return UBS_RSLT_NOK;
  if (data == NULL) return UBS_RSLT_NOK;
  if (*dataSize == 0) return UBS_RSLT_NOK;

  // NULL the data first
  memset(data, 0, *dataSize);

  DBG("uBS Read name: %s, readSize %u\r\n", name, readSize);
  if (uBSSeekName(name, nameSize, &address) != UBS_RSLT_OK) {
    *dataSize = 0;
    return UBS_RSLT_NOT_FOUND;
  }

  do {
    if (uBSReadBlock(address, &readBuf[0], UBS_BLOCK_SIZE) != UBS_RSLT_OK) return UBS_RSLT_DAMAGED;
    //for (uint8_t i = 0; i < UBS_BLOCK_SIZE; i++) DBG("%u-", readBuf[i]); DBG("\r\n");

    curSize = UBS_GET_BLOCK_DATA_SIZE(readBuf[0]);
    if (curSize > *dataSize) curSize = *dataSize;

    //DBG("uBS Read address: %u, size %u\r\n", address, curSize);
    if (block == 0) {
      memcpy(data, &readBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE], curSize);
    } else {
      memcpy(data, &readBuf[UBS_HEADER_BLOCK_SIZE], curSize);
    }

    address = 0;
    for (uint8_t i = 0; i < UBS_ADDRESS_SIZE; i++) {
      address += (readBuf[1 + i] << ((UBS_ADDRESS_SIZE - i - 1) * 8));
    }
    block++;
    *dataSize -= curSize;
    data += curSize;
    readSize += curSize;
    //DBG("uBS Read dataSize: %u, readSize %u\r\n", *dataSize, readSize);
  } while ((address) && (*dataSize));

  *dataSize = readSize;
  DBG("uBS Read end size: %u\r\n", *dataSize);
  return UBS_RSLT_OK;
}
/*
 * uBSSeekAll - Find all master block(s).
 * Use 0 as start address.
 *
 */
int8_t uBSSeekAll(uint32_t* address, void* name, uint8_t nameSize) {
  uint8_t readBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE];

  if (*address == 0) *address = UBS_ADDRESS_START;
  else *address += UBS_BLOCK_SIZE;
  if (nameSize > UBS_NAME_SIZE) nameSize = UBS_NAME_SIZE;

  DBG("uBS Seek all address: %u\r\n", *address);
  do {
    if (uBSReadBlock(*address, &readBuf[0], UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE) != UBS_RSLT_OK)
      return UBS_RSLT_DAMAGED;
    if (UBS_GET_MASTER_FLAG(readBuf[0])) {
      memcpy(name, &readBuf[UBS_HEADER_BLOCK_SIZE], nameSize);
      DBG("uBS Seek all found %s, address: %u\r\n", name, *address);
      return UBS_RSLT_OK;
    }
    *address += UBS_BLOCK_SIZE;
  } while (*address < UBS_ADDRESS_END);

  return UBS_RSLT_NOK;
}
/*
 * uBSRename - Update master block name.
 *
 */
int8_t uBSRename(void* oldName, uint8_t oldNameSize, void* newName, uint8_t newNameSize) {
  uint8_t readBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE];
  uint32_t address = UBS_ADDRESS_START;
  if (newNameSize > UBS_NAME_SIZE) return UBS_RSLT_TOO_LARGE;

  do {
    if (uBSReadBlock(address, &readBuf[0], UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE) != UBS_RSLT_OK)
      return UBS_RSLT_DAMAGED;
    //DBG("uBS rename address: %u\r\n", address);
    if ((UBS_GET_MASTER_FLAG(readBuf[0])) &&
        (memcmp(&readBuf[UBS_HEADER_BLOCK_SIZE], oldName, oldNameSize) == 0)) {
      DBG("uBS rename found %s, address: %u\r\n", oldName, address);
      memset(&readBuf[UBS_HEADER_BLOCK_SIZE], 0x0, UBS_NAME_SIZE);
      memcpy(&readBuf[UBS_HEADER_BLOCK_SIZE], newName, newNameSize);
      uBSWriteBlock(address, &readBuf[0], UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE);
      return UBS_RSLT_OK;
    }
    address += UBS_BLOCK_SIZE;
  } while (address < UBS_ADDRESS_END);

  return UBS_RSLT_NOK;
}
