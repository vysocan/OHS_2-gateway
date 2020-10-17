#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tcl.h"

#include "hal.h"
#include "chprintf.h"

#include "umm_malloc_cfg.h"
#include "umm_malloc.h"

#if 0
#define DBG printf
#else
//#define DBG(...) {chprintf((BaseSequentialStream*) &SD3, __VA_ARGS__); chprintf((BaseSequentialStream*) &SD3, "\r\n");}
#define DBG(...)
#endif
//#define VAR(...) {chprintf((BaseSequentialStream*) &SD3, __VA_ARGS__); chprintf((BaseSequentialStream*) &SD3, "\r\n");}
#define VAR(...)

#ifndef MAX_VAR_LENGTH
#define MAX_VAR_LENGTH 256
#endif

// OHS changes
uint16_t              tcl_iteration;
BaseSequentialStream *tcl_output;

#ifndef TCL_DISABLE_MATH
  const char* math[] = { "+", "-", "*", "/", ">", ">=", "<", "<=", "==", "!=", "&&", "||"};
#endif

struct tcl;
int tcl_eval(struct tcl* tcl, const char* s, size_t len);

static int tcl_is_special(char c, int q) {
  return (
      c == '$' || (!q && (c == '{' || c == '}' || c == ';' || c == '\r' || c == '\n')) || c == '[' || c == ']' || c == '"' || c == '\0');
}

static int tcl_is_space(char c) { return (c == ' ' || c == '\t'); }

static int tcl_is_end(char c) {
  return (c == '\n' || c == '\r' || c == ';' || c == '\0');
}

int tcl_next(const char* s, size_t n, const char** from, const char** to, int* q) {
  unsigned int i = 0;
  int depth = 0;
  char open;
  char close;
  DBG("tcl_next(%.*s)+%d+%d|q:%d", n, s, *from - s, *to - s, *q);

  // Check for endless loop
  if (!tcl_iteration) {
    TCL_ERROR("Maximum iteration reached");
    return TERROR;
  }
  tcl_iteration--;

  /* Skip leading spaces if not quoted */
  for (; !*q && n > 0 && tcl_is_space(*s); s++, n--) {
  }
  *from = s;
  /* Terminate command if not quoted */
  if (!*q && n > 0 && tcl_is_end(*s)) {
    *to = s + 1;
    return TCMD;
  }
  if (*s == '$') { /* Variable token, must not start with a space or quote */
    if (tcl_is_space(s[1]) || s[1] == '"') {
      TCL_ERROR("Variable '%.*s' malformatted", n, s);
      return TERROR;
    }
    int mode = *q;
    *q = 0;
    int r = tcl_next(s + 1, n - 1, to, to, q);
    *q = mode;
    return ((r == TWORD && *q) ? TPART : r);
  }

  if (*s == '[' || (!*q && *s == '{')) {
    /* Interleaving pairs are not welcome, but it simplifies the code */
    open = *s;
    close = (open == '[' ? ']' : '}');
    for (i = 0, depth = 1; i < n && depth != 0; i++) {
      if (i > 0 && s[i] == open) {
        depth++;
      } else if (s[i] == close) {
        depth--;
      }
    }
  } else if (*s == '"') {
    *q = !*q;
    *from = *to = s + 1;
    if (*q) {
      DBG("TPART>(%.*s)|%d\r\n", n, s, *q);
      // Check depth
      for (i = 0; i < n; i++) {
        if (i > 0 && s[i] == '"') break;
      }
      if (i == n) {
        TCL_ERROR("End of quote not received '%.*s'.", n, s);
        return TERROR;
      }
      return TPART;
    }
    if (n < 2 || (!tcl_is_space(s[1]) && !tcl_is_end(s[1]))) {
      TCL_ERROR("End of command expected, but received '%.*s'.", n, s);
      return TERROR;
    }
    *from = *to = s + 1;
    DBG("TWORD>(%.*s)|%d\r\n", n, s, q);
    return TWORD;
  } else {
    while (i <= n && (*q || !tcl_is_space(s[i])) && !tcl_is_special(s[i], *q)) {
      i++;
    }
  }
  *to = s + i;
  if (i == n) {
    TCL_ERROR("Continuation expected '%.*s'", n, s);
    return TERROR;
  }
  if (*q) {
    return TPART;
  }
  return (tcl_is_space(s[i]) || tcl_is_end(s[i])) ? TWORD : TPART;
}

