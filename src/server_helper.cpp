#include <iostream>
#include <math.h>
#include <cstring>

#include "server_helper.h"
using namespace std;

bool equality(HNode *lhs, HNode *rhs){
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key==re->key;
}

bool entry_eq(HNode *lhs, HNode *rhs) {
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key == re->key;
}

bool str2dbl(const string &s, double &out) {
    char *endp = NULL;
    out = strtod(s.c_str(), &endp);
    return endp == s.c_str() + s.size() && !isnan(out);
}

bool str2int(const string &s, int64_t &out) {
    char *endp = NULL;
    out = strtoll(s.c_str(), &endp, 10);
    return endp == s.c_str() + s.size();
}

/*@brief Instantiation of Cache Time To Live(TTL)*/
void set_ttl(Entry *ent, uint64_t ttl_ms){
    if(ttl_ms < 0 && ent->h_indx != (size_t)-1){
        HeapDelete(g_data.heap, ent->h_indx);
        ent->h_indx = -1;
    }else if (ttl_ms >=0){
        uint64_t  expire_at = get_monotonic_msec() + ttl_ms;
        HeapItem item = {expire_at, &ent->h_indx};
    }
}