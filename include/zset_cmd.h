#pragma once

#include <iostream>
#include <vector>

#include "zset.h"
#include "server_helper.h"

using namespace std;

void do_zadd(vector<string>&cmd, Buffer &out);
Zset *expect_zset(string s);
void do_zrem(vector<string>&cmd, Buffer &out);
void do_zscore(vector<string>&cmd, Buffer &out);
void do_zquery(vector<string>&cmd, Buffer &out);