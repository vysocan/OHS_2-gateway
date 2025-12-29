/*
 * Created on: Dec 21, 2025
 * Author: vysocan
 *
 * Hierarchical command dispatcher for embedded systems.
 */

#include "cmd_dispatcher.h"
#include <string.h>
#include <stdint.h>

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
 * @brief Safe string copy with buffer overflow protection
 * @param dest Destination buffer
 * @param len Pointer to current length of destination buffer
 * @param max Maximum size of destination buffer
 * @param src Source string
 * @return void
 */
void safeStrcat(char *dest, uint16_t *len, uint16_t max, const char *src) {
  while (*src && *len < max - 1) {
    dest[(*len)++] = *src++;
  }
  dest[*len] = '\0';
}

/*
 * @brief Tokenize input string by spaces
 * @param input Input string to tokenize
 * @param tokens Array to store token pointers
 * @param token_count Pointer to store number of tokens
 * @param max_tokens Maximum number of tokens to extract
 * @return cmd_status_t Status code
 */
cmd_status_t cmdTokenize(char *input, const char *tokens[],
    uint8_t *token_count, uint8_t max_tokens) {
  *token_count = 0;
  if (!input || !*input) return CMD_INVALID_ARGS;

  char *current = input;
  while (*current == ' ' || *current == '\t')
    current++;

  while (*current && *token_count < max_tokens) {
    while (*current == ' ' || *current == '\t')
      current++;
    if (!*current) break;

    tokens[*token_count] = current;
    while (*current && *current != ' ' && *current != '\t')
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
static const cmd_entry_t* findCommand(const cmd_entry_t *table, uint8_t count,
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
 * @param result Result buffer
 * @param result_len Maximum length of result buffer
 */
static void appendSubHelp(const cmd_entry_t *cmd, char *result,
    uint16_t result_len) {

  uint16_t len = chsnprintf(result, result_len, "Subcommands for %s:\n", cmd->name);

  for (uint8_t i = 0; i < cmd->sub_count; i++) {
    const cmd_entry_t *sub = &cmd->sub[i];
    safeStrcat (result, &len, result_len, " ");
    safeStrcat (result, &len, result_len, sub->name);
    if (sub->help && sub->help[0]) {
      safeStrcat (result, &len, result_len, " - ");
      safeStrcat (result, &len, result_len, sub->help);
    }
    safeStrcat (result, &len, result_len, "\n");
  }
}

/*
 * @brief Recursively execute command through the command tree
 * @param table Current command table
 * @param table_count Number of entries in table
 * @param tokens Tokenized input
 * @param token_count Number of tokens
 * @param result Result buffer
 * @param result_len Maximum length of result buffer
 * @return cmd_status_t Status code
 */
static cmd_status_t execCommandTree(const cmd_entry_t *table,
    uint8_t table_count, const char *tokens[], uint8_t token_count,
    char *result, uint16_t result_len) {
  if (token_count == 0) return CMD_INCOMPLETE;

  const cmd_entry_t* cmd = findCommand(table, table_count, tokens[0]);
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
      result[0] = '\0';
      appendSubHelp (cmd, result, result_len);
      return CMD_INCOMPLETE;
    }

    cmd_status_t sub_status = execCommandTree (cmd->sub, cmd->sub_count,
        next_tokens, next_count, result, result_len);
    if (sub_status != CMD_UNKNOWN) return sub_status;
  }

  /* Try handler */
  if (cmd->handler) {
    return cmd->handler (next_tokens, next_count, result, result_len);
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
cmd_status_t cmdProcess(char *input, const cmd_entry_t *table,
    uint8_t table_count, char *result, uint16_t result_len) {
  if (!input || !table || !result) return CMD_INVALID_ARGS;

  const char *tokens[CMD_MAX_TOKENS];
  uint8_t token_count = 0;
  cmd_status_t status = cmdTokenize(input, tokens, &token_count, CMD_MAX_TOKENS);
  if (status != CMD_OK) return status;

  result[0] = '\0';
  return execCommandTree (table, table_count, tokens, token_count, result,
      result_len);
}

/*
 * @brief BUILT-IN HELP SYSTEM
 */

/* Global pointer to command table for help handler */
static const cmd_entry_t *g_cmd_table = NULL;
static uint8_t g_cmd_table_count = 0;

/*
 * @brief Append formatted help for a single command
 * @param cmd Command entry
 * @param indent_level Indentation level for hierarchy
 * @param result Result buffer
 * @param result_len Pointer to current length
 * @param result_max Maximum length of result buffer
 */
static void appendCmdHelp(const cmd_entry_t *cmd, uint8_t indent_level,
    char *result, uint16_t *result_len, uint16_t result_max) {

  for (uint8_t i = 0; i < indent_level; i++) {
    safeStrcat (result, result_len, result_max, "-");
  }

  safeStrcat (result, result_len, result_max, cmd->name);
  // if (cmd->help && cmd->help[0]) {
  //     safeStrcat(result, result_len, result_max, " - ");
  //     safeStrcat(result, result_len, result_max, cmd->help);
  // }
  safeStrcat (result, result_len, result_max, "\n");
}

/*
 * @brief Recursively append help for command tree
 * @param table Command table
 * @param table_count Number of entries
 * @param indent_level Current indentation level
 * @param result Result buffer
 * @param result_len Pointer to current length
 * @param result_max Maximum length of result buffer
 */
static void appendTreeHelp(const cmd_entry_t *table, uint8_t table_count,
    uint8_t indent_level, char *result, uint16_t *result_len,
    uint16_t result_max) {
  for (uint8_t i = 0; i < table_count; i++) {
    // Stop if buffer is full
    if (*result_len >= result_max - 1) break;

    const cmd_entry_t *cmd = &table[i];
    appendCmdHelp (cmd, indent_level, result, result_len, result_max);
    if (cmd->sub && cmd->sub_count > 0) {
      appendTreeHelp (cmd->sub, cmd->sub_count, indent_level + 1, result,
          result_len, result_max);
    }
  }
}

/*
 * @brief Built-in HELP command handler (wrapper with correct signature)
 * @param tokens Token array (unused)
 * @param token_count Token count (unused)
 * @param result Output buffer
 * @param result_len Maximum length of output buffer
 * @return cmd_status_t Always returns CMD_OK
 */
cmd_status_t cmdHandleHelp(const char *tokens[], uint8_t token_count,
    char *result, uint16_t result_len) {
  (void) tokens;
  (void) token_count;

  if (!g_cmd_table || g_cmd_table_count == 0) {
    uint16_t len = 0;
    safeStrcat (result, &len, result_len,
        "ERROR: Command table not initialized\n");
    return CMD_ERROR;
  }

  uint16_t len = 0;
  safeStrcat (result, &len, result_len, "Commands:\n");
  appendTreeHelp (g_cmd_table, g_cmd_table_count, 0, result, &len,
      result_len);
  safeStrcat (result, &len, result_len, "\n");
  return CMD_OK;
}

/*
 * @brief Initialize command table for help handler
 * @param table Root command table
 * @param count Number of root commands
 * @return void
 */
void cmdInitHelp(const cmd_entry_t *table, uint8_t count) {
  g_cmd_table = table;
  g_cmd_table_count = count;
}
