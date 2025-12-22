/*
 * Created on: Dec 21, 2025
 *     Author: vysocan
 *
 * Hierarchical command dispatcher for embedded systems.
 *
 */

#include "cmd_dispatcher.h"

/*
 * @brief Case-insensitive string comparison
 *
 * @param a First string
 * @param b Second string
 *
 * @return int8_t 0 if equal, negative if a < b, positive if a > b
 *
 */
int8_t strcmpi (const char *a, const char *b) {
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
 *
 * @param dest Destination buffer
 * @param len Pointer to current length of destination buffer
 * @param max Maximum size of destination buffer
 * @param src Source string
 *
 * @return void
 */
void safe_strcat (char *dest, uint16_t *len, uint16_t max, const char *src) {
  while (*src && *len < max - 1) {
    dest[(*len)++] = *src++;
  }
  dest[*len] = '\0';
}

/* @brief Tokenize input string by spaces
 *
 * @param input Input string to tokenize, it will be modified
 * @param tokens Output array of token pointers
 * @param token_count Output number of tokens found
 * @param max_tokens Maximum number of tokens to parse
 *
 * @return cmd_status_t Status of tokenization
 *
 */
cmd_status_t cmd_tokenize (char *input, const char *tokens[], uint8_t *token_count, uint8_t max_tokens) {
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
 * @brief Find command in table
 *
 * @param table Command table
 * @param count Number of commands in table
 * @param name Command name to find
 *
 * @return const cmd_entry_t* Pointer to command entry, or NULL if not found
 *
 */
static const cmd_entry_t* find_command (const cmd_entry_t *table, uint8_t count, const char *name) {
  for (uint8_t i = 0; i < count; i++) {
    if (strcmpi (table[i].name, name) == 0) {
      return &table[i];
    }
  }
  return NULL;
}

/* Append auto-generated help for a command's subcommands */
static void append_sub_help (const cmd_entry_t *cmd, char *result,
                             uint16_t result_len) {
  uint16_t len = (uint16_t) strlen (result);

  safe_strcat (result, &len, result_len, "Available subcommands for ");
  safe_strcat (result, &len, result_len, cmd->name);
  safe_strcat (result, &len, result_len, ":\n");

  for (uint8_t i = 0; i < cmd->sub_count; i++) {
    const cmd_entry_t *sub = &cmd->sub[i];
    safe_strcat (result, &len, result_len, "  ");
    safe_strcat (result, &len, result_len, sub->name);
    if (sub->help && sub->help[0]) {
      safe_strcat (result, &len, result_len, " - ");
      safe_strcat (result, &len, result_len, sub->help);
    }
    safe_strcat (result, &len, result_len, "\n");
  }
}
/*
 * @breif Recursive command execution over static tree
 *
 * @param table Current command table
 * @param table_count Number of commands in current table
 * @param tokens Tokenized input command
 * @param token_count Number of tokens
 * @param result Output result buffer
 * @param result_len Length of output result buffer
 *
 * @return cmd_status_t Status of command processing
 *
 */
static cmd_status_t exec_command_tree (const cmd_entry_t *table, uint8_t table_count,
  const char *tokens[], uint8_t token_count, char *result, uint16_t result_len) {

  if (token_count == 0) return CMD_INCOMPLETE;

  const cmd_entry_t *cmd = find_command(table, table_count, tokens[0]);
  if (!cmd) return CMD_UNKNOWN;

  const char **next_tokens = tokens + 1;
  uint8_t next_count = token_count - 1;

  /* Try subcommands first */
  if (cmd->sub && cmd->sub_count > 0) {
      if (next_count == 0) {
          result[0] = '\0';
          append_sub_help(cmd, result, result_len);
          return CMD_INCOMPLETE;
      }

      cmd_status_t sub_status = exec_command_tree(
          cmd->sub, cmd->sub_count, next_tokens, next_count,
          result, result_len);
      if (sub_status != CMD_UNKNOWN) return sub_status;
  }

  /* Try handler */
  if (cmd->handler) {
      return cmd->handler(next_tokens, next_count, result, result_len);
  }

  return CMD_UNKNOWN;
}
/*
 * @breif Core command processor - main entry point
 *
 * @param input Input command string, it will be modified during processing
 * @param dispatcher Command dispatcher context
 * @param result Output result buffer
 * @param result_len Length of output result buffer
 *
 * @return cmd_status_t Status of command processing
 *
 */
cmd_status_t cmd_process (char *input, const cmd_dispatcher_t *dispatcher,
  char *result, uint16_t result_len) {
  if (!input || !dispatcher || !result) return CMD_INVALID_ARGS;

  const char *tokens[CMD_MAX_TOKENS];
  uint8_t token_count = 0;

  cmd_status_t status = cmd_tokenize(input, tokens, &token_count, CMD_MAX_TOKENS);
  if (status != CMD_OK) return status;

  result[0] = '\0';
  return exec_command_tree (dispatcher->commands, dispatcher->cmd_count, tokens,
    token_count, result, result_len);
}
