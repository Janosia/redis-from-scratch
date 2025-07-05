#pragma once 


#include <iostream>
#include <vector> 
#include <unistd.h>

#include "hashtable.h"

using namespace std;

const size_t k_max_msg = 32 <<20, k_max_args = 200 *1000;
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
};

class LookupKey{
    public:
        HNode node;
        string key;
};
