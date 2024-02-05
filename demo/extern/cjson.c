/*
 * Copyright (C) 2019 ASR Micro Limited
 * All Rights Reserved.
 */
#include "cjson.h"

//
// cjson_delete -  Delete a json entity and all subentities.
// c        : json_t string to be free
// return    : void
//
void cjson_delete(cjson_t c)
{
    while (c) {
        cjson_t next = c->next;
        // discard ref and const improve
        free(c->key);
        if (c->type & CJSON_STRING)
            free(c->vs);
        // recursive del child node
        if (c->child)
            cjson_delete(c->child);
        free(c);
        c = next;
    }
}

// create a new cjson object
static inline cjson_t _cjson_new(void)
{
    cjson_t node = malloc(sizeof(struct cjson));
    if (NULL == node)
        EXIT("malloc struct cjson is null!");
    return memset(node, 0, sizeof(struct cjson));
}

// parse 4 digit hexadecimal number
static unsigned _parse_hex4(const char str[])
{
    unsigned c, h = 0, i = 0;
    // hexadecimal convert
    for (;;) {
        c = *str;
        if (c >= '0' && c <= '9')
            h += c - '0';
        else if (c >= 'A' && c <= 'F')
            h += 10 + c - 'A';
        else if (c >= 'a' && c <= 'z')
            h += 10 + c - 'a';
        else
            return 0;
        // shift left to make place for the next nibble
        if (4 == ++i)
            break;
        h <<= 4;
        ++str;
    }

    return h;
}

// parse string sub function
static const char *_parse_string(cjson_t item, const char *str)
{
    static unsigned char _marks[] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
    const char *ptr;
    char c, *nptr, *out;
    unsigned len = 1, uc, nuc;

    // check if it is string , and record size
    if (*str != '\"')
        RETURN(NULL, "need \\\" str => %s error!", str);
    for (ptr = str + 1; (c = *ptr++) != '\"' && c; ++len)
        if (c == '\\') {
            // skip if  escape character \0
            if (*ptr == '\0')
                RETURN(NULL, "ptr is end len = %d.", len);
            ++ptr;
        }
    if (c != '\"')
        RETURN(NULL, "need string \\\" end there c = %d, %c!", c, c);

    // copy
    if (!(nptr = out = malloc(len)))
        EXIT("calloc size = %d is error!", len);
    for (ptr = str + 1; (c = *ptr) != '\"' && c; ++ptr) {
        if (c != '\\') {
            *nptr++ = c;
            continue;
        }
        // process escape character
        switch ((c = *++ptr)) {
            case 'b':
                *nptr++ = '\b';
                break;
            case 'f':
                *nptr++ = '\f';
                break;
            case 'n':
                *nptr++ = '\n';
                break;
            case 'r':
                *nptr++ = '\r';
                break;
            case 't':
                *nptr++ = '\t';
                break;
            case 'u':  // utf16 => utf8,
                uc = _parse_hex4(ptr + 1);
                ptr += 4;  // skip 4 character, unicode
                if (0 == uc || (uc >= 0xDC00 && uc <= 0xDFFF))
                    break; /* check for invalid. */

                if (uc >= 0xD800 && uc <= 0xDBFF) { /* UTF16 surrogate pairs. */
                    if (ptr[1] != '\\' || ptr[2] != 'u')
                        break; /* missing second-half of surrogate. */
                    nuc = _parse_hex4(ptr + 3);
                    ptr += 6;
                    if (nuc < 0xDC00 || nuc > 0xDFFF)
                        break; /* invalid second-half of surrogate.    */
                    uc = 0x10000 + (((uc & 0x3FF) << 10) | (nuc & 0x3FF));
                }

                if (uc < 0x80)
                    len = 1;
                else if (uc < 0x800)
                    len = 2;
                else if (uc < 0x10000)
                    len = 3;
                else
                    len = 4;
                nptr += len;

                switch (len) {
                    case 4:
                        *--nptr = ((uc | 0x80) & 0xBF);
                        uc >>= 6;
                    case 3:
                        *--nptr = ((uc | 0x80) & 0xBF);
                        uc >>= 6;
                    case 2:
                        *--nptr = ((uc | 0x80) & 0xBF);
                        uc >>= 6;
                    case 1:
                        *--nptr = (uc | _marks[len]);
                }
                nptr += len;
                break;
            default:
                *nptr++ = c;
        }
    }
    *nptr = '\0';
    item->vs = out;
    item->type = CJSON_STRING;
    return ++ptr;
}

