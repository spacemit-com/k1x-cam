/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _H_CAM_CJSON
#define _H_CAM_CJSON

#include "sstr.h"

/* cjson Types: */
#define CJSON_NULL   (0u << 0)
#define CJSON_FALSE  (1u << 0)
#define CJSON_TRUE   (1u << 1)
#define CJSON_NUMBER (1u << 2)
#define CJSON_STRING (1u << 3)
#define CJSON_ARRAY  (1u << 4)
#define CJSON_OBJECT (1u << 5)

struct cjson {
    struct cjson *next; /* next/prev allow you to walk array/object chains. Alternatively, use
                           GetArraySize/GetArrayItem/GetObjectItem */
    struct cjson *prev;
    struct cjson *child; /* An array or object item will have a child pointer pointing to a chain of the items in the
                            array/object. */
    unsigned char type;  /* The type of the item, as above. */
    char *key;           /*key name of json*/
    union {
        char *vs;  /* The item's string, if type==CJSON_STRING */
        double vd; /* The item's number, if type==CJSON_NUMBER */
    };
};

/* define cjson_t*/
typedef struct cjson *cjson_t;

//
// cjson_getint - get int value of json
// item        : cjson_t node to be get
// return    : int
//
#define cjson_get_int(item) ((int)((item)->vd))

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
//#define cjson_set_int(object,val)            ((object)?(int)((object)->vd)=(val):(val))
extern void cjson_set_int(struct cjson *item, int number);

//
// cjson_set_str - set string value of json
// item        : cjson_t node to be set
// return    : void
//
extern void cjson_set_str(struct cjson *item, const char *string);

//
// cjson_delete -  Delete a json entity and all subentities.
// c        : json_t string to be free
// return    : void
//
extern void cjson_delete(cjson_t c);

//
// cjson_newxxx -  convert source str/sstr/patch to a new json struct
// str        : string, format is char*
// sstr        : string, format is  sstr_t
// path        : json file path
// return    : new json_t struct, and will be NULL if failed
//
extern cjson_t cjson_new_tokener_str(const char *str);
extern cjson_t cjson_newsstr(sstr_t sstr);
extern cjson_t cjson_new_file(const char *path);

//
// cjson_get_len -  Retrieve item number "item" from array "array".
// array    : cjson_t array to be get
// return    : item number "item" from array "array",Returns NULL if unsuccessful.
//
extern size_t cjson_get_len(cjson_t array);

//
// cjson_getxxx -  Get Array item / object item. *
// array    : json array item
// idx        : index of array
// object    : json object
// key        : key name value
// return    : json item
//
extern cjson_t cjson_get_array(cjson_t array, size_t idx);
extern cjson_t cjson_get_object(cjson_t object, const char *key);

// ---------------------------------  cjson output interface -----------------------------------------

//
// cjson_gett?str -  Get item "string" from object. Case insensitive
// json        : json item
// return    : item "string" from object, need free
//
extern char *cjson_get_str(cjson_t json);
extern sstr_t cjson_getsstr(cjson_t json);

// ---------------------------------  cjson construct interface -----------------------------------------

//
// cjson_newxxx - create new json object
// b        : bool
// vd        : double
// vs        : string
// return    : new json object
//
extern cjson_t cjson_new_null(void);
extern cjson_t cjson_new_array(void);
extern cjson_t cjson_new_object(void);
extern cjson_t cjson_new_bool(bool b);
extern cjson_t cjson_new_number(double vd);
extern cjson_t cjson_new_tokener_string(const char *vs);

//
// cjson_newtype_array - create an Array of count items
// type        : type micro
// array    : array to be create
// len        : array len
// return    : return a json item
//
extern cjson_t cjson_newtype_array(unsigned char type, const void *array, size_t len);

//
// cjson_detach_array -  Remove/Detatch items from Arrays
// array    : array to be detatch
// idx        : index of array
// return    : return json_t item after attached
//
extern cjson_t cjson_detach_array(cjson_t array, size_t idx);

//
// cjson_detach_object -  Remove/Detatch items fromObjects
// object    : object to be detatch
// key        : key name value
// return    : return json_t item after attached
//
extern cjson_t cjson_detach_object(cjson_t object, const char *key);

extern int cjson_add_item_to_object(struct cjson *object, const char *string, struct cjson *item);

//
// cjson_is_type -  check cjson type
// json        : json item
// return    : true mens matched
//
bool cjson_is_type(struct cjson *const item, unsigned char type);

#endif  // !_H_CAM_CJSON
