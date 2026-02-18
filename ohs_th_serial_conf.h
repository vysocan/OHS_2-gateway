/*
 * ohs_th_serial_conf.h
 *
 * Serial port configuration read/write thread.
 * Uses MsgPack format over USART1 (SD1) via CMP library (camgunz/cmp).
 *
 * Frame: | STX (0x02) | CMD (1B) | LEN (2B LE) | MSGPACK (LEN B) | CRC16 (2B) |
 *
 * Field dispatch is table-driven via scFieldTable[].
 *
 * Created on: 13. 2. 2026
 *     Author: OHS
 */

#ifndef OHS_TH_SERIAL_CONF_H_
#define OHS_TH_SERIAL_CONF_H_

#include "cmp.h"

#ifndef SERIAL_CONF_DEBUG
#define SERIAL_CONF_DEBUG 0
#endif

#if SERIAL_CONF_DEBUG
#define DBG_SC(...) {chprintf(console, __VA_ARGS__);}
#else
#define DBG_SC(...)
#endif

/* Protocol constants */
#define SC_STX           0x02
#define SC_CMD_READ      0x01
#define SC_CMD_READ_RSP  0x02
#define SC_CMD_WRITE     0x03
#define SC_CMD_WRITE_RSP 0x04

/* Buffer sizes */
#define SC_BUF_SIZE       256

/* ========================================================================= */
/*  CMP memory-buffer I/O callbacks                                          */
/* ========================================================================= */

typedef struct {
  uint8_t *buf;
  size_t   pos;
  size_t   len;
} scBufCtx_t;

static bool scBufReader(cmp_ctx_t *ctx, void *data, size_t count) {
  scBufCtx_t *bc = (scBufCtx_t *)ctx->buf;
  if (bc->pos + count > bc->len) return false;
  memcpy(data, bc->buf + bc->pos, count);
  bc->pos += count;
  return true;
}

static bool scBufSkipper(cmp_ctx_t *ctx, size_t count) {
  scBufCtx_t *bc = (scBufCtx_t *)ctx->buf;
  if (bc->pos + count > bc->len) return false;
  bc->pos += count;
  return true;
}

static size_t scBufWriter(cmp_ctx_t *ctx, const void *data, size_t count) {
  scBufCtx_t *bc = (scBufCtx_t *)ctx->buf;
  if (bc->pos + count > bc->len) return 0;
  memcpy(bc->buf + bc->pos, data, count);
  bc->pos += count;
  return count;
}

/* Init a CMP context backed by a memory buffer */
static void scCmpInit(cmp_ctx_t *cmp, scBufCtx_t *bc, uint8_t *buf, size_t len) {
  bc->buf = buf;
  bc->pos = 0;
  bc->len = len;
  cmp_init(cmp, bc, scBufReader, scBufSkipper, scBufWriter);
}

/* Current position in the write buffer */
#define SC_CMP_POS(bc) ((bc)->pos)

/* ========================================================================= */
/*  Field descriptor table                                                   */
/* ========================================================================= */

typedef enum {
  SC_U8,     /* uint8_t   */
  SC_U16,    /* uint16_t  */
  SC_U32,    /* uint32_t  */
  SC_I16,    /* int16_t   */
  SC_FLOAT,  /* float     */
  SC_STR,    /* char[]    */
  SC_STRUCT  /* struct with custom encode/decode */
} scType_t;

typedef struct scFieldDesc scFieldDesc_t;

typedef bool (*scStructEnc_t)(cmp_ctx_t *cmp, scBufCtx_t *bc, const void *data);
typedef bool (*scStructDec_t)(cmp_ctx_t *cmp, void *data);

struct scFieldDesc {
  const char    *name;      /* name of the field */
  uint8_t        nameLen;   /* length of the name */
  scType_t       type;      /* type of the field */
  void          *data;      /* pointer to the data */ 
  uint8_t        maxLen;    /* for SC_STR: max buffer size */
  uint8_t        arraySize; /* 0=scalar, N=array of N */
  uint16_t       elemSize;  /* sizeof one element */
  scStructEnc_t  encode;    /* for SC_STRUCT */
  scStructDec_t  decode;    /* for SC_STRUCT */
};

/* ========================================================================= */
/*  Struct encoders/decoders                                                 */
/* ========================================================================= */
/* ========================================================================= */
/*  Pack helpers (inline functions)                                          */
/* ========================================================================= */

/*
 * @brief Encode a string field
 * @param ctx CMP context
 * @param key Key string
 * @param keyLen Length of the key string
 * @param value Value string
 * @param maxLen Maximum length of the value string
 */
static inline void scEncStr(cmp_ctx_t *ctx, const char *key, uint8_t keyLen,
                            const char *value, uint8_t maxLen) {
  cmp_write_str(ctx, key, keyLen);
  cmp_write_str(ctx, value, (uint32_t)strnlen(value, maxLen));
}