/* ------------------------------------------------------- */

static inline void* tcl_malloc(size_t n)        { return umm_malloc(n); }
inline void  tcl_free(void* v)           { umm_free(v); }
static inline void* tcl_realloc(void* v, int n) { return umm_realloc(v, n); }

// --- const char *empty_str = "";
char *empty_str = "";

// --- const char* tcl_string(tcl_value_t* v) { return v == NULL ? empty_str : v; }
char* tcl_string(tcl_value_t* v) { return v == NULL ? empty_str : v; }

int tcl_strcmp(tcl_value_t* u, tcl_value_t* v) {
  return strcmp(u, v);
}

int tcl_length(tcl_value_t* v) { return v == NULL ? 0 : strlen(v); }
int tcl_int(tcl_value_t* v) { return strtoul(v, NULL, 0); }
float tcl_float(tcl_value_t* v) { return strtof(v, NULL); }

tcl_value_t* tcl_append_string(tcl_value_t* v, const char* s, size_t len) {
  size_t n = tcl_length(v);
  v = tcl_realloc(v, n + len + 1);
  memset((char*)tcl_string(v) + n, 0, len + 1);
  strncpy((char*)tcl_string(v) + n, s, len);
  DBG("Re-allocate '%x', value '%s'.", v, s);
  return v;
}

tcl_value_t* tcl_append(tcl_value_t* v, tcl_value_t* tail) {
  v = tcl_append_string(v, tcl_string(tail), tcl_length(tail));
  DBG("FREE tcl_append %x.", tail);
  tcl_free(tail);
  return v;
}

tcl_value_t* tcl_alloc(const char* s, size_t len) {
  return tcl_append_string(NULL, s, len);
}

tcl_value_t* tcl_dup(tcl_value_t* v) {
  DBG("tcl_dup %s.", v);
  return tcl_alloc(tcl_string(v), tcl_length(v));
}

tcl_value_t* tcl_list_alloc(void) {
  return tcl_alloc("", 0);
}

int tcl_list_length(tcl_value_t* v) {
  int count = 0;
  tcl_each(tcl_string(v), tcl_length(v) + 1, 0) {
    DBG("tcl_list_length(%.*s)%d\r\n", tcl_length(v) + 1, tcl_string(v), p.token);
    if (p.token == TWORD) {
      count++;
    }
  }
  DBG("tcl_list_length=%d\r\n", count);
  return count;
}

void tcl_list_free(tcl_value_t* v) {
  DBG("FREE tcl_list_free %x.", v);
  tcl_free(v);
}

tcl_value_t* tcl_list_at(tcl_value_t* v, int index) {
  int i = 0;
  tcl_each(tcl_string(v), tcl_length(v) + 1, 0) {
    if (p.token == TWORD) {
      if (i == index) {
        if (p.from[0] == '{') {
          return tcl_alloc(p.from + 1, p.to - p.from - 2);
        }
        return tcl_alloc(p.from, p.to - p.from);
      }
      i++;
    }
  }
  return NULL;
}

tcl_value_t* tcl_list_append(tcl_value_t* v, tcl_value_t* tail) {
  DBG("tcl_list_append %s, %s.", v, tail)
  if (tcl_length(v) > 0) {
    DBG("tcl_list_append 1")
    v = tcl_append(v, tcl_alloc(" ", 2));
  }
  if (tcl_length(tail) > 0) {
    DBG("tcl_list_append 2")
    int q = 0;
    const char* p;
    for (p = tcl_string(tail); *p; p++) {
      if (tcl_is_space(*p) || tcl_is_special(*p, 0)) {
        DBG("tcl_list_append break")
        q = 1;
        break;
      }
    }
    if (q) {
      DBG("tcl_list_append 3")
      v = tcl_append(v, tcl_alloc("{", 1));
    }
    v = tcl_append(v, tcl_dup(tail));
    if (q) {
      DBG("tcl_list_append 4")
      v = tcl_append(v, tcl_alloc("}", 1));
    }
  } else {
    DBG("tcl_list_append 5")
    v = tcl_append(v, tcl_alloc("{}", 2));
  }
  DBG("tcl_list_append return %x", v);
  return v;
}

