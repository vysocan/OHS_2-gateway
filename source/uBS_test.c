/*
 * uBS Mock Test
 *
 * This file provides a mock implementation of the SPI FRAM and tests for uBS.
 * Compile and run on PC: gcc -o ubs_test uBS_test.c -DUBS_MOCK_TEST
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

/*
 * Mock SPI and HAL types
 */
typedef struct { int dummy; } SPIDriver;
SPIDriver SPID1;

#define osalDbgCheck(x) assert(x)

/*
 * Mock FRAM storage (128KB should be enough for 512 blocks * 128 bytes)
 */
#define MOCK_FRAM_SIZE (128 * 1024)
static uint8_t mockFram[MOCK_FRAM_SIZE];
static uint32_t mockSpiAddress = 0;
static uint8_t mockSpiCmd = 0;

/*
 * Mock SPI functions
 */
void spiAcquireBus(SPIDriver* spip) { (void)spip; }
void spiReleaseBus(SPIDriver* spip) { (void)spip; }
void spiSelect(SPIDriver* spip) { (void)spip; }
void spiUnselect(SPIDriver* spip) { (void)spip; }

void spiSend(SPIDriver* spip, size_t n, const void* txbuf) {
  (void)spip;
  const uint8_t* buf = (const uint8_t*)txbuf;

  if (n >= 1) {
    mockSpiCmd = buf[0];
  }
  if (n >= 4) {
    // Extract 3-byte address
    mockSpiAddress = ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
  }
  // If this is a write command with data following the address
  if (mockSpiCmd == 0x02 && n > 4) {
    // Data follows address in same spiSend call
    memcpy(&mockFram[mockSpiAddress], &buf[4], n - 4);
  }
}

void spiReceive(SPIDriver* spip, size_t n, void* rxbuf) {
  (void)spip;
  if (mockSpiCmd == 0x03) { // READ command
    memcpy(rxbuf, &mockFram[mockSpiAddress], n);
  }
}

/*
 * Override spiSend to handle write data separately
 * The uBS code sends address first, then data in a second spiSend
 */
static uint8_t lastCmd = 0;

// We need to intercept the pattern: send(cmd+addr), send(data)
// So let's track state
#define spiSend spiSendMock
void spiSendMock(SPIDriver* spip, size_t n, const void* txbuf) {
  (void)spip;
  const uint8_t* buf = (const uint8_t*)txbuf;

  if (n == 1) {
    // Single byte command (WREN, etc.)
    lastCmd = buf[0];
    return;
  }

  if (n == 4) {
    // Command + 3-byte address
    mockSpiCmd = buf[0];
    mockSpiAddress = ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
    return;
  }

  // Data transfer for write
  if (mockSpiCmd == 0x02) { // WRITE
    memcpy(&mockFram[mockSpiAddress], buf, n);
  }
}

/*
 * Mock chprintf (debug output)
 */
typedef void* BaseSequentialStream;
void* SD3 = NULL;
void chprintf(BaseSequentialStream* chp, const char* fmt, ...) {
  (void)chp;
  (void)fmt;
  // Suppress debug output in tests
}

/*
 * Include uBS configuration
 */
#define UBS_USE_FREE_MAP      1
#define UBS_USE_MASTER_MAP    1
#define UBS_BLOCK_SIZE        128
#define UBS_BLOCK_COUNT       512
#define UBS_NAME_SIZE         16
#define UBS_ADDRESS_START     65536

#define UBS_ADDRESS_END       (UBS_ADDRESS_START + (UBS_BLOCK_SIZE * UBS_BLOCK_COUNT))
#define UBS_HEADER_SIZE       1
#define UBS_ADDRESS_SIZE      3
#define UBS_HEADER_BLOCK_SIZE (UBS_HEADER_SIZE + UBS_ADDRESS_SIZE)
#define UBS_HEADER_ALLOW_SIZE 127
#define UBS_DATA_SIZE         (UBS_BLOCK_SIZE - UBS_HEADER_BLOCK_SIZE)
#define UBS_FIRST_DATA_SIZE   (UBS_DATA_SIZE - UBS_NAME_SIZE)
#define UBS_CMD_BUF_SIZE      (1 + UBS_ADDRESS_SIZE)
#define UBS_MAP_SIZE          (UBS_BLOCK_COUNT/8) + ((UBS_BLOCK_COUNT%8) ? 1 : 0)
#define UBS_SPACE_MAX         ((UBS_BLOCK_COUNT * UBS_DATA_SIZE) - UBS_NAME_SIZE)

