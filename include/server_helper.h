#pragma once 

// C++ Imports
#include <iostream>
#include <vector> 
#include <unistd.h>
#include <cassert>
#include <cstring>

// Project Imports
#include "hashtable.h"
#include "common.h"
#include "zset.h"
#include "dl_list.h"
#include "heap.h"
#include "timer.h"

using namespace std;

const size_t k_max_msg = 32 << 20, k_max_args = 200 *1000;
typedef vector<uint8_t>Buffer;

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

        // timer
        uint64_t last_active_ms =0;
        Dlist idle_node;
};

// global state
struct {
    HMap db;
    vector<Conn *>fd2conn;
    Dlist idle_list;
    vector<HeapItem>heap;
}g_data; // top level

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
        size_t h_indx;
};

// Constructor
inline Entry *entry_new(uint32_t type) {
    Entry *ent = new Entry();
    ent->type = type;
    return ent;
}


// Destructor of entry and also deletes the heap for timers
inline void entry_del(Entry *ent) {
    if (ent->type == T_ZSET) {
        zset_clear(&ent->zset);
    }
    set_ttl(ent, -1);
    delete ent;
}

// data serialization in response

inline void buf_append(Buffer &buf, const uint8_t*data, size_t len){
    buf.insert(buf.end(), data, data+len);
};

inline void buf_append_u8(Buffer &buf, uint8_t data){
    buf.push_back(data);
}

inline void buf_append_u32(Buffer &buf, uint32_t data){
    buf_append(buf, (const uint8_t *)&data, 4);
};

inline void buf_append_i64(Buffer &buf, int64_t data){
    buf_append(buf, (const uint8_t *)&data, 8);
};

inline void buf_append_dbl(Buffer &buf, double data){
    buf_append(buf, (const uint8_t *)&data, 8);
};

// comparison function for Entry
bool equality(HNode *lhs, HNode *rhs);
bool entry_eq(HNode *lhs, HNode *rhs);

inline void out_nil(Buffer &out){
    buf_append_u8(out, TAG_NIL);
};
inline void out_str(Buffer &out, const char *s, size_t size){
    buf_append_u8(out, TAG_STR);
    buf_append_u32(out, (uint32_t)size);
    buf_append(out, (const uint8_t *)s, size);
};
inline void out_int(Buffer &out, int64_t val){
    buf_append_u8(out, TAG_INT);
    buf_append_i64(out, val);
};
inline void out_arr(Buffer &out, uint32_t n){
    buf_append_u8(out, TAG_ARR);
    buf_append_u32(out, n);
};
inline void out_dbl(Buffer &out, double val){
    buf_append_u8(out, TAG_DBL);
    buf_append_dbl(out, val);
};
inline void out_err(Buffer &out, uint32_t code, const string &msg){buf_append_u8(out, TAG_ERR);
    buf_append_u32(out, code);
    buf_append_u32(out, (uint32_t)msg.size());
    buf_append(out, (const uint8_t *)msg.data(), msg.size());
};

inline size_t out_begin_arr(Buffer &out){
    out.push_back(TAG_ARR);
    buf_append_u32(out, 0);     
    return out.size() - 4;
};
inline void out_end_arr(Buffer &out, size_t ctx, uint32_t n){
    assert(out[ctx - 1] == TAG_ARR);
    memcpy(&out[ctx], &n, 4);
};

bool str2dbl(const string &s, double &out);
bool str2int(const string &s, int64_t &out);
void set_ttl(Entry *ent, uint64_t ttl_ms);
void do_ttl(vector<string> &cmd, Buffer &out);