/* ----------------------------- */
/* ----------------------------- */
/* ----------------------------- */
/* ----------------------------- */

struct tcl_env* tcl_env_alloc(struct tcl_env* parent) {
  struct tcl_env* env = tcl_malloc(sizeof(*env));
  env->vars = NULL;
  env->parent = parent;
  return env;
}

struct tcl_var* tcl_env_var(struct tcl_env* env, tcl_value_t* name) {
  struct tcl_var* var = tcl_malloc(sizeof(struct tcl_var));
  var->name = tcl_dup(name);
  var->next = env->vars;
  var->value = tcl_alloc("", 0);
  //var->var_type = VARCHAR;
  env->vars = var;
  DBG("Allocate '%s'.", name);
  return var;
}

struct tcl_env* tcl_env_free(struct tcl_env* env) {
  struct tcl_env* parent = env->parent;
  while (env->vars) {
    struct tcl_var* var = env->vars;
    env->vars = env->vars->next;
    tcl_free(var->name);
    tcl_free(var->value);
    tcl_free(var);
  }
  tcl_free(env);
  return parent;
}
/*
 * Variable type tests
 */
/*
static int tcl_is_digit(char c) { return (c >= '0' && c <= '9'); }

tcl_var_t tcl_var_type(tcl_value_t * val) {
  tcl_var_t var_type = VARINT;
  uint16_t len = strlen(val);
  if (len > 0) {
    for (uint16_t i = 0; i < len; ++i) {
      if (!tcl_is_digit(val[i])) {
        var_type = VARFLOAT;
        if (val[i] != '.') {
          var_type = VARCHAR;
          break;
        }
      }
    }
  } else {
    var_type = VARCHAR;
  }
  return var_type;
}
*/

tcl_value_t* tcl_var(struct tcl* tcl, tcl_value_t* name, tcl_value_t* v) {
  struct tcl_var* var;

  VAR("tcl_var start |%s|%s|", name, v);
  for (var = tcl->env->vars; var != NULL; var = var->next) {
    if (strcmp(var->name, tcl_string(name)) == 0) {
      VAR("- Get '%s = %s'.", var->name, var->value);
      break;
    }
  }
  // Error reporting
  if ((var == NULL) && (v == NULL)) {
    TCL_WARNING("Variable '%s' is empty", name);
  }
  if (var == NULL) {
    VAR("- Allocate new %s.", name);
    var = tcl_env_var(tcl->env, name);
  }
  if (v != NULL) {
    tcl_free(var->value);
    var->value = tcl_dup(v);
    VAR("- Set str '%s = %s|%s'.", var->name, var->value, v);
    tcl_free(v);
  }
  VAR("tcl_var end");
  return var->value;
}

int tcl_result(struct tcl* tcl, int flow, tcl_value_t* result) {
  DBG("tcl_result (%.*s), flow=%d", tcl_length(result), tcl_string(result), flow);
  DBG("FREE tcl_result %x.", tcl->result);
  tcl_free(tcl->result);
  tcl->result = result;
  return flow;
}