/*
 * @brief Encode an unsigned integer field
 * @param ctx CMP context
 * @param key Key string
 * @param keyLen Length of the key string
 * @param value Value
 */
static inline void scEncUint(cmp_ctx_t *ctx, const char *key, uint8_t keyLen,
                             uint64_t value) {
  cmp_write_str(ctx, key, keyLen);
  cmp_write_uinteger(ctx, value);
}

/*
 * @brief Encode a signed integer field
 * @param ctx CMP context
 * @param key Key string
 * @param keyLen Length of the key string
 * @param value Value
 */
static inline void scEncInt(cmp_ctx_t *ctx, const char *key, uint8_t keyLen,
                            int64_t value) {
  cmp_write_str(ctx, key, keyLen);
  cmp_write_integer(ctx, value);
}

/*
 * @brief Encode a float field
 * @param ctx CMP context
 * @param key Key string
 * @param keyLen Length of the key string
 * @param value Value
 */
static inline void scEncFloat(cmp_ctx_t *ctx, const char *key, uint8_t keyLen,
                              float value) {
  cmp_write_str(ctx, key, keyLen);
  cmp_write_float(ctx, value);
}
/*
 * @brief Encode a char field
 * @param ctx CMP context
 * @param key Key string
 * @param keyLen Length of the key string
 * @param value Value
 */
static inline void scEncChar(cmp_ctx_t *ctx, const char *key, uint8_t keyLen,
                             char value) {
  cmp_write_str(ctx, key, keyLen);
  cmp_write_str(ctx, &value, 1);
}

/* ========================================================================= */
/*  Unpack helpers (functions)                                               */
/* ========================================================================= */

/*
 * @brief Read a map key string into keyBuf, return key length or 0 on error
 * @param ctx CMP context
 * @param keyBuf Destination buffer
 * @param keyBufSize Maximum length of the destination buffer
 * @return Length of the key string or 0 if error
 */
static uint8_t scReadMapKey(cmp_ctx_t *ctx, char *keyBuf, uint8_t keyBufSize) {
  uint32_t ksz;

  if (!cmp_read_str_size(ctx, &ksz)) return 0;
  if (ksz > keyBufSize || !scBufReader(ctx, keyBuf, ksz)) return 0;
  return (uint8_t)ksz;
}

/*
 * @brief Decode a string field
 * @param ctx CMP context
 * @param dest Destination buffer
 * @param maxLen Maximum length of the destination buffer
 * @return true if successful, false otherwise
 */
static bool scDecStr(cmp_ctx_t *ctx, char *dest, uint8_t maxLen) {
  uint32_t ssz;

  memset(dest, 0, maxLen);
  if (!cmp_read_str_size(ctx, &ssz)) return false;
  uint8_t cl = (ssz < maxLen) ? (uint8_t)ssz : (uint8_t)(maxLen - 1);
  if (ssz > 0 && !scBufReader(ctx, dest, ssz)) return false;
  dest[cl] = '\0';
  return true;
}

/* 
 * @brief Decode a char field
 * @param ctx CMP context
 * @param dest Destination buffer
 * @return true if successful, false otherwise
 */
static bool scDecChar(cmp_ctx_t *ctx, char *dest) {
  uint32_t sl;

  if (!cmp_read_str_size(ctx, &sl)) return false;
  char cv[2] = {0};
  if (sl > 0 && !scBufReader(ctx, cv, sl > 1 ? 1 : sl)) return false;
  if (sl > 1) scBufSkipper(ctx, sl - 1);
  *dest = cv[0];
  return true;
}

/* ========================================================================= */
/*  Struct encoders/decoders                                                 */
/* ========================================================================= */

/* --- group_conf_t --- */
static bool scEncGroup(cmp_ctx_t *c, scBufCtx_t *bc, const void *data) {
  (void)bc;
  const group_conf_t *g = (const group_conf_t *)data;

  cmp_write_map(c, 2);
  scEncStr(c, "nm", 2, g->name, NAME_LENGTH);
  scEncUint(c, "set", 3, g->setting);
  return c->error == 0;
}
static bool scDecGroup(cmp_ctx_t *c, void *data) {
  uint32_t mc;
  group_conf_t *g = (group_conf_t *)data;

  if (!cmp_read_map(c, &mc)) return false;
  for (uint32_t m = 0; m < mc; m++) {
    char kb[8]; uint8_t kl = scReadMapKey(c, kb, sizeof(kb));
    if (kl == 0) return false;
    if (safeStrcmp2(kb, kl, "nm", 2) == 0) { if (!scDecStr(c, g->name, NAME_LENGTH)) return false; }
    else if (safeStrcmp2(kb, kl, "set", 3) == 0) { if (!cmp_read_ushort(c, &g->setting)) return false; }
    else { cmp_skip_object_no_limit(c); }
  }
  return c->error == 0;
}

