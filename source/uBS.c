/*
 * uBS - micro Block System for FRAM
 * Adam Baron 2020
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
 *
 */

#include "uBS.h"
#include <string.h>

#ifndef UBS_DEBUG
#define UBS_DEBUG 1
#endif

#if UBS_DEBUG
#define DBG(...) {chprintf((BaseSequentialStream*)&SD3, __VA_ARGS__);}
#else
#define DBG(...)
#endif


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

/*
 * Sanity checks
 */
#if UBS_NAME_SIZE > UBS_DATA_SIZE
  #error Size of master block name larger then block data size!
#endif

#if UBS_DATA_SIZE > UBS_HEADER_ALLOW_SIZE
  #error Size of data is larger then UBS_HEADER_ALLOW_SIZE!
#endif
/*
 * Macros
 */
#define UBS_GET_MASTER_FLAG(x)     ((x) & 0b1)
#define UBS_GET_BLOCK_DATA_SIZE(x) ((x >> 1U) & UBS_HEADER_ALLOW_SIZE)

#define UBS_SET_MASTER_FLAG(x)       x |= 1
#define UBS_SET_BLOCK_DATA_SIZE(x,y) x |= (y << 1U)
//#define UBS_SET_MASTER_FLAG(x,y)     x = (((x)&(0b11111110))|(((y & 0b1) << 0U)&(0b00000001)))
//#define UBS_SET_BLOCK_DATA_SIZE(x,y) x = (((x)&(0b00000001))|(((y & 0b1111111) << 1U)&(0b11111110)))


/*
 * Defines
 */
#define UBS_RSLT_OK        (1)
#define UBS_RSLT_NOK       (0)
#define UBS_RSLT_NOT_FOUND (-1)
#define UBS_RSLT_TOO_LARGE (-2)
/*
 * Variables
 */
static uint8_t  uBSCmdBuf[UBS_CMD_BUF_SIZE];
//static uint8_t  uBSBlockBuf[UBS_BLOCK_SIZE];
static uint16_t uBSFreeSpace = 0;
static uint16_t uBSFreeBlocks = 0;
// TODO OHS Check that address is within boundary of START and END.
/*
 * ReadBlock
 */
void uBSReadBlock(uint32_t address, uint8_t *data, uint8_t size) {

  osalDbgCheck(size <= 32);

  uBSCmdBuf[0] = CMD_25AA_READ;
  for (uint8_t i = 0; i < UBS_POINTER_SIZE; i++) {
    uBSCmdBuf[1 + i] = (address >> ((UBS_POINTER_SIZE - i - 1) * 8)) & 0xFF;
  }

  uBSCmdBuf[1] = (address >> 8) & 0xFF;
  uBSCmdBuf[2] = address & 0xFF;

  spiAcquireBus(&SPID1);
  spiSelect(&SPID1);
  spiSend(&SPID1, UBS_CMD_BUF_SIZE, uBSCmdBuf);
  spiReceive(&SPID1, size, data);
  spiUnselect(&SPID1);
  spiReleaseBus(&SPID1);
}
/*
 * uBSWriteBlock
 */
