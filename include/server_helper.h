#pragma once 

#include <iostream>
#include <vector> 
#include <unistd.h>
#include <cassert>

#include "hashtable.h"
#include "common.h"
#include "zset.h"

using namespace std;

const size_t k_max_msg = 32 <<20, k_max_args = 200 *1000;
typedef vector<uint8_t>Buffer;
static struct {HMap db;}g_data; // top level

// error codes 
enum ErrorCode {
    ERR_UNKNOWN = -1,
    ERR_TOO_BIG = -2,
    ERR_BAD_TYP = -3,
    ERR_BAD_ARG = -4,
};

// DATA TYPE CODES
enum{
    TAG_NIL = 0,    // nil
    TAG_ERR = 1,    // error code + msg
    TAG_STR = 2,    // string
    TAG_INT = 3,    // int64
    TAG_DBL = 4,    // double
    TAG_ARR = 5,    // array
};

enum{
    T_INIT = 0,
    T_STR = 1, // string
    T_ZSET = 2, // sorted set
};

class Conn {
    public:
        int fd = -1;
        
        // what application wants 
        bool want_read = false;
        bool want_write = false;
        bool want_close = false;

        Buffer incoming;
        Buffer outgoing;
};

class LookupKey{
    public:
        HNode node;
        string key;
};

class Entry {
    public:
        struct HNode node;  
        string key;
        uint32_t type = 0;
        string str;
        Zset zset;
};

// Constructor
static Entry *entry_new(uint32_t type) {
    Entry *ent = new Entry();
    ent->type = type;
    return ent;
}


// Destructor
static void entry_del(Entry *ent) {
    if (ent->type == T_ZSET) {
        zset_clear(&ent->zset);
    }
    delete ent;
}

// comparison function for Entry
static bool equality(HNode *lhs, HNode *rhs);
static bool entry_eq(HNode *lhs, HNode *rhs);

static void out_nil(Buffer &out);
static void out_str(Buffer &out, const char *s, size_t size);
static void out_int(Buffer &out, int64_t val);
static void out_arr(Buffer &out, uint32_t n);
static void out_dbl(Buffer &out, double val);
static void out_err(Buffer &out, uint32_t code, const string &msg);

static size_t out_begin_arr(Buffer &out);
static void out_end_arr(Buffer &out, size_t ctx, uint32_t n);

static bool str2dbl(const string &s, double &out);
static bool str2int(const string &s, int64_t &out);

static void buf_append_u8(Buffer &buf, uint8_t data);
static void buf_append(Buffer &buf, const uint8_t*data, size_t len);
static void buf_append_u32(Buffer &buf, uint32_t data);
static void buf_append_i64(Buffer &buf, int64_t data);
static void buf_append_dbl(Buffer &buf, double data);