/* --- contact_conf_t --- */
static bool scEncContact(cmp_ctx_t *c, scBufCtx_t *bc, const void *data) {
  (void)bc;
  const contact_conf_t *ct = (const contact_conf_t *)data;

  cmp_write_map(c, 4);
  scEncStr(c, "nm", 2, ct->name, NAME_LENGTH);
  scEncStr(c, "ph", 2, ct->phone, PHONE_LENGTH);
  scEncStr(c, "eml", 3, ct->email, EMAIL_LENGTH);
  scEncUint(c, "set", 3, ct->setting);
  return c->error == 0;
}
static bool scDecContact(cmp_ctx_t *c, void *data) {
  uint32_t mc;
  contact_conf_t *ct = (contact_conf_t *)data;

  if (!cmp_read_map(c, &mc)) return false;
  for (uint32_t m = 0; m < mc; m++) {
    char kb[8]; uint8_t kl = scReadMapKey(c, kb, sizeof(kb));
    if (kl == 0) return false;
    if (safeStrcmp2(kb, kl, "nm", 2) == 0) { if (!scDecStr(c, ct->name, NAME_LENGTH)) return false; }
    else if (safeStrcmp2(kb, kl, "ph", 2) == 0) { if (!scDecStr(c, ct->phone, PHONE_LENGTH)) return false; }
    else if (safeStrcmp2(kb, kl, "eml", 3) == 0) { if (!scDecStr(c, ct->email, EMAIL_LENGTH)) return false; }
    else if (safeStrcmp2(kb, kl, "set", 3) == 0) { if (!cmp_read_uchar(c, &ct->setting)) return false; }
    else { cmp_skip_object_no_limit(c); }
  }
  return c->error == 0;
}

/* --- key_conf_t --- */
static bool scEncKey(cmp_ctx_t *c, scBufCtx_t *bc, const void *data) {
  (void)bc;
  const key_conf_t *k = (const key_conf_t *)data;

  cmp_write_map(c, 3);
  scEncUint(c, "val", 3, k->value);
  scEncUint(c, "set", 3, k->setting);
  scEncUint(c, "con", 3, k->contact);
  return c->error == 0;
}
static bool scDecKey(cmp_ctx_t *c, void *data) {
  uint32_t mc;
  key_conf_t *k = (key_conf_t *)data;

  if (!cmp_read_map(c, &mc)) return false;
  for (uint32_t m = 0; m < mc; m++) {
    char kb[8]; uint8_t kl = scReadMapKey(c, kb, sizeof(kb));
    if (kl == 0) return false;
    if (safeStrcmp2(kb, kl, "val", 3) == 0) { if (!cmp_read_uint(c, &k->value)) return false; }
    else if (safeStrcmp2(kb, kl, "set", 3) == 0) { if (!cmp_read_uchar(c, &k->setting)) return false; }
    else if (safeStrcmp2(kb, kl, "con", 3) == 0) { if (!cmp_read_uchar(c, &k->contact)) return false; }
    else { cmp_skip_object_no_limit(c); }
  }
  return c->error == 0;
}

/* --- calendar_t (timer) - excluding runtime nextOn/nextOff --- */
static bool scEncTimer(cmp_ctx_t *c, scBufCtx_t *bc, const void *data) {
  (void)bc;
  const calendar_t *t = (const calendar_t *)data;

  cmp_write_map(c, 11);
  scEncUint(c, "set", 3, t->setting);
  scEncUint(c, "per", 3, t->periodTime);
  scEncUint(c, "sta", 3, t->startTime);
  scEncUint(c, "run", 3, t->runTime);
  scEncFloat(c, "con", 3, t->constantOn);
  scEncFloat(c, "cof", 3, t->constantOff);
  scEncUint(c, "toa", 3, t->toAddress);
  scEncChar(c, "tof", 3, t->toFunction);
  scEncUint(c, "ton", 3, t->toNumber);
  scEncStr(c, "nm", 2, t->name, NAME_LENGTH);
  scEncStr(c, "scr", 3, t->evalScript, NAME_LENGTH);
  return c->error == 0;
}
static bool scDecTimer(cmp_ctx_t *c, void *data) {
  uint32_t mc;
  calendar_t *t = (calendar_t *)data;

  if (!cmp_read_map(c, &mc)) return false;
  for (uint32_t m = 0; m < mc; m++) {
    char kb[8]; uint8_t kl = scReadMapKey(c, kb, sizeof(kb));
    if (kl == 0) return false;
    if (safeStrcmp2(kb, kl, "set", 3) == 0) { if (!cmp_read_ushort(c, &t->setting)) return false; }
    else if (safeStrcmp2(kb, kl, "per", 3) == 0) { if (!cmp_read_uchar(c, &t->periodTime)) return false; }
    else if (safeStrcmp2(kb, kl, "sta", 3) == 0) { if (!cmp_read_ushort(c, &t->startTime)) return false; }
    else if (safeStrcmp2(kb, kl, "run", 3) == 0) { if (!cmp_read_uchar(c, &t->runTime)) return false; }
    else if (safeStrcmp2(kb, kl, "con", 3) == 0) { if (!cmp_read_float(c, &t->constantOn)) return false; }
    else if (safeStrcmp2(kb, kl, "cof", 3) == 0) { if (!cmp_read_float(c, &t->constantOff)) return false; }
    else if (safeStrcmp2(kb, kl, "toa", 3) == 0) { if (!cmp_read_uchar(c, &t->toAddress)) return false; }
    else if (safeStrcmp2(kb, kl, "tof", 3) == 0) { if (!scDecChar(c, &t->toFunction)) return false; }
    else if (safeStrcmp2(kb, kl, "ton", 3) == 0) { if (!cmp_read_uchar(c, &t->toNumber)) return false; }
    else if (safeStrcmp2(kb, kl, "nm", 2) == 0) { if (!scDecStr(c, t->name, NAME_LENGTH)) return false; }
    else if (safeStrcmp2(kb, kl, "scr", 3) == 0) { if (!scDecStr(c, t->evalScript, NAME_LENGTH)) return false; }
    else { cmp_skip_object_no_limit(c); }
  }
  return c->error == 0;
}