void uBSWriteBlock(uint32_t address, uint8_t* data, uint8_t size) {

  osalDbgCheck(size <= 32);

  spiAcquireBus(&SPID1);

  spiSelect(&SPID1);
  uBSCmdBuf[0] = CMD_25AA_WREN;
  spiSend(&SPID1, 1, uBSCmdBuf);
  spiUnselect(&SPID1);

  uBSCmdBuf[0] = CMD_25AA_WRITE;
  for (uint8_t i = 0; i < UBS_POINTER_SIZE; i++) {
    uBSCmdBuf[1 + i] = (address >> ((UBS_POINTER_SIZE - i - 1) * 8)) & 0xFF;
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
int8_t uBSGetFreeBlock(uint32_t* address) {
  uint8_t readBuf[UBS_HEADER_SIZE];
  uint32_t toSkip = *address;
  *address = UBS_START_ADDRESS;

  DBG("uBS free address skip: %u\r\n", toSkip);
  while (*address < UBS_END_ADDRESS) {
    DBG("uBS free - address: %u\r\n", *address);
    if (*address != toSkip) {
      uBSReadBlock(*address, &readBuf[0], UBS_HEADER_SIZE);
      if (readBuf[0] == 0) {
        uBSFreeSpace -= UBS_DATA_SIZE;
        uBSFreeBlocks--;
        return UBS_RSLT_OK;
      }
    }
    *address += UBS_BLOCK_SIZE;
  }
  return UBS_RSLT_NOK;
}
/*
 * uBSSeekName - Find block with blockName.
 */
int8_t uBSSeekName(void* blockName, uint8_t nameSize, uint32_t* address) {
  uint8_t readBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE];

  *address = UBS_START_ADDRESS;
  do {
    uBSReadBlock(*address, &readBuf[0], UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE);
    if ((UBS_GET_MASTER_FLAG(readBuf[0])) &&
        (memcmp(&readBuf[UBS_HEADER_BLOCK_SIZE], blockName, nameSize) == 0)) {
      DBG("uBS Seek address: %u\r\n", *address);
      return UBS_RSLT_OK;
    }
    *address += UBS_BLOCK_SIZE;
  } while (*address < UBS_END_ADDRESS);

  return UBS_RSLT_NOK;
}
/*
 * uBSSeekNameGetBlockSize - Find block with blockName and its size in block(s)
 *
 */
int8_t uBSSeekNameGetBlockSize(void* blockName, uint8_t nameSize, uint32_t* address, uint16_t* blocks) {
  uint8_t readBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE];
  uint32_t nextBlock;

  *address = UBS_START_ADDRESS;
  do {
    uBSReadBlock(*address, &readBuf[0], UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE);
    if ((UBS_GET_MASTER_FLAG(readBuf[0])) &&
        (memcmp(&readBuf[UBS_HEADER_BLOCK_SIZE], blockName, nameSize) == 0)) {
      *blocks = 1;
      for (uint8_t i = 0; i < UBS_POINTER_SIZE; i++) {
        nextBlock += (readBuf[1 + i] << ((UBS_POINTER_SIZE - i - 1) * 8));
      }
      while (nextBlock) {
        uBSReadBlock(nextBlock, &readBuf[0], UBS_HEADER_BLOCK_SIZE);
        (*blocks)++;
        nextBlock = 0;
        for (uint8_t i = 0; i < UBS_POINTER_SIZE; i++) {
          nextBlock += (readBuf[1 + i] << ((UBS_POINTER_SIZE - i - 1) * 8));
        }
      }
      DBG("uBS Seek address: %u, size: %u\r\n", *address, *blocks);
      return UBS_RSLT_OK;
    }
    *address += UBS_BLOCK_SIZE;
  } while (*address < UBS_END_ADDRESS);

  *blocks = 0;
  return UBS_RSLT_NOK;
}
/*
 * uBSFormat - erase all blocks
 */
void uBSFormat(void) {
  uint32_t address = UBS_START_ADDRESS;
  uint8_t writeBuf[UBS_BLOCK_SIZE]; // Assume 0x0

  while (address < UBS_END_ADDRESS) {
    uBSWriteBlock(address, &writeBuf[0], UBS_BLOCK_SIZE);
    //DBG("uBS Format: %u\r\n", address);
    address += UBS_BLOCK_SIZE;
  }
}
/*
 * uBSInit
 *
 * Calculate free space on start
 */