int tcl_subst(struct tcl* tcl, const char* s, size_t len) {
  DBG("subst(%.*s)", (int)len, s);
  if (len == 0) {
    return tcl_result(tcl, FNORMAL, tcl_alloc("", 0));
  }
  switch (s[0]) {
  case '{':
    if (len <= 1) {
      TCL_ERROR("Length of subst(%.*s) is too small", (int)len, s);
      return tcl_result(tcl, FERROR, tcl_alloc("", 0));
    }
    return tcl_result(tcl, FNORMAL, tcl_alloc(s + 1, len - 2));
  case '$': {
    if (len >= MAX_VAR_LENGTH) {
      TCL_ERROR("Length of subst(%.*s) reached maximum(%u)", (int)len, s, MAX_VAR_LENGTH);
      return tcl_result(tcl, FERROR, tcl_alloc("", 0));
    }
    char buf[5 + MAX_VAR_LENGTH] = "set ";
    strncat(buf, s + 1, len - 1);
    DBG("subst (%.*s)", (int)strlen(buf) + 1, buf);
    return tcl_eval(tcl, buf, strlen(buf) + 1);
  }
  case '[': {
    tcl_value_t* expr = tcl_alloc(s + 1, len - 2);
    int r = tcl_eval(tcl, tcl_string(expr), tcl_length(expr) + 1);
    DBG("FREE tcl_subst %x.", expr);
    tcl_free(expr);
    return r;
  }
  default:
    return tcl_result(tcl, FNORMAL, tcl_alloc(s, len));
  }
}

int tcl_eval(struct tcl* tcl, const char* s, size_t len) {
  DBG("Eval(%.*s)->", (int)len, s);
  tcl_value_t* list = tcl_list_alloc();
  tcl_value_t* cur = NULL;
  tcl_each(s, len, 1) {
    switch (p.token) {
      case TERROR:
        DBG("Eval: FERROR, lexer error");
        TCL_ERROR("At %.*s", (int)(p.to - p.from), p.from);
        tcl_list_free(list);
        tcl_free(cur);
        return tcl_result(tcl, FERROR, tcl_alloc("", 0));
      case TWORD:
        DBG("TWORD %.*s, length=%d, cur=%u", (int)(p.to - p.from), p.from, (int)(p.to - p.from), cur);
        if (cur != NULL) {
          tcl_subst(tcl, p.from, p.to - p.from);
          tcl_value_t* part = tcl_dup(tcl->result);
          cur = tcl_append(cur, part);
        } else {
          tcl_subst(tcl, p.from, p.to - p.from);
          cur = tcl_dup(tcl->result);
        }
        list = tcl_list_append(list, cur);
        DBG("FREE tcl_eval cur %x.", cur);
        tcl_free(cur);
        cur = NULL;
        break;
      case TPART:
        DBG("TWORD %.*s, length=%d, cur=%u", (int)(p.to - p.from), p.from, (int)(p.to - p.from), cur);
        tcl_subst(tcl, p.from, p.to - p.from);
        tcl_value_t* part = tcl_dup(tcl->result);
        cur = tcl_append(cur, part);
        break;
      case TCMD:
        DBG("TCMD %.*s, length=%d, cur=%u", (int)(p.to - p.from), p.from, (int)(p.to - p.from), cur);
        if (tcl_list_length(list) == 0) {
          tcl_result(tcl, FNORMAL, tcl_alloc("", 0));
        } else {
          tcl_value_t* cmdname = tcl_list_at(list, 0);
          struct tcl_cmd* cmd = NULL;
          int r = FERROR;
          for (cmd = tcl->cmds; cmd != NULL; cmd = cmd->next) {
            if (tcl_strcmp(tcl_string(cmdname), tcl_string(cmd->name)) == 0) {
              if (cmd->arity == 0 || cmd->arity == tcl_list_length(list)) {
                r = cmd->fn(tcl, list, cmd->arg);
                // Error reporting
                if (r == FERROR) {
                  TCL_WARNING("Command '%s' returned ERROR", tcl_string(cmdname));
                }
                break;
              } else {
                // Error reporting
                TCL_ERROR("Expected %u argument(s) after '%s', received %u, at '%s'",
                      cmd->arity-1, tcl_string(cmdname), tcl_list_length(list)-1, list);
                break;
              }
            }
          }
          // Error reporting
          if (cmd == NULL) {
            TCL_ERROR("Command '%s' unknown", tcl_string(cmdname));
          }
          DBG("FREE tcl_eval cmdname %x.", cmdname);
          tcl_free(cmdname);
          if (cmd == NULL || r != FNORMAL) {
            DBG("FREE tcl_eval list %x.", list);
            tcl_list_free(list);
            return r;
          }
        }
        DBG("FREE tcl_eval list %x.", list);
        tcl_list_free(list);
        list = tcl_list_alloc();
        break;
    } // switch
  }
  DBG("FREE tcl_eval list %x.", list);
  tcl_list_free(list);
  return FNORMAL;
}