/* --- trigger_t - excluding runtime nextOff --- */
static bool scEncTrigger(cmp_ctx_t *c, scBufCtx_t *bc, const void *data) {
  (void)bc;
  const trigger_t *t = (const trigger_t *)data;

  cmp_write_map(c, 16);
  scEncUint(c, "set", 3, t->setting);
  scEncUint(c, "adr", 3, t->address);
  scEncChar(c, "typ", 3, t->type);
  scEncChar(c, "fnc", 3, t->function);
  scEncUint(c, "num", 3, t->number);
  scEncUint(c, "cnd", 3, t->condition);
  scEncFloat(c, "val", 3, t->value);
  scEncFloat(c, "con", 3, t->constantOn);
  scEncFloat(c, "cof", 3, t->constantOff);
  scEncUint(c, "toa", 3, t->toAddress);
  scEncChar(c, "tof", 3, t->toFunction);
  scEncUint(c, "ton", 3, t->toNumber);
  scEncUint(c, "oft", 3, t->offTime);
  scEncStr(c, "nm", 2, t->name, NAME_LENGTH);
  scEncStr(c, "scr", 3, t->evalScript, NAME_LENGTH);
  scEncFloat(c, "hys", 3, t->hysteresis);
  return c->error == 0;
}
static bool scDecTrigger(cmp_ctx_t *c, void *data) {
  uint32_t mc;
  trigger_t *t = (trigger_t *)data;

  if (!cmp_read_map(c, &mc)) return false;
  for (uint32_t m = 0; m < mc; m++) {
    char kb[8]; uint8_t kl = scReadMapKey(c, kb, sizeof(kb));
    if (kl == 0) return false;
    if (safeStrcmp2(kb, kl, "set", 3) == 0) { if (!cmp_read_ushort(c, &t->setting)) return false; }
    else if (safeStrcmp2(kb, kl, "adr", 3) == 0) { if (!cmp_read_uchar(c, &t->address)) return false; }
    else if (safeStrcmp2(kb, kl, "typ", 3) == 0) { if (!scDecChar(c, &t->type)) return false; }
    else if (safeStrcmp2(kb, kl, "fnc", 3) == 0) { if (!scDecChar(c, &t->function)) return false; }
    else if (safeStrcmp2(kb, kl, "num", 3) == 0) { if (!cmp_read_uchar(c, &t->number)) return false; }
    else if (safeStrcmp2(kb, kl, "cnd", 3) == 0) { if (!cmp_read_uchar(c, &t->condition)) return false; }
    else if (safeStrcmp2(kb, kl, "val", 3) == 0) { if (!cmp_read_float(c, &t->value)) return false; }
    else if (safeStrcmp2(kb, kl, "con", 3) == 0) { if (!cmp_read_float(c, &t->constantOn)) return false; }
    else if (safeStrcmp2(kb, kl, "cof", 3) == 0) { if (!cmp_read_float(c, &t->constantOff)) return false; }
    else if (safeStrcmp2(kb, kl, "toa", 3) == 0) { if (!cmp_read_uchar(c, &t->toAddress)) return false; }
    else if (safeStrcmp2(kb, kl, "tof", 3) == 0) { if (!scDecChar(c, &t->toFunction)) return false; }
    else if (safeStrcmp2(kb, kl, "ton", 3) == 0) { if (!cmp_read_uchar(c, &t->toNumber)) return false; }
    else if (safeStrcmp2(kb, kl, "oft", 3) == 0) { if (!cmp_read_uchar(c, &t->offTime)) return false; }
    else if (safeStrcmp2(kb, kl, "nm", 2) == 0) { if (!scDecStr(c, t->name, NAME_LENGTH)) return false; }
    else if (safeStrcmp2(kb, kl, "scr", 3) == 0) { if (!scDecStr(c, t->evalScript, NAME_LENGTH)) return false; }
    else if (safeStrcmp2(kb, kl, "hys", 3) == 0) { if (!cmp_read_float(c, &t->hysteresis)) return false; }
    else { cmp_skip_object_no_limit(c); }
  }
  return c->error == 0;
}