void uBSInit(void) {
  uint32_t address = UBS_START_ADDRESS;
  uint8_t readBuf[UBS_HEADER_SIZE];

  uBSFreeSpace = 0;
  uBSFreeBlocks = 0;

  while (address < UBS_END_ADDRESS) {
    uBSReadBlock(address, &readBuf[0], UBS_HEADER_SIZE);
    if (readBuf[0] == 0) {
      uBSFreeSpace += UBS_DATA_SIZE;
      uBSFreeBlocks++;
    }
    //DBG("uBS Init: %u:%u:%u\r\n", address, readBuf[0], uBSFreeSpace);
    address += UBS_BLOCK_SIZE;
  }
  if (uBSFreeSpace > 0) uBSFreeSpace -= UBS_NAME_SIZE; // Remove one name size of master block
}
/*
 * uBSWrite
 */
int8_t uBSWrite(void* name, uint8_t nameSize, void *data, uint16_t dataSize) {
  uint16_t curSize, newBlocks, oldBlocks;
  uint32_t address, nextBlock, eraseBlock;
  uint8_t writeBuf[UBS_BLOCK_SIZE];
  uint8_t readBuf[UBS_HEADER_BLOCK_SIZE];

  if (name == NULL) return UBS_RSLT_NOK;
  if (nameSize > UBS_NAME_SIZE) return UBS_RSLT_NOK;
  if (nameSize == 0) return UBS_RSLT_NOK;
  if (data == NULL) return UBS_RSLT_NOK;
  if (dataSize == 0) return UBS_RSLT_NOK;

  // Get new block size
  newBlocks = (dataSize + UBS_NAME_SIZE) / UBS_DATA_SIZE;
  if ((dataSize + UBS_NAME_SIZE) % UBS_DATA_SIZE) newBlocks++;

  if (uBSSeekNameGetBlockSize(name, nameSize, &address, &oldBlocks) == UBS_RSLT_NOK) {
    address = UBS_END_ADDRESS;
    uBSGetFreeBlock(&address);
  }

  DBG("uBS Write address %u, newBlocks: %u, oldBlocks:%u\r\n", address, newBlocks, oldBlocks);

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
          uBSReadBlock(address, &readBuf[0], UBS_HEADER_BLOCK_SIZE);
          for (uint8_t i = 0; i < UBS_POINTER_SIZE; i++) {
            nextBlock += (readBuf[1 + i] << ((UBS_POINTER_SIZE - i - 1) * 8));
          }
        }
      } else {
        // Store old block pointer to erase
        uBSReadBlock(address, &readBuf[0], UBS_HEADER_BLOCK_SIZE);
        eraseBlock = 0;
        for (uint8_t i = 0; i < UBS_POINTER_SIZE; i++) {
          eraseBlock += (readBuf[1 + i] << ((UBS_POINTER_SIZE - i - 1) * 8));
        }
      }
    } else {
      // Last new block no pointer
      if (block < (newBlocks - 1)) {
        nextBlock = address;
        uBSGetFreeBlock(&nextBlock);
      }
    }
    for (uint8_t i = 0; i < UBS_POINTER_SIZE; i++) {
      writeBuf[1 + i] = (nextBlock >> ((UBS_POINTER_SIZE - i - 1) * 8)) & 0xFF;
    }
    DBG("|%u,%u,%u, size %u, next %u|-", writeBuf[0], writeBuf[1], writeBuf[2], curSize, nextBlock);

    // Copy name and/or data
    if (block == 0) {
      memcpy(&writeBuf[UBS_HEADER_BLOCK_SIZE], name, nameSize);
      memcpy(&writeBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE], data, curSize);
    } else {
      memcpy(&writeBuf[UBS_HEADER_BLOCK_SIZE], data, curSize);
    }
    for (uint8_t i = 0; i < UBS_BLOCK_SIZE; i++) DBG("%u-", writeBuf[i]);
    DBG("uBS Write data: %u:%u\r\n", block, address);
    uBSWriteBlock(address, &writeBuf[0], UBS_BLOCK_SIZE);

    address = nextBlock;
    (data) += curSize;
    dataSize -= curSize;
  }

  // Erase old not used blocks
  memset(&writeBuf[0], 0x0, UBS_BLOCK_SIZE);
  for (uint8_t block = newBlocks; block < oldBlocks; block++) {
    address = eraseBlock;
    uBSReadBlock(address, &readBuf[0], UBS_HEADER_BLOCK_SIZE);
    eraseBlock = 0;
    for (uint8_t i = 0; i < UBS_POINTER_SIZE; i++) {
      eraseBlock += (readBuf[1 + i] << ((UBS_POINTER_SIZE - i - 1) * 8));
    }
    DBG("uBS Write erase: %u:%u\r\n", block, address);
    uBSWriteBlock(address, &writeBuf[0], UBS_BLOCK_SIZE);
    uBSFreeSpace += UBS_DATA_SIZE;
    uBSFreeBlocks++;
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

  DBG("uBS Read name: %s, readSize %u\r\n", name, readSize);
  if (uBSSeekName(name, nameSize, &address) == UBS_RSLT_NOK) {
    *dataSize = 0;
    return UBS_RSLT_NOT_FOUND;
  }

  do {
    uBSReadBlock(address, &readBuf[0], UBS_BLOCK_SIZE);
    for (uint8_t i = 0; i < UBS_BLOCK_SIZE; i++) DBG("%u-", readBuf[i]); DBG("\r\n");

    curSize = UBS_GET_BLOCK_DATA_SIZE(readBuf[0]);
    if (curSize > *dataSize) curSize = *dataSize;

    DBG("uBS Read address: %u, size %u\r\n", address, curSize);
    if (block == 0) {
      memcpy(data, &readBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE], curSize);
    } else {
      memcpy(data, &readBuf[UBS_HEADER_BLOCK_SIZE], curSize);
    }

    address = 0;
    for (uint8_t i = 0; i < UBS_POINTER_SIZE; i++) {
      address += (readBuf[1 + i] << ((UBS_POINTER_SIZE - i - 1) * 8));
    }
    block++;
    *dataSize -= curSize;
    data += curSize;
    readSize += curSize;
    DBG("uBS Read dataSize: %u, readSize %u\r\n", *dataSize, readSize);
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

  if (*address == 0) *address = UBS_START_ADDRESS;
  else *address += UBS_BLOCK_SIZE;
  if (nameSize > UBS_NAME_SIZE) nameSize = UBS_NAME_SIZE;

  DBG("uBS Seek all address: %u\r\n", *address);
  do {
    uBSReadBlock(*address, &readBuf[0], UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE);
    if (UBS_GET_MASTER_FLAG(readBuf[0])) {
      memcpy(name, &readBuf[UBS_HEADER_BLOCK_SIZE], nameSize);
      DBG("uBS Seek all found %s, address: %u\r\n", name, *address);
      return UBS_RSLT_OK;
    }
    *address += UBS_BLOCK_SIZE;
  } while (*address < UBS_END_ADDRESS);

  return UBS_RSLT_NOK;
}
/*
 * uBSRename - Update master block name.
 *
 */
int8_t uBSRename(void* oldName, void* newName, uint8_t nameSize) {
  uint8_t readBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE];
  uint32_t address = UBS_START_ADDRESS;
  if (nameSize > UBS_NAME_SIZE) nameSize = UBS_NAME_SIZE;

  do {
    uBSReadBlock(address, &readBuf[0], UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE);
    DBG("uBS rename address: %u\r\n", address);
    if ((UBS_GET_MASTER_FLAG(readBuf[0])) &&
        (memcmp(&readBuf[UBS_HEADER_BLOCK_SIZE], oldName, nameSize) == 0)) {
      DBG("uBS rename found %s, address: %u\r\n", oldName, address);
      memset(&readBuf[UBS_HEADER_BLOCK_SIZE], 0x0, UBS_NAME_SIZE);
      memcpy(&readBuf[UBS_HEADER_BLOCK_SIZE], newName, nameSize);
      uBSWriteBlock(address, &readBuf[0], nameSize);
      return UBS_RSLT_OK;
    }
    address += UBS_BLOCK_SIZE;
  } while (address < UBS_END_ADDRESS);

  return UBS_RSLT_NOK;
}