/* --------------------------------- */
/* --------------------------------- */
/* --------------------------------- */
/* --------------------------------- */
/* --------------------------------- */
void tcl_register(struct tcl* tcl, const char* name, tcl_cmd_fn_t fn, int arity,
    void* arg, const char* description) {
  struct tcl_cmd* cmd = tcl_malloc(sizeof(struct tcl_cmd));
  cmd->name = tcl_alloc(name, strlen(name));
  cmd->fn = fn;
  cmd->arg = arg;
  cmd->arity = arity;
  cmd->description = description;
  cmd->next = tcl->cmds;
  tcl->cmds = cmd;
}

static int tcl_cmd_set(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  tcl_value_t* var = tcl_list_at(args, 1);
  tcl_value_t* val = tcl_list_at(args, 2);
  DBG("%s = %s", var, val);
  int r = tcl_result(tcl, FNORMAL, tcl_dup(tcl_var(tcl, var, val)));
  DBG("FREE tcl_cmd_set %x.", var);
  tcl_free(var);
  return r;
}

static int tcl_cmd_subst(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  tcl_value_t* s = tcl_list_at(args, 1);
  int r = tcl_subst(tcl, tcl_string(s), tcl_length(s));
  DBG("FREE tcl_cmd_subst %x.", s);
  tcl_free(s);
  return r;
}

#ifndef TCL_DISABLE_PUTS
static int tcl_cmd_puts(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  tcl_value_t* text = tcl_list_at(args, 1);
  chprintf(tcl_output, "%s\r\n", tcl_string(text));
  return tcl_result(tcl, FNORMAL, text);
}
#endif

static int tcl_user_proc(struct tcl* tcl, tcl_value_t* args, void* arg) {
  tcl_value_t* code = (tcl_value_t*)arg;
  tcl_value_t* params = tcl_list_at(code, 2);
  tcl_value_t* body = tcl_list_at(code, 3);
  tcl->env = tcl_env_alloc(tcl->env);
  for (int i = 0; i < tcl_list_length(params); i++) {
    tcl_value_t* param = tcl_list_at(params, i);
    tcl_value_t* v = tcl_list_at(args, i + 1);
    tcl_var(tcl, param, v);
    DBG("FREE tcl_user_proc %x.", param);
    tcl_free(param);
  }
  int r = tcl_eval(tcl, tcl_string(body), tcl_length(body) + 1);
  DBG("FREE tcl_user_proc %x.", tcl->env);
  tcl->env = tcl_env_free(tcl->env);
  DBG("FREE tcl_user_proc %x.", params);
  tcl_free(params);
  DBG("FREE tcl_user_proc %x.", body);
  tcl_free(body);
  /* return FNORMAL; */
  return r;
}

static int tcl_cmd_proc(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  tcl_value_t* name = tcl_list_at(args, 1);
  tcl_register(tcl, tcl_string(name), tcl_user_proc, 0, tcl_dup(args), NULL);
  DBG("FREE tcl_cmd_proc %x.", name);
  tcl_free(name);
  return tcl_result(tcl, FNORMAL, tcl_alloc("", 0));
}