#define UBS_RSLT_OK        (1)
#define UBS_RSLT_NOK       (0)
#define UBS_RSLT_NOT_FOUND (-1)
#define UBS_RSLT_TOO_LARGE (-2)
#define UBS_RSLT_DAMAGED   (-127)

#define CMD_25AA_WRSR     0x01
#define CMD_25AA_WRITE    0x02
#define CMD_25AA_READ     0x03
#define CMD_25AA_WRDI     0x04
#define CMD_25AA_RDSR     0x05
#define CMD_25AA_WREN     0x06
#define CMD_25AA_RDID     0x9F

/*
 * uBS variables
 */
static uint8_t uBSCmdBuf[UBS_CMD_BUF_SIZE];
uint32_t uBSFreeSpace = 0;
uint16_t uBSFreeBlocks = 0;
static uint8_t uBSFreeMap[UBS_MAP_SIZE];
static uint8_t uBSMasterMap[UBS_MAP_SIZE];

/*
 * uBS Macros
 */
#define UBS_GET_MASTER_FLAG(x)     ((x) & 0b1)
#define UBS_GET_BLOCK_DATA_SIZE(x) ((x >> 1U) & UBS_HEADER_ALLOW_SIZE)
#define UBS_SET_MASTER_FLAG(x)       x |= 1
#define UBS_SET_BLOCK_DATA_SIZE(x,y) x |= (y << 1U)
#define UBS_ADDR_TO_BUF(buf, addr) {\
                                     (buf)[0] = (uint8_t)((addr) >> 16);\
                                     (buf)[1] = (uint8_t)((addr) >> 8);\
                                     (buf)[2] = (uint8_t)(addr);\
                                   }
#define UBS_BUF_TO_ADDR(buf)       ((((uint32_t)(buf)[0]) << 16) | (((uint32_t)(buf)[1]) << 8) | ((uint32_t)(buf)[2]))

#define UBS_DEBUG 1
#define DBG(...) printf(__VA_ARGS__)

/*
 * Include uBS implementation inline (simplified for test)
 */
static int8_t uBSReadBlock(uint32_t address, uint8_t *data, uint8_t size) {
  osalDbgCheck(size <= UBS_BLOCK_SIZE);
  if ((address < UBS_ADDRESS_START) || (address > UBS_ADDRESS_END)) return UBS_RSLT_NOK;

  uBSCmdBuf[0] = CMD_25AA_READ;
  UBS_ADDR_TO_BUF(&uBSCmdBuf[1], address);

  spiAcquireBus(&SPID1);
  spiSelect(&SPID1);
  spiSend(&SPID1, UBS_CMD_BUF_SIZE, uBSCmdBuf);
  spiReceive(&SPID1, size, data);
  spiUnselect(&SPID1);
  spiReleaseBus(&SPID1);

  return UBS_RSLT_OK;
}

static void uBSWriteBlock(uint32_t address, uint8_t* data, uint8_t size) {
  osalDbgCheck(size <= UBS_BLOCK_SIZE);

  spiAcquireBus(&SPID1);

  spiSelect(&SPID1);
  uBSCmdBuf[0] = CMD_25AA_WREN;
  spiSend(&SPID1, 1, uBSCmdBuf);
  spiUnselect(&SPID1);

  uBSCmdBuf[0] = CMD_25AA_WRITE;
  UBS_ADDR_TO_BUF(&uBSCmdBuf[1], address);

  spiSelect(&SPID1);
  spiSend(&SPID1, UBS_CMD_BUF_SIZE, uBSCmdBuf);
  spiSend(&SPID1, size, data);
  spiUnselect(&SPID1);

  spiReleaseBus(&SPID1);
}

