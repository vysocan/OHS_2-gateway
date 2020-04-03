#ifndef __TCL_H__
#define __TCL_H__

#ifdef __cplusplus
extern "C" {
#endif

struct tcl;

/* Token type and control flow constants */
enum { TCMD,
  TWORD,
  TPART,
  TERROR };
enum { FERROR,
  FNORMAL,
  FRETURN,
  FBREAK,
  FAGAIN };

#ifndef TCL_DISABLE_PUTS
void tcl_set_puts(int (*pFunc)(const char*));
#endif


/* A helper parser struct and macro (requires C99) */
struct tcl_parser {
  const char* from;
  const char* to;
  const char* start;
  const char* end;
  int q;
  int token;
};
#define tcl_each(s, len, skiperr)                                                                                          \
  for (struct tcl_parser p = { NULL, NULL, (s), (s) + (len), 0, TERROR };                                                  \
       p.start < p.end && (((p.token = tcl_next(p.start, p.end - p.start, &p.from, &p.to, &p.q)) != TERROR) || (skiperr)); \
       p.start = p.to)

typedef char tcl_value_t;

#define TCL_DISABLE_MALLOC 1
#ifndef TCL_DISABLE_MALLOC

#define TCL_FREE(m) free(m)
#define TCL_MALLOC(m) malloc(m)
#define TCL_REALLOC(m, n) realloc(m, n)

#endif

#define TCL_FREE(m) sfree(m)
#define TCL_MALLOC(m) smalloc(m)
#define TCL_REALLOC(m, n) srealloc(m, n)


//****
#define MAX_STRING_LENGTH ((uint8_t)200)
#define MAX_AMOUNT_OF_STRINGTH ((uint8_t)100)

#define IN_USE ((uint8_t)1)
#define NOT_IN_USE ((uint8_t)0)
//****

const char* tcl_string(tcl_value_t* v);
int tcl_int(tcl_value_t* v);
int tcl_length(tcl_value_t* v);

/* void tcl_free(tcl_value_t* v); */

int tcl_next(const char* s, size_t n, const char** from, const char** to,
    int* q);

tcl_value_t* tcl_append_string(tcl_value_t* v, const char* s, size_t len);
tcl_value_t* tcl_append(tcl_value_t* v, tcl_value_t* tail);
tcl_value_t* tcl_alloc(const char* s, size_t len);
tcl_value_t* tcl_dup(tcl_value_t* v);
tcl_value_t* tcl_list_alloc();
void tcl_list_free(tcl_value_t* v);
tcl_value_t* tcl_list_at(tcl_value_t* v, int index);
tcl_value_t* tcl_list_append(tcl_value_t* v, tcl_value_t* tail);

typedef int (*tcl_cmd_fn_t)(struct tcl*, tcl_value_t*, void*);

struct tcl_cmd {
  tcl_value_t* name;
  int arity;
  tcl_cmd_fn_t fn;
  void* arg;
  struct tcl_cmd* next;
};

struct tcl_var {
  tcl_value_t* name;
  tcl_value_t* value;
  struct tcl_var* next;
};

struct tcl_env {
  struct tcl_var* vars;
  struct tcl_env* parent;
};

struct tcl {
  struct tcl_env* env;
  struct tcl_cmd* cmds;
  tcl_value_t* result;
};

struct tcl_env* tcl_env_alloc(struct tcl_env* parent);
struct tcl_var* tcl_env_var(struct tcl_env* env, tcl_value_t* name);
struct tcl_env* tcl_env_free(struct tcl_env* env);
tcl_value_t* tcl_var(struct tcl* tcl, tcl_value_t* name, tcl_value_t* v);
int tcl_result(struct tcl* tcl, int flow, tcl_value_t* result);
int tcl_subst(struct tcl* tcl, const char* s, size_t len);
int tcl_eval(struct tcl* tcl, const char* s, size_t len);
void tcl_register(struct tcl* tcl, const char* name, tcl_cmd_fn_t fn, int arity,
    void* arg);
void tcl_init(struct tcl* tcl);
void tcl_destroy(struct tcl* tcl);

#ifdef __cplusplus
}
#endif
#endif
