#include<iostream>
#include <vector>

#include "zset.h"
#include "server_helper.h"

using namespace std;

static void do_zadd(vector<string>&cmd, Buffer &out);
static Zset *expect_zset(string s);
static void do_zrem(vector<string>&cmd, Buffer &out);
static void do_zscore(vector<string>&cmd, Buffer &out);
static void do_zquery(vector<string>&cmd, Buffer &out);