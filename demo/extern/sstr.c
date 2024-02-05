/*
 * Copyright (C) 2019 ASR Micro Limited
 * All Rights Reserved.
 */
#include <sstr.h>

//----------------------------------- string process API -------------------------------

//
// Brian Kernighan and Dennis Ritchie simple and fast hash alogic
// str        :string
// return    :hash value after calculate
//
unsigned sstr_hash(const char* str)
{
    register unsigned h = 0;
    if (str) {
        register unsigned c;
        while ((c = *str++)) h = h * 131 + c;
    }
    return h;
}

//
// string compare, case is not insensitive
// ls        : left string
// rs        : right string
// return    : ls > rs return > 0; ... < 0; ... =0
//
int sstr_icmp(const char* ls, const char* rs)
{
    int l, r;
    if (!ls || !rs)
        return (int)(ls - rs);

    do {
        if ((l = *ls++) >= 'a' && l <= 'z')
            l -= 'a' - 'A';
        if ((r = *rs++) >= 'a' && r <= 'z')
            r -= 'a' - 'A';
    } while (l && l == r);

    return l - r;
}

//
// string copy, need free
// str        : string to be copy
// return    : string after copy
//
char* sstr_dup(const char* str)
{
    size_t len;
    char* nstr;
    if (NULL == str)
        return NULL;

    len = strlen(str) + 1;
    nstr = malloc(sizeof(char) * len);
    //
    // if malloc failed.
    // log + exit.
    //
    if (NULL == nstr)
        EXIT("malloc len = %zu is empty!", len);

    return memcpy(nstr, str, len);
}

//
// file read ,read content of given path,return NULL if failed
// path        : file path
// return    : a string with format tsrt, return NULL if failed
//
sstr_t sstr_freadend(const char* path)
{
    int err;
    size_t rn;
    sstr_t sstr;
    char buf[BUFSIZ];
    FILE* txt = fopen(path, "rb");
    if (NULL == txt) {
        RETURN(NULL, "fopen r %s is error!", path);
    }

    // alloc memory
    sstr = sstr_creates(NULL);

    // read file
    do {
        rn = fread(buf, sizeof(char), BUFSIZ, txt);
        if ((err = ferror(txt))) {
            fclose(txt);
            sstr_delete(sstr);
            RETURN(NULL, "fread err path = %d, %s.", err, path);
        }
        // save data
        sstr_appendn(sstr, buf, rn);
    } while (rn == BUFSIZ);

    fclose(txt);

    // continue, add \0 at last line
    sstr_cstr(sstr);

    return sstr;
}

static int _sstr_fwrite(const char* path, const char* str, const char* mode)
{
    FILE* txt;
    if (!path || !*path || !str) {
        RETURN(ErrParam, "check !path || !*path || !str'!!!");
    }

    // open file , write, close file
    if ((txt = fopen(path, mode)) == NULL) {
        RETURN(ErrFd, "fopen mode = '%s', path = '%s' error!", mode, path);
    }
    fputs(str, txt);
    fclose(txt);

    return SufBase;
}

//
// write string to file
// path        : file path
// str        : string
// return    : SufBase | ErrParam | ErrFd
//
inline int sstr_fwrites(const char* path, const char* str)
{
    return _sstr_fwrite(path, str, "wb");
}

//
// append string to file
// path        : file path
// str        : string
// return    : SufBase | ErrParam | ErrFd
//
inline int sstr_fappends(const char* path, const char* str)
{
    return _sstr_fwrite(path, str, "ab");
}

//-----------------------------------string core API -------------------------------

//
// sstr_t create a new sstr_t
// str        : string
// len        : len to be create
// return    : return sstr_t string
//
sstr_t sstr_create(const char* str, size_t len)
{
    sstr_t sstr = calloc(1, sizeof(struct sstr));
    if (NULL == sstr)
        EXIT("malloc sizeof struct sstr is error!");
    if (str && len > 0)
        sstr_appendn(sstr, str, len);
    return sstr;
}

sstr_t sstr_creates(const char* str)
{
    sstr_t sstr = calloc(1, sizeof(struct sstr));
    if (NULL == sstr)
        EXIT("malloc sizeof struct sstr is error!");
    if (str)
        sstr_appends(sstr, str);
    return sstr;
}

//
// sstr_t free
// sstr        : sstr to be free
//
inline void sstr_delete(sstr_t sstr)
{
    free(sstr->str);
    free(sstr);
}

// size of str orig size
#define _UINT_SSTR (32u)

//
// sstr_expand - expand sstr, this is a low api
// sstr        : sstr to be expand
// len        : len to be expand
// return    : sstr->str + sstr->len
//
char* sstr_expand(sstr_t sstr, size_t len)
{
    size_t cap = sstr->cap;
    if ((len += sstr->len) > cap) {
        char* nstr;
        for (cap = cap < _UINT_SSTR ? _UINT_SSTR : cap; cap < len; cap <<= 1)
            ;
        // alloc
        if ((nstr = realloc(sstr->str, cap)) == NULL) {
            sstr_delete(sstr);
            EXIT("realloc cap = %zu empty!!!", cap);
        }

        //
        sstr->str = nstr;
        sstr->cap = cap;
    }

    return sstr->str + sstr->len;
}

//
// append char/string for a sstr_t
// c        : char to be add
// str        : string to be add
// sz        : size to be add
//
inline void sstr_appendc(sstr_t sstr, int c)
{
    // no safe check
    sstr_expand(sstr, 1);
    sstr->str[sstr->len++] = c;
}

void sstr_appends(sstr_t sstr, const char* str)
{
    size_t len;
    if (!sstr || !str) {
        RETURN(NIL, "check '!sstr || !str' param is error!");
    }

    len = strlen(str);
    if (len > 0)
        sstr_appendn(sstr, str, len);
    sstr_cstr(sstr);
}

inline void sstr_appendn(sstr_t sstr, const char* str, size_t sz)
{
    sstr_expand(sstr, sz);
    memcpy(sstr->str + sstr->len, str, sz);
    sstr->len += sz;
}

//
// sstr_popup - pop a sstr
// sstr        : sstr to be pop
// len        : len to be pop
// return    : void
//
inline void sstr_popup(sstr_t sstr, size_t len)
{
    if (len > sstr->len)
        sstr->len = 0;
    else {
        sstr->len -= len;
        memmove(sstr->str, sstr->str + len, sstr->len);
    }
}

//
// get a string , need free
// sstr        : sstr_t string
// return    : string
//
char* sstr_dupstr(sstr_t sstr)
{
    size_t len;
    char* str;
    if (!sstr || sstr->len < 1)
        return NULL;

    // calc size and malloc
    len = sstr->len + !!sstr->str[sstr->len - 1];
    str = malloc(len * sizeof(char));
    if (NULL == str)
        EXIT("malloc len = %zu is error!", len);

    memcpy(str, sstr->str, len - 1);
    str[len - 1] = '\0';

    return str;
}

//
// get a string
// sstr        : sstr_t sring
// return    : string, memory address sstr->str
//
inline char* sstr_cstr(sstr_t sstr)
{
    // chek if last character is '\0'
    if (sstr->len < 1 || sstr->str[sstr->len - 1]) {
        sstr_expand(sstr, 1);
        sstr->str[sstr->len] = '\0';
    }

    return sstr->str;
}
