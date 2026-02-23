/*
 * Created on: Dec 21, 2025
 * Author: vysocan
 *
 * Hierarchical command dispatcher for embedded systems.
 */

#include "cmd_dispatcher.h"
#include <string.h>
#include <stdint.h>
#include "hal.h"
#include "chprintf.h"
#include "memstreams.h"

// Memory stream for output
MemoryStream ms;
BaseSequentialStream *chp;

/*
 * @brief Case-insensitive string comparison
 * @param a First string
 * @param b Second string
 * @return int8_t 0 if equal, negative if a < b, positive if a > b
 */
int8_t strcmpi(const char *a, const char *b) {
  while (*a && *b) {
    char ca = (*a >= 'a' && *a <= 'z') ? (*a - 32) : *a;
    char cb = (*b >= 'a' && *b <= 'z') ? (*b - 32) : *b;
    if (ca != cb) return ca - cb;
    a++;
    b++;
  }
  return (unsigned char) *a - (unsigned char) *b;
}

/*
 * @brief Tokenize input string by spaces OR forward slashes
 * @param input Input string to tokenize
 * @param tokens Array to store token pointers
 * @param token_count Pointer to store number of tokens
 * @param max_tokens Maximum number of tokens to extract
 * @return cmd_status_t Status code
 */
cmdStatus_t cmdTokenize(char *input, const char *tokens[],
    uint8_t *token_count, uint8_t max_tokens) {
  *token_count = 0;

  if (!input || !*input) return CMD_INVALID_ARGS;

  char *current = input;
  // Skip leading whitespace or slashes
  while (*current == ' ' || *current == '\t' || *current == '/')
    current++;

  while (*current && *token_count < max_tokens) {
    // Skip whitespace or slashes between tokens
    while (*current == ' ' || *current == '\t' || *current == '/')
      current++;
    if (!*current) break;

    tokens[*token_count] = current;
    // Advance until next delimiter (space, tab, or slash)
    while (*current && *current != ' ' && *current != '\t' && *current != '/')
      current++;

    if (*current) {
      *current = '\0';
      current++;
    }
    (*token_count)++;
  }

  return *token_count > 0 ? CMD_OK : CMD_INVALID_ARGS;
}

/*
 * @brief Find command in table by name (case-insensitive)
 * @param table Command table
 * @param count Number of entries in table
 * @param name Command name to find
 * @return Pointer to cmd_entry_t or NULL if not found
 */
static const cmdEntry_t* findCommand(const cmdEntry_t *table, uint8_t count,
    const char *name) {
  for (uint8_t i = 0; i < count; i++) {
    if (strcmpi (table[i].name, name) == 0) {
      return &table[i];
    }
  }
  return NULL;
}

/*
 * @brief Append auto-generated help for subcommands (CMD_INCOMPLETE)
 * @param cmd Parent command entry
 * @param *chp Output stream
 */
static void appendSubHelp(const cmdEntry_t *cmd, BaseSequentialStream *chp) {

  chprintf(chp, "Subcommands for %s:\n", cmd->name);

  for (uint8_t i = 0; i < cmd->sub_count; i++) {
    const cmdEntry_t *sub = &cmd->sub[i];
    chprintf(chp, "  %s", sub->name);
    if (sub->help && sub->help[0]) {
      chprintf(chp, " %s %s", INDENT_STR, sub->help);
    }
    chprintf(chp, "\n");
  }
}

/*
 * @brief Recursively execute command through the command tree
 * @param table Current command table
 * @param table_count Number of entries in table
 * @param tokens Tokenized input
 * @param token_count Number of tokens
 * @param *chp Output stream
 * @return cmd_status_t Status code
 */