/* --- mqtt_conf_t --- */
static bool scEncMqtt(cmp_ctx_t *c, scBufCtx_t *bc, const void *data) {
  (void)bc;
  const mqtt_conf_t *m = (const mqtt_conf_t *)data;
  
  cmp_write_map(c, 5);
  scEncStr(c, "adr", 3, m->address, URL_LENGTH);
  scEncStr(c, "usr", 3, m->user, NAME_LENGTH);
  scEncStr(c, "pwd", 3, m->password, NAME_LENGTH);
  scEncUint(c, "prt", 3, m->port);
  scEncUint(c, "set", 3, m->setting);
  return c->error == 0;
}
static bool scDecMqtt(cmp_ctx_t *c, void *data) {
  uint32_t mc;
  mqtt_conf_t *m = (mqtt_conf_t *)data;

  if (!cmp_read_map(c, &mc)) return false;
  for (uint32_t m2 = 0; m2 < mc; m2++) {
    char kb[8]; uint8_t kl = scReadMapKey(c, kb, sizeof(kb));
    if (kl == 0) return false;
    if (safeStrcmp2(kb, kl, "adr", 3) == 0) { if (!scDecStr(c, m->address, URL_LENGTH)) return false; }
    else if (safeStrcmp2(kb, kl, "usr", 3) == 0) { if (!scDecStr(c, m->user, NAME_LENGTH)) return false; }
    else if (safeStrcmp2(kb, kl, "pwd", 3) == 0) { if (!scDecStr(c, m->password, NAME_LENGTH)) return false; }
    else if (safeStrcmp2(kb, kl, "prt", 3) == 0) { if (!cmp_read_ushort(c, &m->port)) return false; }
    else if (safeStrcmp2(kb, kl, "set", 3) == 0) { if (!cmp_read_ushort(c, &m->setting)) return false; }
    else { cmp_skip_object_no_limit(c); }
  }
  return c->error == 0;
}

/* ========================================================================= */
/*  Field table                                                              */
/* ========================================================================= */

#define SC_SCALAR(nm, nl, t, ptr) \
  { nm, nl, t, (void*)(ptr), 0, 0, 0, NULL, NULL }

#define SC_SCALAR_STR(nm, nl, ptr, ml) \
  { nm, nl, SC_STR, (void*)(ptr), ml, 0, 0, NULL, NULL }

#define SC_ARRAY(nm, nl, t, ptr, cnt, esz) \
  { nm, nl, t, (void*)(ptr), 0, cnt, esz, NULL, NULL }

#define SC_ARRAY_STR(nm, nl, ptr, ml, cnt, esz) \
  { nm, nl, SC_STR, (void*)(ptr), ml, cnt, esz, NULL, NULL }

#define SC_STRUCT_ENTRY(nm, nl, ptr, cnt, esz, enc_fn, dec_fn) \
  { nm, nl, SC_STRUCT, (void*)(ptr), 0, cnt, esz, enc_fn, dec_fn }

#define SC_STRUCT_SCALAR(nm, nl, ptr, enc_fn, dec_fn) \
  { nm, nl, SC_STRUCT, (void*)(ptr), 0, 0, 0, enc_fn, dec_fn }