// parse number sub function
static const char *_parse_number(cjson_t item, const char *str)
{
    double n = .0, ns = 1.0, nd = .0;  // ns is positive or negative, negative is - 1, nd is the number of after decimal
    int e = 0, es = 1;                 // e is pow, es is positive or negative, negative is - 1
    char c;

    if ((c = *str) == '-' || c == '+') {
        ns = c == '-' ? -1.0 : 1.0;  // positive or negative
        ++str;
    }
    // process int
    for (c = *str; c >= '0' && c <= '9'; c = *++str) n = n * 10 + c - '0';
    if (c == '.')
        for (; (c = *++str) >= '0' && c <= '9'; --nd) n = n * 10 + c - '0';

    // process pow
    if (c == 'e' || c == 'E') {
        if ((c = *++str) == '+')  // process pow
            ++str;
        else if (c == '-')
            es = -1, ++str;
        for (; (c = *str) >= '0' && c <= '9'; ++str) e = e * 10 + c - '0';
    }

    // return reslut number = +/- number.fraction * 10^+/- exponent
    item->vd = ns * n * pow(10.0, nd + es * e);
    item->type = CJSON_NUMBER;
    return str;
}

// Retrieve descend
static const char *_parse_value(cjson_t item, const char *str);
static const char *_parse_array(cjson_t item, const char *str);
static const char *_parse_object(cjson_t item, const char *str);

// parse array sub function
static const char *_parse_array(cjson_t item, const char *str)
{
    cjson_t child;

    if (*str != '[') {
        RETURN(NULL, "array str error start: %s.", str);
    }

    item->type = CJSON_ARRAY;
    if (*++str == ']')  // skip if ']'
        return str + 1;

    item->child = child = _cjson_new();
    str = _parse_value(child, str);
    if (NULL == str) {
        RETURN(NULL, "array str error e n d one: %s.", str);
    }

    while (*str == ',') {
        // process if more ',' in line end
        if (str[1] == ']')
            return str + 2;

        //
        child->next = _cjson_new();
        child = child->next;
        str = _parse_value(child, str + 1);
        if (NULL == str) {
            RETURN(NULL, "array str error e n d two: %s.", str);
        }
    }

    if (*str != ']') {
        RETURN(NULL, "array str error e n d: %s.", str);
    }
    return str + 1;
}

// parse object sub function
static const char *_parse_object(cjson_t item, const char *str)
{
    cjson_t child;

    if (*str != '{') {
        RETURN(NULL, "object str error start: %s.", str);
    }

    item->type = CJSON_OBJECT;
    if (*++str == '}')
        return str + 1;

    // process child node, read a key
    item->child = child = _cjson_new();
    str = _parse_string(child, str);
    if (!str || *str != ':') {
        RETURN(NULL, "_parse_string is error : %s!", str);
    }
    child->key = child->vs;

    child->vs = NULL;
    str = _parse_value(child, str + 1);
    if (!str) {
        RETURN(NULL, "_parse_value is error 2!");
    }

    // Retrieve parse
    while (*str == ',') {
        // process if more ',' in line end
        if (str[1] == '}')
            return str + 2;

        child->next = _cjson_new();
        child = child->next;
        str = _parse_string(child, str + 1);
        if (!str || *str != ':') {
            RETURN(NULL, "_parse_string need name or no equal ':' %s.", str);
        }
        child->key = child->vs;

        child->vs = NULL;
        str = _parse_value(child, str + 1);
        if (!str) {
            RETURN(NULL, "_parse_value need item two ':' %s.", str);
        }
    }

    if (*str != '}') {
        RETURN(NULL, "object str error e n d: %s.", str);
    }
    return str + 1;
}