static cmdStatus_t execCommandTree(const cmdEntry_t *table,
    uint8_t table_count, const char *tokens[], uint8_t token_count,
    BaseSequentialStream *chp) {
  if (token_count == 0) return CMD_INCOMPLETE;

  const cmdEntry_t* cmd = findCommand(table, table_count, tokens[0]);
#if SMD_STRICT_MODE
  if (!cmd) return CMD_UNKNOWN;
#else
  if (!cmd) {
    cmd = findCommand(table, table_count, "help");
    if (!cmd) return CMD_UNKNOWN;
  }
#endif

  const char **next_tokens = tokens + 1;
  uint8_t next_count = token_count - 1;

  /* Try subcommands first */
  if (cmd->sub && cmd->sub_count > 0) {
    if (next_count == 0) {
      appendSubHelp (cmd, chp);
      return CMD_INCOMPLETE;
    }

    cmdStatus_t sub_status = execCommandTree (cmd->sub, cmd->sub_count,
        next_tokens, next_count, chp);
    if (sub_status != CMD_UNKNOWN) return sub_status;
  }

  /* Try handler */
  if (cmd->handler) {
    return cmd->handler (next_tokens, next_count, chp);
  }

  return CMD_UNKNOWN;
}

/*
 * @brief Main entry point for command processing
 * @param input Raw input string from user
 * @param table Root command table
 * @param table_count Number of root commands
 * @param result Output buffer for command result
 * @param result_len Maximum length of result buffer
 * @return cmd_status_t Status code
 */
cmdStatus_t cmdProcess(char *input, const cmdEntry_t *table,
    uint8_t table_count, char *result, uint16_t result_len) {
  if (!input || !table || !result) return CMD_INVALID_ARGS;

  const char *tokens[CMD_MAX_TOKENS];
  uint8_t token_count = 0;
  cmdStatus_t status = cmdTokenize(input, tokens, &token_count, CMD_MAX_TOKENS);
  if (status != CMD_OK) return status;

  // Prepare memory stream for output
  memset(result, 0, result_len);
  msObjectInit(&ms, (uint8_t *)result, result_len - 1, 0);
  chp = (BaseSequentialStream *)(void *)&ms;

  // Perform command execution
  return execCommandTree (table, table_count, tokens, token_count, chp);
}

/*
 * @brief BUILT-IN HELP SYSTEM
 */

/* Global pointer to command table for help handler */
static const cmdEntry_t *g_cmd_table = NULL;
static uint8_t g_cmd_table_count = 0;

/*
 * @brief Append formatted help for a single command
 * @param cmd Command entry
 * @param indent_level Indentation level for hierarchy
 * @param *chp Output stream
 */
static void appendCmdHelp(const cmdEntry_t *cmd, uint8_t indent_level,
    BaseSequentialStream *chp) {

  for (uint8_t i = 0; i < indent_level; i++) {
    chprintf(chp, "%s", INDENT_STR);
  }
  chprintf(chp, "%s\r\n", cmd->name);
}

/*
 * @brief Recursively append help for command tree
 * @param table Command table
 * @param table_count Number of entries
 * @param indent_level Current indentation level
 * @param *chp Output stream
 */
static void appendTreeHelp(const cmdEntry_t *table, uint8_t table_count,
    uint8_t indent_level, BaseSequentialStream *chp) {
  for (uint8_t i = 0; i < table_count; i++) {

    const cmdEntry_t *cmd = &table[i];
    appendCmdHelp (cmd, indent_level, chp);
    if (cmd->sub && cmd->sub_count > 0) {
      appendTreeHelp (cmd->sub, cmd->sub_count, indent_level + 1, chp);
    }
  }
}

/*
 * @brief Built-in HELP command handler (wrapper with correct signature)
 * @param tokens Token array (unused)
 * @param token_count Token count (unused)
 * @param *chp Output stream
 * @return cmd_status_t Always returns CMD_OK
 */
cmdStatus_t cmdHandleHelp(const char *tokens[], uint8_t token_count,
    BaseSequentialStream *chp) {
  (void) tokens;
  (void) token_count;

  if (!g_cmd_table || g_cmd_table_count == 0) {
    chprintf(chp, "ERROR: Command table not initialized\r\n");
    return CMD_ERROR;
  }

  chprintf(chp, "Commands:\r\n");
  appendTreeHelp (g_cmd_table, g_cmd_table_count, 0, chp);
  chprintf(chp, "\r\n");
  return CMD_OK;
}

/*
 * @brief Initialize command table for help handler
 * @param table Root command table
 * @param count Number of root commands
 * @return void
 */
void cmdInitHelp(const cmdEntry_t *table, uint8_t count) {
  g_cmd_table = table;
  g_cmd_table_count = count;
}