static const scFieldDesc_t scFieldTable[] = {
  /* Simple scalars */
  SC_SCALAR("conf.versionMajor",   17, SC_U8,    &conf.versionMajor),
  SC_SCALAR("conf.versionMinor",   17, SC_U8,    &conf.versionMinor),
  SC_SCALAR("conf.logOffset",      14, SC_U16,   &conf.logOffset),
  SC_SCALAR("conf.armDelay",       13, SC_U8,    &conf.armDelay),
  SC_SCALAR("conf.autoArm",        12, SC_U8,    &conf.autoArm),
  SC_SCALAR("conf.openAlarm",      14, SC_U8,    &conf.openAlarm),
  SC_SCALAR("conf.tclSetting",     15, SC_U8,    &conf.tclSetting),
  SC_SCALAR("conf.tclIteration",   17, SC_U16,   &conf.tclIteration),
  SC_SCALAR("conf.systemFlags",    16, SC_U8,    &conf.systemFlags),
  SC_SCALAR("conf.SMTPPort",       13, SC_U16,   &conf.SMTPPort),
  /* int16 scalars */
  SC_SCALAR("conf.timeStdOffset",  18, SC_I16,   &conf.timeStdOffset),
  SC_SCALAR("conf.timeDstOffset",  18, SC_I16,   &conf.timeDstOffset),
  /* Time uint8 scalars */
  SC_SCALAR("conf.timeStdWeekNum", 19, SC_U8,    &conf.timeStdWeekNum),
  SC_SCALAR("conf.timeStdDow",     15, SC_U8,    &conf.timeStdDow),
  SC_SCALAR("conf.timeStdMonth",   17, SC_U8,    &conf.timeStdMonth),
  SC_SCALAR("conf.timeStdHour",    16, SC_U8,    &conf.timeStdHour),
  SC_SCALAR("conf.timeDstWeekNum", 19, SC_U8,    &conf.timeDstWeekNum),
  SC_SCALAR("conf.timeDstDow",     15, SC_U8,    &conf.timeDstDow),
  SC_SCALAR("conf.timeDstMonth",   17, SC_U8,    &conf.timeDstMonth),
  SC_SCALAR("conf.timeDstHour",    16, SC_U8,    &conf.timeDstHour),
  /* String scalars */
  SC_SCALAR_STR("conf.dateTimeFormat", 19, &conf.dateTimeFormat, NAME_LENGTH),
  SC_SCALAR_STR("conf.SNTPAddress",    16, &conf.SNTPAddress,    URL_LENGTH),
  SC_SCALAR_STR("conf.SMTPAddress",    16, &conf.SMTPAddress,    URL_LENGTH),
  SC_SCALAR_STR("conf.SMTPUser",       13, &conf.SMTPUser,       EMAIL_LENGTH),
  SC_SCALAR_STR("conf.SMTPPassword",   17, &conf.SMTPPassword,   NAME_LENGTH),
  SC_SCALAR_STR("conf.user",            9, &conf.user,           NAME_LENGTH),
  SC_SCALAR_STR("conf.password",       13, &conf.password,       NAME_LENGTH),
  SC_SCALAR_STR("conf.radioKey",       13, &conf.radioKey,       RADIO_KEY_SIZE),
  /* Simple arrays */
  SC_ARRAY("conf.zone",        9, SC_U16,  &conf.zone,        ALARM_ZONES,            sizeof(uint16_t)),
  SC_ARRAY("conf.zoneAddress", 16, SC_U8,  &conf.zoneAddress,  ALARM_ZONES - HW_ZONES, sizeof(uint8_t)),
  SC_ARRAY("conf.dummy",      10, SC_U32,  &conf.dummy,        3,                      sizeof(uint32_t)),
  SC_ARRAY("conf.alert",      10, SC_U32,  &conf.alert,        ARRAY_SIZE(alertType),  sizeof(uint32_t)),
  /* String arrays */
  SC_ARRAY_STR("conf.zoneName", 13, &conf.zoneName, NAME_LENGTH, ALARM_ZONES, NAME_LENGTH),
  /* Struct arrays */
  SC_STRUCT_ENTRY("conf.group",    10, &conf.group,   ALARM_GROUPS,  sizeof(group_conf_t),   scEncGroup,   scDecGroup),
  SC_STRUCT_ENTRY("conf.contact",  12, &conf.contact, CONTACTS_SIZE, sizeof(contact_conf_t), scEncContact, scDecContact),
  SC_STRUCT_ENTRY("conf.key",       8, &conf.key,     KEYS_SIZE,     sizeof(key_conf_t),     scEncKey,     scDecKey),
  SC_STRUCT_ENTRY("conf.timer",    10, &conf.timer,   TIMER_SIZE,    sizeof(calendar_t),     scEncTimer,   scDecTimer),
  SC_STRUCT_ENTRY("conf.trigger",  12, &conf.trigger, TRIGGER_SIZE,  sizeof(trigger_t),      scEncTrigger, scDecTrigger),
  /* Struct scalar */
  SC_STRUCT_SCALAR("conf.mqtt",     9, &conf.mqtt,    scEncMqtt,     scDecMqtt),
};

#define SC_FIELD_COUNT (sizeof(scFieldTable) / sizeof(scFieldTable[0]))

/* ========================================================================= */
/*  Generic encode/decode using field table                                  */
/* ========================================================================= */

static const scFieldDesc_t* scFindField(const char *name, uint8_t nameLen) {
  for (uint16_t i = 0; i < SC_FIELD_COUNT; i++) {
    if (safeStrcmp2(name, nameLen, scFieldTable[i].name, scFieldTable[i].nameLen) == 0) {
      return &scFieldTable[i];
    }
  }
  return NULL;
}

static void* scGetElemPtr(const scFieldDesc_t *fd, uint8_t idx) {
  if (fd->arraySize > 0) {
    return (uint8_t*)fd->data + (uint16_t)idx * fd->elemSize;
  }
  return fd->data;
}

static bool scEncodeField(cmp_ctx_t *cmp, scBufCtx_t *bc, const scFieldDesc_t *fd, uint8_t idx) {
  if (fd->arraySize > 0 && idx >= fd->arraySize) return false;
  void *ptr = scGetElemPtr(fd, idx);

  switch (fd->type) {
    case SC_U8:    return cmp_write_uinteger(cmp, *(uint8_t*)ptr);
    case SC_U16:   return cmp_write_uinteger(cmp, *(uint16_t*)ptr);
    case SC_U32:   return cmp_write_uinteger(cmp, *(uint32_t*)ptr);
    case SC_I16:   return cmp_write_integer(cmp, *(int16_t*)ptr);
    case SC_FLOAT: return cmp_write_float(cmp, *(float*)ptr);
    case SC_STR: {
      uint8_t l = (uint8_t)strnlen((const char*)ptr, fd->maxLen);
      return cmp_write_str(cmp, (const char*)ptr, l);
    }
    case SC_STRUCT:
      return fd->encode(cmp, bc, ptr);
  }
  return false;
}

