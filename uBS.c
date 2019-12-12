/*
 * uBS - micro Block System for FRAM
 * Adam Baron 2020
 *
 * BLOCK:
 * 0123456789~~~01
 * H............PP
 * H - Header
 * . - Data
 * P - Pointer to next block
 *
 * Header bits
 * 76543210
 * SSSSSSSM
 * M - Master block flag 1
 * S - Size up to 128 blocks, 0 empty block
 *
 * Master block
 *
 *
 */

#include "uBS.h"

/*
 * uBS data block definitions
 */
// Editable
#define UBS_BLOCK_SIZE       32U
#define UBS_BLOCK_COUNT      64U
#define UBS_NAME_SIZE        10U
#define UBS_START_ADDRESS    0U
// Fixed
#define UBS_END_ADDRESS      (UBS_START_ADDRESS + (UBS_BLOCK_SIZE * UBS_BLOCK_COUNT))
#define UBS_POINTER_SIZE     2U
#define UBS_HEADER_SIZE      1U
#define UBS_DATA_SIZE        (UBS_BLOCK_SIZE - UBS_POINTER_SIZE - UBS_HEADER_SIZE)
#define UBS_MAX_DATA_SIZE    ((UBS_DATA_SIZE * 128U) - UBS_NAME_SIZE)
#define UBS_MASTER_DATA_SIZE (UBS_DATA_SIZE - UBS_HEADER_SIZE - UBS_NAME_SIZE)
/*
 * Sanity checks
 */
#if UBS_NAME_SIZE > UBS_DATA_SIZE
  #error Size of master block name larger then block data size!
#endif
/*
 * Macros
 */
#define UBS_GET_MASTER_FLAG(x)     ((x) & 0b1)
#define UBS_GET_BLOCK_DATA_SIZE(x) ((x >> 1U) & 0b1111111)

#define UBS_SET_MASTER_FLAG(x,y)     (((x)&((0b11111111)^(0b00000001)))|((y)&(0b00000001)))
#define UBS_SET_BLOCK_DATA_SIZE(x,y) (((x)&((0b11111111)^(0b11111110)))|((y << 1U)&(0b11111110)))
/*
 * Defines
 */
#define UBS_RSLT_OK        (1)
#define UBS_RSLT_NOK       (-1)
#define UBS_RSLT_TOO_LARGE (-2)
#define UBS_NO_SAPCE       (-3)

static uint8_t uBSWriteBuf[UBS_BLOCK_SIZE + 3];
static uint8_t uBSReadBuf[UBS_BLOCK_SIZE];
static uint16_t uBSFreeSpace = 0;


void uBSReadBlock(uint32_t address) {
  uBSWriteBuf[0] = CMD_25AA_READ;
  uBSWriteBuf[1] = (address >> 8) & 0xFF;
  uBSWriteBuf[2] = address & 0xFF;

  spiAcquireBus(&SPID1);              // Acquire ownership of the bus.
  spiSelect(&SPID1);                  // Slave Select assertion.
  spiSend(&SPID1, 3, uBSWriteBuf);  // Send read command
  spiReceive(&SPID1, UBS_BLOCK_SIZE, uBSReadBuf);
  spiUnselect(&SPID1);                // Slave Select de-assertion.
  spiReleaseBus(&SPID1);              // Ownership release.
}

void uBSWriteBlock(uint32_t address, uint8_t* data) {
  spiAcquireBus(&SPID1);

  spiSelect(&SPID1);
  uBSWriteBuf[0] = CMD_25AA_WREN;
  spiSend(&SPID1, 1, uBSWriteBuf);
  spiUnselect(&SPID1);

  uBSWriteBuf[0] = CMD_25AA_WRITE;
  uBSWriteBuf[1] = (address >> 8) & 0xFF;
  uBSWriteBuf[2] = address & 0xFF;
  memcpy(&uBSWriteBuf[3], data, UBS_BLOCK_SIZE);
  spiSelect(&SPID1);
  spiSend(&SPID1, UBS_BLOCK_SIZE, uBSWriteBuf);
  spiUnselect(&SPID1);
  spiReleaseBus(&SPID1);
}

void uBSInit() {
  uint16_t address = UBS_START_ADDRESS;

  // Calculate free space on start
  do {
    uBSReadBlock(address);
    if (UBS_GET_BLOCK_DATA_SIZE(uBSReadBuf[0])) uBSFreeSpace += UBS_DATA_SIZE;
    address += UBS_BLOCK_SIZE;
  } while (address < UBS_END_ADDRESS);
  if (uBSFreeSpace > 0) uBSFreeSpace -= UBS_HEADER_SIZE - UBS_NAME_SIZE; // Remove one name size of master block
}

int8_t uBSGetFreeBlock(uint32_t* address) {
  *address = UBS_START_ADDRESS;
  do {
    uBSReadBlock(*address);
    *address += UBS_BLOCK_SIZE;
  } while ((UBS_GET_BLOCK_DATA_SIZE(uBSReadBuf[0])) & (*address < UBS_END_ADDRESS));
  if (*address < UBS_END_ADDRESS) return UBS_RSLT_OK;
  else return UBS_RSLT_NOK;
}

int8_t uBSWrite(void* blockname, void *data, uint16_t size) {
  uint16_t currentSize, dataStored;
  uint8_t block[UBS_BLOCK_SIZE];
  uint32_t address;

  if (blockname == NULL) return UBS_RSLT_NOK;
  if (strlen(blockname) > UBS_NAME_SIZE) return UBS_RSLT_NOK;
  if (size > UBS_MAX_DATA_SIZE) return UBS_RSLT_TOO_LARGE;
  if (size > uBSFreeSpace) return UBS_NO_SAPCE;

  // Master block
  memset(&block[0], 0x0, UBS_BLOCK_SIZE);

  if (size > UBS_MASTER_DATA_SIZE) currentSize = UBS_MASTER_DATA_SIZE;
  else currentSize = size;

  block[0] = 1 | (currentSize << 1);
  block[UBS_HEADER_SIZE] = strlen(blockname);
  memcpy(&block[UBS_HEADER_SIZE + UBS_HEADER_SIZE], blockname, strlen(blockname));
  memcpy(&block[UBS_HEADER_SIZE + UBS_HEADER_SIZE + UBS_NAME_SIZE], data, currentSize);
  data += currentSize;
  size -= currentSize;

  if (uBSGetFreeBlock(&address) == UBS_RSLT_OK) {
    uBSWriteBlock(address, &block[0]);
    uBSFreeSpace -= UBS_DATA_SIZE;
  } else return UBS_RSLT_NOK;

  // Rest of block(s)
  while (size > 0) {
    memset(&block[0], 0x0, UBS_BLOCK_SIZE);

    if (size > UBS_DATA_SIZE) currentSize = UBS_DATA_SIZE;
    else currentSize = size;

    block[0] = 1 | (currentSize << 1);
    memcpy(&block[UBS_HEADER_SIZE], data, currentSize);
    data += currentSize;
    size -= currentSize;

    if (uBSGetFreeBlock(&address) == UBS_RSLT_OK) {
      uBSWriteBlock(address, &block[0]);
      uBSFreeSpace -= UBS_DATA_SIZE;
    } else return UBS_RSLT_NOK; // TODO: This needs some undo.
  }

  return UBS_RSLT_OK;
}