// parse value and push to json
static const char *_parse_value(cjson_t item, const char *str)
{
    char c = '\0';
    if ((str) && (c = *str)) {
        switch (c) {
            // n = null, f = false, t = true ...
            case 'n':
                return item->type = CJSON_NULL, str + 4;
            case 'f':
                return item->type = CJSON_FALSE, str + 5;
            case 't':
                return item->type = CJSON_TRUE, item->vd = 1.0, str + 4;
            case '\"':
                return _parse_string(item, str);
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '+':
            case '-':
            case '.':
                return _parse_number(item, str);
            case '[':
                return _parse_array(item, str);
            case '{':
                return _parse_object(item, str);
        }
    }
    // will unexpect value if loop go to here
    RETURN(NULL, "params value = %c, %s!", c, str);
}

//  del unnecessary string in jstr
static size_t _cjson_mini(char *jstr)
{
    char c, *in = jstr, *to = jstr;

    while ((c = *to)) {
        // step 1 : process string
        if (c == '"') {
            *in++ = c;
            while ((c = *++to) && (c != '"' || to[-1] == '\\')) *in++ = c;
            if (c) {
                *in++ = c;
                ++to;
            }
            continue;
        }
        // step 2 : some special char
        if (c < '!') {
            ++to;
            continue;
        }
        if (c == '/') {
            // step 3 : process //  means line end
            if (to[1] == '/') {
                while ((c = *++to) && c != '\n')
                    ;
                continue;
            }

            // step 4 : process /*
            if (to[1] == '*') {
                while ((c = *++to) && (c != '*' || to[1] != '/'))
                    ;
                if (c)
                    to += 2;
                continue;
            }
        }
        // step 5 : save data
        *in++ = *to++;
    }

    *in = '\0';
    return in - jstr;
}

//
static cjson_t _cjson_parse(const char *jstr)
{
    const char *end;
    cjson_t json = _cjson_new();

    if (!(end = _parse_value(json, jstr))) {
        cjson_delete(json);
        RETURN(NULL, "_parse_value params end = %s!", end);
    }

    return json;
}

//
// cjson_newxxx -  convert source str/sstr/patch to a new json struct
// str        : string, format is char*
// sstr        : string, format is  sstr_t
// path        : json file path
// return    : new json_t struct, and will be NULL if failed
//
inline cjson_t cjson_new_tokener_str(const char *str)
{
    cjson_t json;
    SSTR_CREATE(sstr);
    sstr_appends(sstr, str);

    _cjson_mini(sstr->str);
    json = _cjson_parse(sstr->str);

    SSTR_DELETE(sstr);
    return json;
}

inline cjson_t cjson_newsstr(sstr_t sstr)
{
    sstr->len = _cjson_mini(sstr->str);
    return _cjson_parse(sstr->str);
}

cjson_t cjson_new_file(const char *path)
{
    cjson_t json;
    sstr_t sstr = sstr_freadend(path);
    if (!sstr) {
        RETURN(NULL, "sstr_freadend path:%s is error!", path);
    }

    json = cjson_newsstr(sstr);
    sstr_delete(sstr);
    return json;
}

//
// cjson_get_len -  Retrieve item number "item" from array "array".
// array    : cjson_t array to be get
// return    : item number "item" from array "array",Returns NULL if unsuccessful.
//
size_t cjson_get_len(cjson_t array)
{
    size_t len = 0;
    if (array) {
        for (array = array->child; array; array = array->next) ++len;
    }
    return len;
}

//
// cjson_getxxx -  Get Array item / object item. *
// array    : json array item
// idx        : index of array
// object    : json object
// key        : key name value
// return    : json item
//
cjson_t cjson_get_array(cjson_t array, size_t idx)
{
    cjson_t c;

    for (c = array->child; c && idx > 0; c = c->next) --idx;

    return c;
}