static bool scDecodeField(cmp_ctx_t *cmp, const scFieldDesc_t *fd, uint8_t idx) {
  if (fd->arraySize > 0 && idx >= fd->arraySize) return false;
  void *ptr = scGetElemPtr(fd, idx);

  switch (fd->type) {
    case SC_U8:  { uint8_t v;  if (!cmp_read_uchar(cmp, &v))   return false; *(uint8_t*)ptr = v;  return true; }
    case SC_U16: { uint16_t v; if (!cmp_read_ushort(cmp, &v))  return false; *(uint16_t*)ptr = v; return true; }
    case SC_U32: { uint32_t v; if (!cmp_read_uint(cmp, &v))    return false; *(uint32_t*)ptr = v; return true; }
    case SC_I16: { int16_t v;  if (!cmp_read_short(cmp, &v))   return false; *(int16_t*)ptr = v;  return true; }
    case SC_FLOAT: { float v;  if (!cmp_read_float(cmp, &v))   return false; *(float*)ptr = v;    return true; }
    case SC_STR: {
      memset(ptr, 0, fd->maxLen);
      uint32_t sl;
      if (!cmp_read_str_size(cmp, &sl)) return false;
      uint8_t cl = (sl < fd->maxLen) ? (uint8_t)sl : (uint8_t)(fd->maxLen - 1);
      if (sl > 0 && !scBufReader(cmp, ptr, sl)) return false;
      ((char*)ptr)[cl] = '\0';
      return true;
    }
    case SC_STRUCT:
      return fd->decode(cmp, ptr);
  }
  return false;
}

/* ========================================================================= */
/*  CRC16 and frame I/O                                                      */
/* ========================================================================= */

static uint16_t scCalcFrameCrc(uint8_t cmd, const uint8_t *payload, uint16_t len) {
  uint16_t crc = 0xFFFF;

  crc ^= (uint16_t)cmd << 8;
  for (uint8_t j = 0; j < 8; j++) {
    if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
    else              crc <<= 1;
  }
  for (uint16_t i = 0; i < len; i++) {
    crc ^= (uint16_t)payload[i] << 8;
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
      else              crc <<= 1;
    }
  }
  return crc;
}

static void scSendResponse(uint8_t cmd, const uint8_t *payload, uint16_t len) {
  uint8_t header[4], crcBuf[2];
  uint16_t crc;

  header[0] = SC_STX;
  header[1] = cmd;
  header[2] = (uint8_t)(len & 0xFF);
  header[3] = (uint8_t)(len >> 8);
  sdWrite(&SD1, header, 4);

  if (len > 0) {
    sdWrite(&SD1, payload, len);
  }

  crc = scCalcFrameCrc(cmd, payload, len);
  crcBuf[0] = (uint8_t)(crc & 0xFF);
  crcBuf[1] = (uint8_t)(crc >> 8);
  sdWrite(&SD1, crcBuf, 2);
}

/* ========================================================================= */
/*  Thread                                                                   */
/* ========================================================================= */

static uint8_t scRxBuf[SC_BUF_SIZE];
static uint8_t scTxBuf[SC_BUF_SIZE];