static int tcl_cmd_if(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  int i = 1;
  int n = tcl_list_length(args);
  int r = FNORMAL;
  while (i < n) {
    tcl_value_t* cond = tcl_list_at(args, i);
    tcl_value_t* branch = NULL;
    if (i + 1 < n) {
      branch = tcl_list_at(args, i + 1);
    }
    r = tcl_eval(tcl, tcl_string(cond), tcl_length(cond) + 1);
    DBG("FREE tcl_cmd_if %x.", cond);
    tcl_free(cond);
    if (r != FNORMAL) {
      DBG("FREE tcl_cmd_if %x.", branch);
      tcl_free(branch);
      break;
    }
    if (tcl_int(tcl->result)) {
      r = tcl_eval(tcl, tcl_string(branch), tcl_length(branch) + 1);
      DBG("FREE tcl_cmd_if %x.", branch);
      tcl_free(branch);
      break;
    }
    i = i + 2;
    DBG("FREE tcl_cmd_if %x.", branch);
    tcl_free(branch);
  }
  return r;
}

static int tcl_cmd_flow(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  int r = FERROR;
  tcl_value_t* flowval = tcl_list_at(args, 0);
  const char* flow = tcl_string(flowval);
  if (strcmp(flow, "break") == 0) {
    r = FBREAK;
  } else if (strcmp(flow, "continue") == 0) {
    r = FAGAIN;
  } else if (strcmp(flow, "return") == 0) {
    r = tcl_result(tcl, FRETURN, tcl_list_at(args, 1));
  }
  DBG("FREE tcl_cmd_flow %x.", flowval);
  tcl_free(flowval);
  return r;
}

static int tcl_cmd_while(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  tcl_value_t* cond = tcl_list_at(args, 1);
  tcl_value_t* loop = tcl_list_at(args, 2);
  int r;
  for (;;) {
    r = tcl_eval(tcl, tcl_string(cond), tcl_length(cond) + 1);
    if (r != FNORMAL) {
      DBG("FREE tcl_cmd_while %x.", cond);
      tcl_free(cond);
      DBG("FREE tcl_cmd_while %x.", cond);
      tcl_free(loop);
      return r;
    }
    if (!tcl_int(tcl->result)) {
      DBG("FREE tcl_cmd_while %x.", cond);
      tcl_free(cond);
      DBG("FREE tcl_cmd_while %x.", cond);
      tcl_free(loop);
      return FNORMAL;
    }
    int r = tcl_eval(tcl, tcl_string(loop), tcl_length(loop) + 1);
    switch (r) {
    case FBREAK:
      DBG("FREE tcl_cmd_while %x.", cond);
      tcl_free(cond);
      DBG("FREE tcl_cmd_while %x.", cond);
      tcl_free(loop);
      return FNORMAL;
    case FRETURN:
      DBG("FREE tcl_cmd_while %x.", cond);
      tcl_free(cond);
      DBG("FREE tcl_cmd_while %x.", cond);
      tcl_free(loop);
      return FRETURN;
    case FAGAIN:
      continue;
    case FERROR:
      DBG("FREE tcl_cmd_while %x.", cond);
      tcl_free(cond);
      DBG("FREE tcl_cmd_while %x.", cond);
      tcl_free(loop);
      return FERROR;
    }
  }
  return FERROR;
}

#ifndef TCL_DISABLE_MATH
/*
 * Math commands
 */