cjson_t cjson_get_object(cjson_t object, const char *key)
{
    cjson_t c;
    DEBUG_CODE({
        if (!object || !key || !*key) {
            RETURN(NULL, "object:%p, key=%s params is error!", object, key);
        }
    });

    for (c = object->child; c && sstr_icmp(key, c->key); c = c->next)
        ;

    return c;
}

// ---------------------------------  cjson output -----------------------------------------

// item convert to string and save in p
static char *_print_number(cjson_t item, sstr_t p)
{
    char *str = NULL;
    double d = item->vd;
    int i = (int)d;

    if (d == 0) {  // 0
        str = sstr_expand(p, 2);
        str[0] = '0', str[1] = '\0';
    } else if ((fabs(d - i)) <= DBL_EPSILON && d <= INT_MAX && d >= INT_MIN) {
        str = sstr_expand(p, 21);  // int
        sprintf(str, "%d", i);
    } else {
        // positive then compare
        double nd = fabs(d);
        str = sstr_expand(p, 64);
        if (fabs(floor(d) - d) <= DBL_EPSILON && nd < 1.0e60)
            sprintf(str, "%.0f", d);
        else if (nd < 1.0e-6 || nd > 1.0e9)  // entific notation
            sprintf(str, "%e", d);
        else
            sprintf(str, "%f", d);
    }

    return str;
}

// print string
static char *_print_string(char *str, sstr_t p)
{
    size_t len = 0;
    unsigned char c;
    const char *ptr;
    char *nptr, *out;

    if (!str || !*str) {  // none  return NULL
        out = sstr_expand(p, 3);
        out[0] = '\"', out[1] = '\"', out[2] = '\0';
        return out;
    }

    // process  " and  escape character
    for (ptr = str; (c = *ptr) && ++len; ++ptr) {
        if (strchr("\"\\\b\f\n\r\t", c))
            ++len;
        else if (c < 32)  // hide character
            len += 5;
    }

    // expand memory, and add \"
    nptr = out = sstr_expand(p, len + 3);
    //    *nptr++ = '\"';

    // no special character
    if (len == (size_t)(ptr - str)) {
        strcpy(nptr, str);
        //        nptr[len] = '\"';
        nptr[len + 1] = '\0';
        return out;
    }

    for (ptr = str; (c = *ptr); ++ptr) {
        if (c > 31 && c != '\"' && c != '\\') {
            *nptr++ = c;
            continue;
        }
        *nptr++ = '\\';
        switch (c) {
            case '\b':
                *nptr++ = 'b';
                break;
            case '\f':
                *nptr++ = 'f';
                break;
            case '\n':
                *nptr++ = 'n';
                break;
            case '\r':
                *nptr++ = 'r';
                break;
            case '\t':
                *nptr++ = 't';
                break;
            case '\\':
            case '\"':
                *nptr++ = c;
                break;
            // hide character use 4 byte
            default:
                sprintf(nptr, "u%04x", c);
                nptr += 5;
        }
    }

    //    *nptr++ = '\"';
    *nptr = '\0';
    return out;
}

// Retrieve descend, value, array, object
static char *_print_value(cjson_t item, sstr_t p);
static char *_print_array(cjson_t item, sstr_t p);
static char *_print_object(cjson_t item, sstr_t p);

//_print_value function define: private function regard  item and p is exist
static char *_print_value(cjson_t item, sstr_t p)
{
    char *out = NULL;
    switch (item->type) {
        case CJSON_FALSE:
            out = sstr_expand(p, 6);
            strcpy(out, "false");
            break;
        case CJSON_TRUE:
            out = sstr_expand(p, 5);
            strcpy(out, "true");
            break;
        case CJSON_NULL:
            out = sstr_expand(p, 5);
            strcpy(out, "null");
            break;
        case CJSON_NUMBER:
            out = _print_number(item, p);
            break;
        case CJSON_STRING:
            out = _print_string(item->vs, p);
            break;
        case CJSON_ARRAY:
            out = _print_array(item, p);
            break;
        case CJSON_OBJECT:
            out = _print_object(item, p);
            break;
    }
    p->len += strlen(p->str + p->len);

    return out;
}