static int8_t uBSGetFreeBlock(uint32_t* address) {
  uint8_t readBuf[UBS_HEADER_SIZE];
  uint32_t toSkip = *address;
  *address = UBS_ADDRESS_START;

  DBG("uBS free address skip: %u\r\n", toSkip);
  while (*address < UBS_ADDRESS_END) {
    if (*address != toSkip) {
      #if UBS_USE_FREE_MAP > 0
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

void uBSFormat(void) {
  uint32_t address = UBS_ADDRESS_START;
  uint8_t writeBuf[UBS_BLOCK_SIZE] = {0};

  while (address < UBS_ADDRESS_END) {
    uBSWriteBlock(address, &writeBuf[0], UBS_BLOCK_SIZE);
    address += UBS_BLOCK_SIZE;
  }
  #if UBS_USE_FREE_MAP > 0
    memset(&uBSFreeMap[0], 255, UBS_MAP_SIZE);
  #endif
  #if UBS_USE_MASTER_MAP > 0
    memset(&uBSMasterMap[0], 0, UBS_MAP_SIZE);
  #endif
}

int8_t uBSInit(void) {
  uint32_t address = UBS_ADDRESS_START;
  uint8_t readBuf[UBS_HEADER_SIZE];

  uBSFreeSpace = 0;
  uBSFreeBlocks = 0;

  #if UBS_USE_FREE_MAP > 0
    memset(&uBSFreeMap[0], 0, UBS_MAP_SIZE);
  #endif
  #if UBS_USE_MASTER_MAP > 0
    memset(&uBSMasterMap[0], 0, UBS_MAP_SIZE);
  #endif

  while (address < UBS_ADDRESS_END) {
    if (uBSReadBlock(address, &readBuf[0], UBS_HEADER_SIZE) != UBS_RSLT_OK)
      return UBS_RSLT_DAMAGED;
    if (readBuf[0] == 0) {
      uBSFreeSpace += UBS_DATA_SIZE;
      uBSFreeBlocks++;
      #if UBS_USE_FREE_MAP > 0
        uBSFreeMap[((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) / 8] |=
            (1 << (((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) % 8));
      #endif
    }
    #if UBS_USE_MASTER_MAP > 0
      if (UBS_GET_MASTER_FLAG(readBuf[0])) {
        uBSMasterMap[((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) / 8] |=
              (1 << (((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) % 8));
      }
    #endif
    address += UBS_BLOCK_SIZE;
  }
  if (uBSFreeSpace > 0) uBSFreeSpace -= UBS_NAME_SIZE;

  DBG("uBS Init, free blocks: %u, free space:%u\r\n", uBSFreeBlocks, uBSFreeSpace);

  return UBS_RSLT_OK;
}

static int8_t uBSSeekName(void* blockName, uint8_t nameSize, uint32_t* address) {
  uint8_t readBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE];

  *address = UBS_ADDRESS_START;
  do {
    if (uBSReadBlock(*address, &readBuf[0], UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE) != UBS_RSLT_OK)
      return UBS_RSLT_DAMAGED;
    if ((UBS_GET_MASTER_FLAG(readBuf[0])) &&
        (memcmp(&readBuf[UBS_HEADER_BLOCK_SIZE], blockName, nameSize) == 0)) {
      return UBS_RSLT_OK;
    }
    *address += UBS_BLOCK_SIZE;
  } while (*address < UBS_ADDRESS_END);

  return UBS_RSLT_NOT_FOUND;
}

static int8_t uBSSeekNameGetBlockSize(void* blockName, uint8_t nameSize, uint32_t* address, uint16_t* blocks) {
  uint8_t readBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE];
  uint32_t nextBlock = 0;

  *address = UBS_ADDRESS_START;
  *blocks = 0;
  do {
    if (uBSReadBlock(*address, &readBuf[0], UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE) != UBS_RSLT_OK)
      return UBS_RSLT_DAMAGED;
    if ((UBS_GET_MASTER_FLAG(readBuf[0])) &&
        (memcmp(&readBuf[UBS_HEADER_BLOCK_SIZE], blockName, nameSize) == 0)) {
      *blocks = 1;
      nextBlock = UBS_BUF_TO_ADDR(&readBuf[1]);
      while (nextBlock) {
        if (uBSReadBlock(nextBlock, &readBuf[0], UBS_HEADER_BLOCK_SIZE) != UBS_RSLT_OK)
          return UBS_RSLT_DAMAGED;
        (*blocks)++;
        nextBlock = UBS_BUF_TO_ADDR(&readBuf[1]);
      }
      return UBS_RSLT_OK;
    }
    *address += UBS_BLOCK_SIZE;
  } while (*address < UBS_ADDRESS_END);

  *blocks = 0;
  return UBS_RSLT_NOT_FOUND;
}

static int8_t uBSEraseBlocks(uint32_t address, uint16_t eraseBlocks) {
  uint8_t writeBuf[UBS_BLOCK_SIZE] = {0};
  uint8_t readBuf[UBS_HEADER_BLOCK_SIZE];
  uint32_t nextBlock;
  #if UBS_USE_MASTER_MAP > 0
  uint32_t masterAddr = address;
  #endif

  for (uint16_t block = 0; block < eraseBlocks; block++) {
    if (uBSReadBlock(address, &readBuf[0], UBS_HEADER_BLOCK_SIZE) != UBS_RSLT_OK)
      return UBS_RSLT_DAMAGED;
    nextBlock = UBS_BUF_TO_ADDR(&readBuf[1]);
    uBSWriteBlock(address, &writeBuf[0], UBS_BLOCK_SIZE);
    uBSFreeSpace += UBS_DATA_SIZE;
    uBSFreeBlocks++;
    #if UBS_USE_FREE_MAP > 0
      uBSFreeMap[((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) / 8] |=
           1 << (((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) % 8);
    #endif
    address = nextBlock;
  }
  #if UBS_USE_MASTER_MAP > 0
    uBSMasterMap[((masterAddr - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) / 8] &=
        ~(1 << (((masterAddr - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) % 8));
  #endif

  return UBS_RSLT_OK;
}

int8_t uBSDelete(void* name, uint8_t nameSize) {
  uint32_t address;
  uint16_t blocks;

  if (name == NULL) return UBS_RSLT_NOK;
  if (nameSize > UBS_NAME_SIZE) return UBS_RSLT_NOK;
  if (nameSize == 0) return UBS_RSLT_NOK;

  if (uBSSeekNameGetBlockSize(name, nameSize, &address, &blocks) != UBS_RSLT_OK) return UBS_RSLT_NOT_FOUND;

  return uBSEraseBlocks(address, blocks);
}

int8_t uBSWrite(void* name, uint8_t nameSize, void *data, uint16_t dataSize) {
  uint16_t curSize, newBlocks, oldBlocks = 0;
  uint32_t address, nextBlock, eraseBlock;
  uint8_t  writeBuf[UBS_BLOCK_SIZE];
  uint8_t  readBuf[UBS_HEADER_BLOCK_SIZE];

  if (name == NULL) return UBS_RSLT_NOK;
  if (nameSize > UBS_NAME_SIZE) return UBS_RSLT_NOK;
  if (nameSize == 0) return UBS_RSLT_NOK;

  newBlocks = (dataSize + UBS_NAME_SIZE) / UBS_DATA_SIZE;
  if ((dataSize + UBS_NAME_SIZE) % UBS_DATA_SIZE) newBlocks++;

  if (uBSSeekNameGetBlockSize(name, nameSize, &address, &oldBlocks) != UBS_RSLT_OK) {
    address = UBS_ADDRESS_END;
    uBSGetFreeBlock(&address);
  }

  if ((int16_t)(newBlocks - oldBlocks) > uBSFreeBlocks) return UBS_RSLT_TOO_LARGE;

  for (uint8_t block = 0; block < newBlocks; block++) {
    memset(&writeBuf[0], 0x0, UBS_BLOCK_SIZE);

    if (block == 0) {
      UBS_SET_MASTER_FLAG(writeBuf[0]);
      if (dataSize > UBS_FIRST_DATA_SIZE) curSize = UBS_FIRST_DATA_SIZE;
      else curSize = dataSize;
    } else {
      if (dataSize > UBS_DATA_SIZE) curSize = UBS_DATA_SIZE;
      else curSize = dataSize;
    }
    UBS_SET_BLOCK_DATA_SIZE(writeBuf[0], curSize);

    nextBlock = 0;
    if (block < oldBlocks) {
      if (block < (newBlocks - 1)) {
        if (block == (oldBlocks - 1)) {
          nextBlock = address;
          uBSGetFreeBlock(&nextBlock);
        } else {
          if (uBSReadBlock(address, &readBuf[0], UBS_HEADER_BLOCK_SIZE) != UBS_RSLT_OK)
            return UBS_RSLT_DAMAGED;
          nextBlock = UBS_BUF_TO_ADDR(&readBuf[1]);
        }
      } else {
        if (uBSReadBlock(address, &readBuf[0], UBS_HEADER_BLOCK_SIZE) != UBS_RSLT_OK)
          return UBS_RSLT_DAMAGED;
        eraseBlock = UBS_BUF_TO_ADDR(&readBuf[1]);
      }
    } else {
      if (block < (newBlocks - 1)) {
        nextBlock = address;
        uBSGetFreeBlock(&nextBlock);
      }
    }
    UBS_ADDR_TO_BUF(&writeBuf[1], nextBlock);

    if (block == 0) {
      memcpy(&writeBuf[UBS_HEADER_BLOCK_SIZE], name, nameSize);
      memcpy(&writeBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE], data, curSize);
    } else {
      memcpy(&writeBuf[UBS_HEADER_BLOCK_SIZE], data, curSize);
    }
    uBSWriteBlock(address, &writeBuf[0], UBS_BLOCK_SIZE);
    DBG("uBS Write data: %u:%u\r\n", block, address);
    #if UBS_USE_MASTER_MAP > 0
      if (block == 0) {
        uBSMasterMap[((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) / 8] |=
            (1 << (((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) % 8));
      }
    #endif

    address = nextBlock;
    data += curSize;
    dataSize -= curSize;
  }

  memset(&writeBuf[0], 0x0, UBS_BLOCK_SIZE);
  for (uint8_t block = newBlocks; block < oldBlocks; block++) {
    address = eraseBlock;
    if (uBSReadBlock(address, &readBuf[0], UBS_HEADER_BLOCK_SIZE) != UBS_RSLT_OK)
      return UBS_RSLT_DAMAGED;
    eraseBlock = UBS_BUF_TO_ADDR(&readBuf[1]);
    uBSWriteBlock(address, &writeBuf[0], UBS_BLOCK_SIZE);
    uBSFreeSpace += UBS_DATA_SIZE;
    uBSFreeBlocks++;
    #if UBS_USE_FREE_MAP > 0
      uBSFreeMap[((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) / 8] |=
           1 << (((address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) % 8);
    #endif
  }

  return UBS_RSLT_OK;
}

int8_t uBSRead(void* name, uint8_t nameSize, void *data, uint16_t *dataSize) {
  uint16_t block = 0, curSize, readSize = 0;
  uint32_t address;
  uint8_t  readBuf[UBS_BLOCK_SIZE];

  if (name == NULL) return UBS_RSLT_NOK;
  if (nameSize > UBS_NAME_SIZE) return UBS_RSLT_NOK;
  if (nameSize == 0) return UBS_RSLT_NOK;
  if (data == NULL) return UBS_RSLT_NOK;
  if (*dataSize == 0) return UBS_RSLT_NOK;

  memset(data, 0, *dataSize);

  if (uBSSeekName(name, nameSize, &address) != UBS_RSLT_OK) {
    *dataSize = 0;
    return UBS_RSLT_NOT_FOUND;
  }

  do {
    if (uBSReadBlock(address, &readBuf[0], UBS_BLOCK_SIZE) != UBS_RSLT_OK) return UBS_RSLT_DAMAGED;

    curSize = UBS_GET_BLOCK_DATA_SIZE(readBuf[0]);
    if (curSize > *dataSize) curSize = *dataSize;

    if (block == 0) {
      memcpy(data, &readBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE], curSize);
    } else {
      memcpy(data, &readBuf[UBS_HEADER_BLOCK_SIZE], curSize);
    }

    address = UBS_BUF_TO_ADDR(&readBuf[1]);
    block++;
    *dataSize -= curSize;
    data += curSize;
    readSize += curSize;
  } while ((address) && (*dataSize));

  *dataSize = readSize;
  DBG("uBS Read end size: %u\r\n", *dataSize);
  return UBS_RSLT_OK;
}

int8_t uBSSeekAll(uint32_t* address, void* name, uint8_t nameSize) {
  uint8_t readBuf[UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE];

  if (*address == 0) *address = UBS_ADDRESS_START;
  else *address += UBS_BLOCK_SIZE;
  if (nameSize > UBS_NAME_SIZE) nameSize = UBS_NAME_SIZE;

  #if UBS_USE_MASTER_MAP > 0
  do {
    if ((uBSMasterMap[((*address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) / 8] >>
        (((*address - UBS_ADDRESS_START) / UBS_BLOCK_SIZE) % 8)) & 0b1) {
      if (uBSReadBlock(*address, &readBuf[0], UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE) != UBS_RSLT_OK)
        return UBS_RSLT_DAMAGED;
      memcpy(name, &readBuf[UBS_HEADER_BLOCK_SIZE], nameSize);
      return UBS_RSLT_OK;
    }
    *address += UBS_BLOCK_SIZE;
  } while (*address < UBS_ADDRESS_END);
  #else
  do {
    if (uBSReadBlock(*address, &readBuf[0], UBS_HEADER_BLOCK_SIZE + UBS_NAME_SIZE) != UBS_RSLT_OK)
      return UBS_RSLT_DAMAGED;
    if (UBS_GET_MASTER_FLAG(readBuf[0])) {
      memcpy(name, &readBuf[UBS_HEADER_BLOCK_SIZE], nameSize);
      return UBS_RSLT_OK;
    }
    *address += UBS_BLOCK_SIZE;
  } while (*address < UBS_ADDRESS_END);
  #endif

  return UBS_RSLT_NOK;
}

/*
 * ============================================================================
 * TEST CASES
 * ============================================================================
 */

void test_format_and_init(void) {
  printf("Test: Format and Init... ");

  // Clear mock FRAM
  memset(mockFram, 0xFF, MOCK_FRAM_SIZE);

  // Format
  uBSFormat();

  // Init
  assert(uBSInit() == UBS_RSLT_OK);

  // Check free space
  assert(uBSFreeBlocks == UBS_BLOCK_COUNT);
  assert(uBSFreeSpace == UBS_SPACE_MAX);

  printf("PASSED\n");
}

void test_write_and_read_small(void) {
  printf("Test: Write and Read small file... ");

  memset(mockFram, 0xFF, MOCK_FRAM_SIZE);
  uBSFormat();
  uBSInit();

  char name[] = "test.txt";
  char data[] = "Hello, World!";
  char readBuf[64] = {0};
  uint16_t readSize = sizeof(readBuf);

  // Write
  assert(uBSWrite(name, strlen(name), data, strlen(data)) == UBS_RSLT_OK);

  // Read
  assert(uBSRead(name, strlen(name), readBuf, &readSize) == UBS_RSLT_OK);
  assert(readSize == strlen(data));
  assert(memcmp(readBuf, data, strlen(data)) == 0);

  printf("PASSED\n");
}

void test_write_and_read_large(void) {
  printf("Test: Write and Read large file (multi-block)... ");

  memset(mockFram, 0xFF, MOCK_FRAM_SIZE);
  uBSFormat();
  uBSInit();

  char name[] = "bigfile.dat";
  uint8_t data[500];
  uint8_t readBuf[500] = {0};
  uint16_t readSize = sizeof(readBuf);

  // Fill with pattern
  for (int i = 0; i < 500; i++) {
    data[i] = (uint8_t)(i & 0xFF);
  }

  // Write
  assert(uBSWrite(name, strlen(name), data, sizeof(data)) == UBS_RSLT_OK);

  // Read
  assert(uBSRead(name, strlen(name), readBuf, &readSize) == UBS_RSLT_OK);
  assert(readSize == sizeof(data));
  assert(memcmp(readBuf, data, sizeof(data)) == 0);

  printf("PASSED\n");
}

void test_delete(void) {
  printf("Test: Delete file... ");

  memset(mockFram, 0xFF, MOCK_FRAM_SIZE);
  uBSFormat();
  uBSInit();

  uint16_t initialFreeBlocks = uBSFreeBlocks;

  char name[] = "todelete.txt";
  char data[] = "This will be deleted";

  // Write
  assert(uBSWrite(name, strlen(name), data, strlen(data)) == UBS_RSLT_OK);
  assert(uBSFreeBlocks < initialFreeBlocks);

  // Delete
  assert(uBSDelete(name, strlen(name)) == UBS_RSLT_OK);
  assert(uBSFreeBlocks == initialFreeBlocks);

  // Try to read - should fail
  char readBuf[64];
  uint16_t readSize = sizeof(readBuf);
  assert(uBSRead(name, strlen(name), readBuf, &readSize) == UBS_RSLT_NOT_FOUND);

  printf("PASSED\n");
}

void test_seek_all(void) {
  printf("Test: Seek all files... ");

  memset(mockFram, 0xFF, MOCK_FRAM_SIZE);
  uBSFormat();
  uBSInit();

  // Write 3 files
  char name1[] = "file1.txt";
  char name2[] = "file2.txt";
  char name3[] = "file3.txt";
  char data[] = "some data";

  assert(uBSWrite(name1, strlen(name1), data, strlen(data)) == UBS_RSLT_OK);
  assert(uBSWrite(name2, strlen(name2), data, strlen(data)) == UBS_RSLT_OK);
  assert(uBSWrite(name3, strlen(name3), data, strlen(data)) == UBS_RSLT_OK);

  // Seek all
  uint32_t addr = 0;
  char foundName[UBS_NAME_SIZE] = {0};
  int count = 0;

  while (uBSSeekAll(&addr, foundName, UBS_NAME_SIZE) == UBS_RSLT_OK) {
    count++;
    memset(foundName, 0, UBS_NAME_SIZE);
  }

  assert(count == 3);

  printf("PASSED\n");
}

void test_master_map_update(void) {
  printf("Test: Master map update on write/delete... ");

  memset(mockFram, 0xFF, MOCK_FRAM_SIZE);
  uBSFormat();
  uBSInit();

  char name[] = "maptest.txt";
  char data[] = "testing master map";

  // Initially no master bits set
  int masterBits = 0;
  for (int i = 0; i < UBS_MAP_SIZE; i++) {
    for (int b = 0; b < 8; b++) {
      if (uBSMasterMap[i] & (1 << b)) masterBits++;
    }
  }
  assert(masterBits == 0);

  // Write - should set master bit
  assert(uBSWrite(name, strlen(name), data, strlen(data)) == UBS_RSLT_OK);

  masterBits = 0;
  for (int i = 0; i < UBS_MAP_SIZE; i++) {
    for (int b = 0; b < 8; b++) {
      if (uBSMasterMap[i] & (1 << b)) masterBits++;
    }
  }
  assert(masterBits == 1);

  // Delete - should clear master bit
  assert(uBSDelete(name, strlen(name)) == UBS_RSLT_OK);

  masterBits = 0;
  for (int i = 0; i < UBS_MAP_SIZE; i++) {
    for (int b = 0; b < 8; b++) {
      if (uBSMasterMap[i] & (1 << b)) masterBits++;
    }
  }
  assert(masterBits == 0);

  printf("PASSED\n");
}

void test_free_map_update(void) {
  printf("Test: Free map update on allocation/deallocation... ");

  memset(mockFram, 0xFF, MOCK_FRAM_SIZE);
  uBSFormat();
  uBSInit();

  uint16_t initialFreeBlocks = uBSFreeBlocks;

  char name[] = "freetest.txt";
  char data[] = "testing free map";

  // Write - should allocate block
  assert(uBSWrite(name, strlen(name), data, strlen(data)) == UBS_RSLT_OK);
  assert(uBSFreeBlocks == initialFreeBlocks - 1);

  // Delete - should free block
  assert(uBSDelete(name, strlen(name)) == UBS_RSLT_OK);
  assert(uBSFreeBlocks == initialFreeBlocks);

  printf("PASSED\n");
}

int main(void) {
  printf("\n=== uBS Mock Tests ===\n\n");

  test_format_and_init();
  test_write_and_read_small();
  test_write_and_read_large();
  test_delete();
  test_seek_all();
  test_master_map_update();
  test_free_map_update();

  printf("\n=== All tests PASSED! ===\n\n");

  return 0;
}
