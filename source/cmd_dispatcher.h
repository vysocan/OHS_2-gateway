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
} cmd_status_t;

/* Command handler function signature */
typedef cmd_status_t (*cmd_handler_t)(const char *tokens[], uint8_t token_count,
    char *result, uint16_t result_len);

/* Forward declaration */
struct cmd_entry;

/* Command entry in lookup table (statically typed tree node) */
typedef struct cmd_entry {
  const char *name; /* command or subcommand name */
  cmd_handler_t handler; /* may be NULL if purely namespace */
  const char *help; /* short help for this command */
  const struct cmd_entry *sub; /* pointer to subcommand table */
  uint8_t sub_count; /* length of sub table */
} cmd_entry_t;

/* API Functions */
cmd_status_t cmdProcess(char *input, const cmd_entry_t *table,
    uint8_t table_count, char *result, uint16_t result_len);
cmd_status_t cmdTokenize(char *input, const char *tokens[],
    uint8_t *token_count, uint8_t max_tokens);
cmd_status_t cmdHandleHelp(const char *tokens[], uint8_t token_count,
    char *result, uint16_t result_len);
void cmdInitHelp(const cmd_entry_t *table, uint8_t count);

/* Help functions */
int8_t strcmpi(const char *a, const char *b);
void safeStrcat(char *dest, uint16_t *len, uint16_t max, const char *src);

#endif /* SOURCE_CMD_DISPATCHER_H_ */