// _print_array function define: private function regard  item and p is exist
static char *_print_array(cjson_t item, sstr_t p)
{
    size_t i;
    char *ptr;
    cjson_t child;

    i = p->len;
    ptr = sstr_expand(p, 1);
    *ptr = '[';
    ++p->len;

    for (child = item->child; (child); child = child->next) {
        _print_value(child, p);
        if (child->next) {
            ptr = sstr_expand(p, 2);
            *ptr++ = ',';
            *ptr = '\0';
            p->len += 1;
        }
    }

    ptr = sstr_expand(p, 2);
    *ptr++ = ']';
    *ptr = '\0';
    return p->str + i;
}

// _print_object function define: private function regard  item and p is exist
static char *_print_object(cjson_t item, sstr_t p)
{
    size_t i;
    char *ptr;
    cjson_t child;

    i = p->len;
    ptr = sstr_expand(p, 1);
    *ptr++ = '{';
    ++p->len;

    // child process
    for (child = item->child; (child); child = child->next) {
        _print_string(child->key, p);
        p->len += strlen(p->str + p->len);

        // add :
        ptr = sstr_expand(p, 1);
        *ptr++ = ':';
        p->len += 1;

        // continue
        _print_value(child, p);

        // end
        if (child->next) {
            ptr = sstr_expand(p, 2);
            *ptr++ = ',';
            *ptr = '\0';
            p->len += 1;
        }
    }

    ptr = sstr_expand(p, 2);
    *ptr++ = '}';
    *ptr = '\0';
    return p->str + i;
}

//
// cjson_dup_str-  Get item "string" from object. Case insensitive
// json        : json item
// return    : item "string" from object, need free
//
char *cjson_dup_str(cjson_t json)
{
    SSTR_CREATE(sstr);
    if (NULL == _print_value(json, sstr)) {
        SSTR_DELETE(sstr);
        RETURN(NULL, "_print_value json is error :%p !", json);
    }
    return realloc(sstr->str, sstr->len + 1);
}

//
// cjson_get_str -  Get item "string" from object. Case insensitive
// json        : json item
// return    : item "string" from object,
//
char *cjson_get_str(cjson_t json)
{
    if (!json)
        return NULL;
    switch (json->type) {
        case CJSON_STRING:
            return json->vs;
        default:
            return NULL;
    }
}

sstr_t cjson_getsstr(cjson_t json)
{
    sstr_t sstr = sstr_creates(NULL);
    if (NULL == _print_value(json, sstr)) {
        sstr_delete(sstr);
        RETURN(NULL, "_print_value json is error :%p !", json);
    }
    return sstr;
}

//
// cjson_is_type -  check cjson type
// json        : json item
// return    : true mens matched
//
bool cjson_is_type(struct cjson *const item, unsigned char type)
{
    if (item == NULL) {
        return false;
    }

    return (item->type & 0xFF) == type;
}

// ---------------------------------  cjson construct function -----------------------------------------

static inline cjson_t _cjson_newt(unsigned char type)
{
    cjson_t item = _cjson_new();
    item->type = type;
    return item;
}

//
// cjson_newxxx -  These calls create a json item of the appropriate type.
// b        : bool value
// vd        : double value
// vs        : string value
// return    : return a json item
//
inline cjson_t cjson_new_null(void)
{
    return _cjson_new();
}

inline cjson_t cjson_new_array(void)
{
    return _cjson_newt(CJSON_ARRAY);
}

inline cjson_t cjson_new_object(void)
{
    return _cjson_newt(CJSON_OBJECT);
}

inline cjson_t cjson_new_bool(bool b)
{
    cjson_t item = _cjson_newt(1u << (1 + b));
    item->vd = b;
    return item;
}

inline cjson_t cjson_new_number(double vd)
{
    cjson_t item = _cjson_newt(CJSON_NUMBER);
    item->vd = vd;
    return item;
}

