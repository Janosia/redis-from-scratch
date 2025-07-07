#pragma once 

#include <iostream>
#include <vector> 
#include <unistd.h>
#include <cassert>

#include "hashtable.h"
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
static bool equality(HNode *lhs, HNode *rhs){
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key==re->key;
}

static bool entry_eq(HNode *lhs, HNode *rhs) {
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key == re->key;
}

static void out_nil(Buffer &out){
    buf_append_u8(out, TAG_NIL);
}
static void out_str(Buffer &out, const char *s, size_t size){
    buf_append_u8(out, TAG_STR);
    buf_append_u32(out, (uint32_t)size);
    buf_append(out, (const uint8_t *)s, size);
}

static void out_int(Buffer &out, int64_t val) {
    buf_append_u8(out, TAG_INT);
    buf_append_i64(out, val);
}
static void out_arr(Buffer &out, uint32_t n) {
    buf_append_u8(out, TAG_ARR);
    buf_append_u32(out, n);
}
static void out_dbl(Buffer &out, double val) {
    buf_append_u8(out, TAG_DBL);
    buf_append_dbl(out, val);
}
static void out_err(Buffer &out, uint32_t code, const string &msg) {
    buf_append_u8(out, TAG_ERR);
    buf_append_u32(out, code);
    buf_append_u32(out, (uint32_t)msg.size());
    buf_append(out, (const uint8_t *)msg.data(), msg.size());
}

static size_t out_begin_arr(Buffer &out) {
    out.push_back(TAG_ARR);
    buf_append_u32(out, 0);     
    return out.size() - 4;     
}
static void out_end_arr(Buffer &out, size_t ctx, uint32_t n) {
    assert(out[ctx - 1] == TAG_ARR);
    memcpy(&out[ctx], &n, 4);
}

static bool str2dbl(const string &s, double &out) {
    char *endp = NULL;
    out = strtod(s.c_str(), &endp);
    return endp == s.c_str() + s.size() && !isnan(out);
}

static bool str2int(const string &s, int64_t &out) {
    char *endp = NULL;
    out = strtoll(s.c_str(), &endp, 10);
    return endp == s.c_str() + s.size();
}