static int tcl_cmd_math(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  char buf[10];
  tcl_value_t* opval = tcl_list_at(args, 0);
  tcl_value_t* aval = tcl_list_at(args, 1);
  tcl_value_t* bval = tcl_list_at(args, 2);
  const char* op = tcl_string(opval);
  float a = tcl_float(aval);
  float b = tcl_float(bval);
  float c = 0;
  if (op[0] == '+') {
    c = a + b;
  } else if (op[0] == '-') {
    c = a - b;
  } else if (op[0] == '*') {
    c = a * b;
  } else if (op[0] == '/') {
    // Error reporting
    if (b == 0) {
      TCL_ERROR("Can't divide by 0");
      tcl_free(opval);
      tcl_free(aval);
      tcl_free(bval);
      return tcl_result(tcl, FERROR, tcl_alloc("", 0));
    }
    c = a / b;
  } else if (op[0] == '>' && op[1] == '\0') {
    c = a > b;
  } else if (op[0] == '>' && op[1] == '=') {
    c = a >= b;
  } else if (op[0] == '<' && op[1] == '\0') {
    c = a < b;
  } else if (op[0] == '<' && op[1] == '=') {
    c = a <= b;
  } else if (op[0] == '=' && op[1] == '=') {
    c = a == b;
  } else if (op[0] == '!' && op[1] == '=') {
    c = a != b;
  } else if (op[0] == '&' && op[1] == '&') {
    c = a && b;
  } else if (op[0] == '|' && op[1] == '|') {
    c = a || b;
  }
  // Check for c type, int or float
  if ((float)(int)c == c) chsnprintf(&buf[0], sizeof(buf), "%d", (int)c);
  else chsnprintf(&buf[0], sizeof(buf), "%.2f", c);

  tcl_free(opval);
  tcl_free(aval);
  tcl_free(bval);
  return tcl_result(tcl, FNORMAL, tcl_alloc(&buf[0], strlen(buf)));
}
/*
 * NOT command
 */
static int tcl_cmd_not(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  char buf[10];

  tcl_value_t* aval = tcl_list_at(args, 1);
  float c = !(tcl_float(aval));

  // Check for c type, int or float
  if ((float)(int)c == c) chsnprintf(&buf[0], sizeof(buf), "%d", (int)c);
  else chsnprintf(&buf[0], sizeof(buf), "%.2f", c);

  tcl_free(aval);
  return tcl_result(tcl, FNORMAL, tcl_alloc(&buf[0], strlen(buf)));
}
#endif /* TCL_DISABLE_MATH */
/*
 * Return the string length of int value
 */
uint8_t lenHelper(unsigned x) {
    if (x < 10)   return 1;
    if (x < 100)  return 2;
    if (x < 1000) return 3;
    else return 4;
}
/*
 * String manipulation
 */
static int tcl_cmd_string(struct tcl* tcl, tcl_value_t* args, void* arg) {
  (void)arg;
  uint8_t ret;
  tcl_value_t* aval;
  char buf[lenHelper(MAX_VAR_LENGTH) + 1];

  tcl_value_t* sub_cmd = tcl_list_at(args, 1);

  switch (*sub_cmd) {
    case 'c': // compare
      ARITY((tcl_list_length(args) == 4), sub_cmd, 2);
      aval = tcl_list_at(args, 2);
      tcl_value_t* bval = tcl_list_at(args, 3);
      if (!strcmp(aval, bval)) ret = tcl_result(tcl, FNORMAL, tcl_alloc("1", 1));
      else                     ret = tcl_result(tcl, FNORMAL, tcl_alloc("0", 1));
      tcl_free(bval);
      tcl_free(aval);
      break;
    case 'l': // length
      ARITY((tcl_list_length(args) == 3), sub_cmd, 1);
      aval = tcl_list_at(args, 2);
      uint8_t resp = chsnprintf(&buf[0], sizeof(buf), "%d", strlen(aval));
      ret = tcl_result(tcl, FNORMAL, tcl_alloc(buf, resp));
      tcl_free(aval);
      break;
    default: // error
      SUBCMDERROR("c_ompare|l_ength");
      ret = tcl_result(tcl, FERROR, tcl_alloc("", 0));
      break;
  }

  tcl_free(sub_cmd);
  return ret;
}
/*
 *
 */