static THD_WORKING_AREA(waSerialConfThread, 768);
static THD_FUNCTION(SerialConfThread, arg) {
  chRegSetThreadName(arg);

  bool ok, hasData;
  uint8_t cmd, index, fieldLen;
  uint8_t fieldBuf[32];
  uint16_t payloadLen, rxCrc;
  uint32_t len, mapCount;  
  size_t dataPos;
  cmp_ctx_t cmp;
  scBufCtx_t rxBc;  
  const scFieldDesc_t *fd;

  while (true) {
    /* Wait for STX */    
    dataPos = sdReadTimeout(&SD1, &index, 1, TIME_MS2I(500));
    if (dataPos == 0 || index != SC_STX) continue;

    /* Read CMD */    
    dataPos = sdReadTimeout(&SD1, &cmd, 1, TIME_MS2I(100));
    if (dataPos == 0) continue;

    /* Read LEN (2 bytes LE) */
    
    dataPos = sdReadTimeout(&SD1, fieldBuf, 2, TIME_MS2I(100));
    if (dataPos < 2) continue;
    payloadLen = (uint16_t)fieldBuf[0] | ((uint16_t)fieldBuf[1] << 8);

    if (payloadLen > SC_BUF_SIZE) continue;

    /* Read payload */
    if (payloadLen > 0) {
      dataPos = sdReadTimeout(&SD1, scRxBuf, payloadLen, TIME_MS2I(500));
      if (dataPos < payloadLen) continue;
    }

    /* Read & verify CRC16 */    
    dataPos = sdReadTimeout(&SD1, fieldBuf, 2, TIME_MS2I(100));
    if (dataPos < 2) continue;
    rxCrc = (uint16_t)fieldBuf[0] | ((uint16_t)fieldBuf[1] << 8);
    if (rxCrc != scCalcFrameCrc(cmd, scRxBuf, payloadLen)) {
      DBG_SC("SC: CRC mismatch\r\n");
      continue;
    }

    /* Decode MsgPack envelope: { "f": fieldName, "i": index, "d": data } */
    scCmpInit(&cmp, &rxBc, scRxBuf, payloadLen);
    if (!cmp_read_map(&cmp, &mapCount) || mapCount < 1 || mapCount > 3) continue;

    fieldLen = 0;
    index = 0;
    hasData = false;
    dataPos = 0;

    for (uint8_t m = 0; m < mapCount; m++) {
      /* Read key */
      if (!cmp_read_str_size(&cmp, &len)) break;
      if (len > sizeof(fieldBuf) || !scBufReader(&cmp, fieldBuf, len)) break;

      if (len == 1 && fieldBuf[0] == 'f') {
        if (!cmp_read_str_size(&cmp, &len) || len > sizeof(fieldBuf)) break;
        if (!scBufReader(&cmp, fieldBuf, len)) break;
        fieldLen = (uint8_t)len;
      } else if (len == 1 && fieldBuf[0] == 'i') {
        if (!cmp_read_uchar(&cmp, &index)) break;
      } else if (len == 1 && fieldBuf[0] == 'd') {
        hasData = true;
        dataPos = rxBc.pos;
        cmp_skip_object_no_limit(&cmp);
      } else {
        cmp_skip_object_no_limit(&cmp);
      }
    }

    if (fieldLen == 0) continue;

    /* Lookup field in table */
    fd = scFindField(fieldBuf, fieldLen);

    DBG_SC("SC: cmd=%u field=%.*s idx=%u found=%d\r\n", cmd, fieldLen, fieldBuf, index, fd != NULL);

    /* Handle commands */
    ok = false;

    switch (cmd) {
      case SC_CMD_READ:
        scCmpInit(&cmp, &rxBc, scTxBuf, SC_BUF_SIZE);

        cmp_write_map(&cmp, 3);
        cmp_write_str(&cmp, "f", 1);
        cmp_write_str(&cmp, fieldBuf, fieldLen);
        cmp_write_str(&cmp, "i", 1);
        cmp_write_uinteger(&cmp, index);
        cmp_write_str(&cmp, "d", 1);

        ok = (fd != NULL) && scEncodeField(&cmp, &rxBc, fd, index);

        if (ok && cmp.error == 0) {
          scSendResponse(SC_CMD_READ_RSP, scTxBuf, (uint16_t)SC_CMP_POS(&rxBc));
        } else {
          /* Error response */
          scCmpInit(&cmp, &rxBc, scTxBuf, SC_BUF_SIZE);
          cmp_write_map(&cmp, 3);
          cmp_write_str(&cmp, "f", 1);
          cmp_write_str(&cmp, fieldBuf, fieldLen);
          cmp_write_str(&cmp, "i", 1);
          cmp_write_uinteger(&cmp, index);
          cmp_write_str(&cmp, "s", 1);
          cmp_write_uinteger(&cmp, 0); /* Error status = 0 (false) */
          scSendResponse(SC_CMD_WRITE_RSP, scTxBuf, (uint16_t)SC_CMP_POS(&rxBc));
        }
        break;

      case SC_CMD_WRITE:
        if (hasData && fd != NULL) {
          scCmpInit(&cmp, &rxBc, scRxBuf + dataPos, payloadLen - dataPos);
          ok = scDecodeField(&cmp, fd, index);
          if (ok) {
            writeToBkpSRAM((uint8_t*)&conf, sizeof(config_t), 0);
          }
        }

        scCmpInit(&cmp, &rxBc, scTxBuf, SC_BUF_SIZE);
        cmp_write_map(&cmp, 3);
        cmp_write_str(&cmp, "f", 1);
        cmp_write_str(&cmp, fieldBuf, fieldLen);
        cmp_write_str(&cmp, "i", 1);
        cmp_write_uinteger(&cmp, index);
        cmp_write_str(&cmp, "s", 1);
        cmp_write_uinteger(&cmp, ok ? 1 : 0); /* 1=OK, 0=Error */
        scSendResponse(SC_CMD_WRITE_RSP, scTxBuf, (uint16_t)SC_CMP_POS(&rxBc));
        break;

      default:
        DBG_SC("SC: Unknown cmd %u\r\n", cmd);
        break;
    }
  }
}

#endif /* OHS_TH_SERIAL_CONF_H_ */
