/*
 * Created on: Dec 21, 2025
 * Author: vysocan
 *
 * Hierarchical command dispatcher for embedded systems.
 */

#ifndef SOURCE_CMD_DISPATCHER_H_
#define SOURCE_CMD_DISPATCHER_H_

#include <stdint.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "chprintf.h"

/* Command dispatcher definitions */
#define SMD_STRICT_MODE FALSE  /* If TRUE, unknown commands return error instead of help */
/* Token buffer - adjust for your max command length */
#define CMD_MAX_TOKENS 5
#define CMD_TOKEN_LEN 16
/* Macro to get array count */
#define ARRAY_COUNT(arr) ((uint8_t)(sizeof(arr) / sizeof((arr)[0])))

/* Return codes */
typedef enum {
  CMD_OK = 1,
  CMD_UNKNOWN = 0,
  CMD_INVALID_ARGS = -1,
  CMD_INCOMPLETE = -2,
  CMD_ERROR = -3
} cmdStatus_t;

/* Command handler function signature */
typedef cmdStatus_t (*cmdHandler_t)(const char *tokens[], uint8_t tokenCount,
    char *result, uint16_t resultLen);

/* Forward declaration */
struct cmd_entry;

/* Command entry in lookup table (statically typed tree node) */
typedef struct cmd_entry {
  const char *name; /* command or subcommand name */
  cmdHandler_t handler; /* may be NULL if purely namespace */
  const char *help; /* short help for this command */
  const struct cmd_entry *sub; /* pointer to subcommand table */
  uint8_t sub_count; /* length of sub table */
} cmdEntry_t;

/* API Functions */
cmdStatus_t cmdProcess(char *input, const cmdEntry_t *table,
    uint8_t table_count, char *result, uint16_t resultLen);
cmdStatus_t cmdTokenize(char *input, const char *tokens[],
    uint8_t *tokenCount, uint8_t maxTokens);
cmdStatus_t cmdHandleHelp(const char *tokens[], uint8_t tokenCount,
    char *result, uint16_t resultLen);
void cmdInitHelp(const cmdEntry_t *table, uint8_t count);

/* Help functions */
int8_t strcmpi(const char *a, const char *b);
void safeStrcat(char *dest, uint16_t *len, uint16_t max, const char *src);

#endif /* SOURCE_CMD_DISPATCHER_H_ */