void tcl_init(struct tcl* tcl, uint16_t max_iterations, BaseSequentialStream *output) {
  tcl_output = output;
  tcl_iteration = max_iterations;
  tcl->env = tcl_env_alloc(NULL);
  tcl->result = tcl_alloc("", 0);
  tcl->cmds = NULL;
  tcl_register(tcl, "set", tcl_cmd_set, 0, NULL,
               "assigns value to the variable (if any) and returns the current variable value");
  tcl_register(tcl, "subst", tcl_cmd_subst, 2, NULL,
               "does command substitution in the argument string");
#ifndef TCL_DISABLE_PUTS
  tcl_register(tcl, "puts", tcl_cmd_puts, 2, NULL,
               "prints argument to the stdout, followed by a newline");
#endif
  tcl_register(tcl, "proc", tcl_cmd_proc, 4, NULL,
               "creates a new user defined command");
  tcl_register(tcl, "if", tcl_cmd_if, 0, NULL,
               "does a simple 'if {cond} {then} {cond2} {then2} {else}'");
  tcl_register(tcl, "while", tcl_cmd_while, 3, NULL,
               "runs a while loop 'while {cond} {body}', use of 'break',"
               "'continue' or 'return' is allowed");
  tcl_register(tcl, "return", tcl_cmd_flow, 0, NULL,
               "return from procedure, or set return code");
  tcl_register(tcl, "break", tcl_cmd_flow, 1, NULL,
               "forces loop to terminate");
  tcl_register(tcl, "continue", tcl_cmd_flow, 1, NULL,
               "forces the next iteration of the loop");
#ifndef TCL_DISABLE_MATH
  for (unsigned int i = 0; i < (sizeof(math) / sizeof(math[0])); i++) {
    tcl_register(tcl, math[i], tcl_cmd_math, 3, NULL, NULL);
  }
  tcl_register(tcl, "!", tcl_cmd_not, 2, NULL,
               "logical not");
#endif
  tcl_register(tcl, "string", tcl_cmd_string, 0, NULL,
               "string manipulation, (c_ompare|l_ength)");
}

void tcl_destroy(struct tcl* tcl) {
  while (tcl->env) {
    tcl->env = tcl_env_free(tcl->env);
  }
  while (tcl->cmds) {
    struct tcl_cmd* cmd = tcl->cmds;
    tcl->cmds = tcl->cmds->next;
    tcl_free(cmd->name);
    tcl_free(cmd->arg);
    tcl_free(cmd);
  }
  DBG("FREE tcl_destroy %x.", tcl->result);
  tcl_free(tcl->result);
}
/*
 * Print tcl.env.vars to stream
 */
void tcl_list_var(struct tcl* tcl, BaseSequentialStream **output, char *separator) {
  struct tcl_var* var;

  for (var = tcl->env->vars; var != NULL; var = var->next) {
    chprintf(*output, "%s = %s", var->name, var->value);
    if (separator) chprintf(*output, "%s", separator);
  }
}
/*
 * Print registered tcl.cmds to stream
 *
 * Options, as binary flags on bits
 * 0 - print arity
 * 1 - print math commands
 * 2 - print flow commands
 * 3 - print other then math & flow commands
 * 4 - print description
 * 5 - print math description
 */
void tcl_list_cmd(struct tcl* tcl, BaseSequentialStream **output, char *separator,
                  const uint8_t options) {
  struct tcl_cmd* cmd;

  for (cmd = tcl->cmds; cmd != NULL; cmd = cmd->next) {
    // Command options
    if ((cmd->fn == tcl_cmd_math) && !((options >> 1) & 0b1)) continue;;
    if ((cmd->fn == tcl_cmd_flow) && !((options >> 2) & 0b1)) continue;
    if ((cmd->fn != tcl_cmd_math) && (cmd->fn != tcl_cmd_flow) && !((options >> 3) & 0b1)) continue;

    chprintf(*output, "%s", cmd->name);
    if (options & 0b1) chprintf(*output, " (%u)", (cmd->arity ? cmd->arity - 1 : 0));
    if ((options >> 4) & 0b1)  chprintf(*output, " - %s.", cmd->description);
    if ((separator) && (cmd->next != NULL)) chprintf(*output, "%s", separator);
  }
#ifndef TCL_DISABLE_MATH
  // Math commands description
  if ((options >> 5) & 0b1) {
    if (separator) chprintf(*output, "%s", separator);
    chprintf(*output, "operators:");
    for (uint8_t i = 0; i < (sizeof(math) / sizeof(math[0])); i++) {
      chprintf(*output, "%s %s", (i ? "," : ""), math[i]);
    }
    chprintf(*output, ".");
  }
#endif
}
