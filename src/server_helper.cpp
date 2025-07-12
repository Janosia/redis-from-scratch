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
void do_ttl(vector<string> &cmd, Buffer &out){
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    if (!node) {
        return out_int(out, -2);    // not found
    }

    Entry *ent = container_of(node, Entry, node);
    if (ent->h_indx == (size_t)-1) {
        return out_int(out, -1);    // no TTL
    }

    uint64_t expire_at = g_data.heap[ent->h_indx].val;
    uint64_t now_ms = get_monotonic_msec();
    return out_int(out, expire_at > now_ms ? (expire_at - now_ms) : 0);
}