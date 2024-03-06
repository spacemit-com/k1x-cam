/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _H_CAM_SSTR
#define _H_CAM_SSTR

//
// CERR - printf error
// EXIT - printf error, and exit
// IF   - Auxiliary macro for conditional exit
//

#define CERR(fmt, ...) \
    fprintf(stderr, "[%s:%d]" fmt "\n", __func__, __LINE__, errno, strerror(errno), ##__VA_ARGS__)

#define EXIT(fmt, ...)            \
    do {                          \
        exit(EXIT_FAILURE);       \
    } while (0)

#define IF(cond) \
    if ((cond))  \
    EXIT(#cond)

//
// RETURN -  printf error, and return result
// val      : return value
// fmt      : string with format
// ...      : Corresponding parameters of fmt
// return   : val
//
//CERR(fmt, ##__VA_ARGS__);
#define RETURN(val, fmt, ...)     \
    do {                          \
        return val;               \
    } while (0)

#define NIL
#define RETNIL(fmt, ...) RETURN(NIL, fmt, ##__VA_ARGS__)

#define RETNUL(fmt, ...) RETURN(NULL, fmt, ##__VA_ARGS__)
/*
 * test macro in debug mode
 *
 * usage :
 * DEBUG_CODE({
 *      puts("debug start...");s
 * });
 */
#ifndef DEBUG_CODE
#ifdef _DEBUG
#define DEBUG_CODE(code) \
    do code while (0)
#else
#define DEBUG_CODE(code)
#endif  //  ! _DEBUG
#endif  //  ! DEBUG_CODE

#ifndef _ENUM_FLAG
//
// int - return value
// >= 0  Success, < 0  Error
//
enum flag {
    ErrParse = -8,  // parse error
    ErrClose = -7,  // close error
    ErrEmpty = -6,  // empty error
    ErrTout = -5,   // timeout error
    ErrFd = -4,     // open file error
    ErrAlloc = -3,  // alloc error
    ErrParam = -2,  // parameters error
    ErrBase = -1,   // error base tye

    SufBase = +0,  // Success
};

//
// Define some general function pointer, mainly used for base library
//
// cmp_f   - int icmp(const void * ln, const void * rn);
// each_f   - int <-> int, node is internal node
// start_f  - pthread lunch auxiliary macro
//
typedef int (*cmp_f)();
typedef void* (*vnew_f)();
typedef void (*node_f)(void* node);
typedef int (*each_f)(void* node, void* arg);
typedef void* (*start_f)(void* arg);

#define _ENUM_FLAG
#endif  // !_ENUM_FLAG

#ifndef _STRUCT_SSTR

struct sstr {
    char* str;   //
    size_t len;  //
    size_t cap;  //
};

//
typedef struct sstr* sstr_t;

#define _STRUCT_SSTR
#endif

//
#define SSTR_CREATE(var) struct sstr var[1] = {{NULL}}
#define SSTR_DELETE(var) free((var)->str)

//----------------------------------- string process API -------------------------------

//
// Brian Kernighan and Dennis Ritchie simple and fast hash alogic
// str        :string
// return    :hash value after calculate
//
extern unsigned sstr_hash(const char* str);

//
// string compare, case is not insensitive
// ls        : left string
// rs        : right string
// return    : ls > rs return > 0; ... < 0; ... =0
//
extern int sstr_icmp(const char* ls, const char* rs);

//
// string copy, need free
// str        : string to be copy
// return    : string after copy
//
extern char* sstr_dup(const char* str);

//
// file read ,read content of given path,return NULL if failed
// path        : file path
// return    : a string with format tsrt, return NULL if failed
//
extern sstr_t sstr_freadend(const char* path);

//
// write string to file
// path        : file path
// str        : string
// return    : SufBase | ErrParam | ErrFd
//
extern int sstr_fwrites(const char* path, const char* str);

//
// append string to file
// path        : file path
// str        : string
// return    : SufBase | ErrParam | ErrFd
//
extern int sstr_fappends(const char* path, const char* str);

//-----------------------------------string core API -------------------------------

//
// sstr_t create a new sstr_t
// str        : string
// len        : len to be create
// return    : return sstr_t string
//
extern sstr_t sstr_create(const char* str, size_t len);
extern sstr_t sstr_creates(const char* str);

//
// sstr_t free
// sstr        : sstr to be free
//
extern void sstr_delete(sstr_t sstr);

//
// sstr_expand - expand sstr, this is a low api
// sstr        : sstr to be expand
// len        : len to be expand
// return    : sstr->str + sstr->len
//
char* sstr_expand(sstr_t sstr, size_t len);

//
// append char/string for a sstr_t
// c        : char to be add
// str        : string to be add
// sz        : size to be add
//
extern void sstr_appendc(sstr_t sstr, int c);
extern void sstr_appends(sstr_t sstr, const char* str);
extern void sstr_appendn(sstr_t sstr, const char* str, size_t sz);

//
// sstr_popup - pop a sstr
// sstr        : sstr to be pop
// len        : len to be pop
// return    : void
//
extern void sstr_popup(sstr_t sstr, size_t len);

//
// get a string , need free
// sstr        : sstr_t string
// return    : string
//
extern char* sstr_dupstr(sstr_t sstr);

//
// get a string
// sstr        : sstr_t sring
// return    : string, memory address sstr->str
//
extern char* sstr_cstr(sstr_t sstr);

#endif  // !_H_CAM_SSTR