inline cjson_t cjson_new_tokener_string(const char *vs)
{
    cjson_t item = _cjson_newt(CJSON_STRING);
    item->vs = sstr_dup(vs);
    return item;
}

//
// cjson_newtype_array - create an Array of count items
// type        : type micro
// array    : array to be create
// len        : array len
// return    : return a json item
//
extern cjson_t cjson_newtype_array(unsigned char type, const void *array, size_t len)
{
    size_t i;
    cjson_t n = NULL, p = NULL, a = NULL;

    for (i = 0; i < len; ++i) {
        switch (type) {
            case CJSON_NULL:
                n = cjson_new_null();
                break;
            case CJSON_TRUE:
                n = cjson_new_bool(array ? ((bool *)array)[i] : true);
                break;
            case CJSON_FALSE:
                n = cjson_new_bool(array ? ((bool *)array)[i] : false);
                break;
            case CJSON_NUMBER:
                n = cjson_new_number(((double *)array)[i]);
                break;
            case CJSON_STRING:
                n = cjson_new_tokener_string(((char **)array)[i]);
                break;
            default:
                RETURN(NULL, "type is error = %d.", type);
        }
        if (i)  // if already exist
            p->next = n;
        else {
            a = cjson_new_array();
            a->child = n;
        }
        p = n;
    }
    return a;
}

//
/* Add item to array/object. */
static void suffix_object(struct cjson *prev, struct cjson *item)
{
    prev->next = item;
    item->prev = prev;
}

static char *cjson_strdup(const char *str)
{
    size_t len;
    char *copy;

    len = strlen(str) + 1;
    if (!(copy = (char *)malloc(len)))
        return 0;
    memcpy(copy, str, len);
    return copy;
}

void cjson_add_item_to_array(struct cjson *array, struct cjson *item)
{
    struct cjson *c = array->child;
    if (!item)
        return;
    if (!c) {
        array->child = item;
    } else {
        while (c && c->next) c = c->next;
        suffix_object(c, item);
    }
}

int cjson_add_item_to_object(struct cjson *object, const char *string, struct cjson *item)
{
    if (!item)
        return -1;
    if (item->vs)
        free(item->vs);
    item->vs = cjson_strdup(string);
    cjson_add_item_to_array(object, item);
    return 0;
}

void cjson_set_int(struct cjson *item, int number)
{
    if (!item)
        return;
    item->vd = number;
}

void cjson_set_str(struct cjson *item, const char *string)
{
    if (!item)
        return;
    if (item->vs)
        free(item->vs);
    item->vs = cjson_strdup(string);
}

//
// cjson_detach_array -  Remove/Detatch items from Arrays
// array    : array to be detatch
// idx        : index of array
// return    : return json_t item after attached
//
cjson_t cjson_detach_array(cjson_t array, size_t idx)
{
    cjson_t c, p;
    if ((!array) || !(c = array->child))
        RETURN(NULL, "check param is array:%p, idx:%zu is error!", array, idx);

    // find
    for (p = NULL; idx > 0 && c; c = c->next) {
        --idx;
        p = c;
    }
    if (NULL == c) {
        RETURN(NULL, "check param is too dig idx:sub %zu.", idx);
    }

    if (NULL == p)
        array->child = c->next;
    else
        p->next = c->next;
    c->next = NULL;
    return c;
}

//
// cjson_detach_object -  Remove/Detatch items fromObjects
// object    : object to be detatch
// key        : key name value
// return    : return json_t item after attached
//
cjson_t cjson_detach_object(cjson_t object, const char *key)
{
    cjson_t c, p;
    if ((!object) || !(c = object->child) || !key || !*key) {
        RETURN(NULL, "check param is object:%p, key:%s.", object, key);
    }

    // find
    for (p = NULL; c && sstr_icmp(c->key, key); c = c->next) {
        p = c;
    }
    if (NULL == c) {
        RETURN(NULL, "check param key:%s to empty!", key);
    }

    //
    if (NULL == p)
        object->child = c->next;
    else
        p->next = c->next;
    c->next = NULL;
    return c;
}
