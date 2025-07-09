#include <iostream>

#include "zset.h"
#include "server_helper.h"
#include "common.h"

const Zset k_empty_zset;

void do_zadd(vector<string>&cmd, Buffer &out){
    double score = 0;
    if(!str2dbl(cmd[2], score)){
        return out_err(out, ERR_BAD_ARG , "expect float");
    }
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *hnode = hm_lookup(&g_data.db, &key.node, &entry_eq);

    Entry *ent = NULL;
    if(!hnode ){
        ent = entry_new(T_ZSET);
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        hm_insert(&g_data.db, &ent->node); 
    }else{
        ent = container_of(hnode, Entry, node);
        if (ent->type != T_ZSET) {
            return out_err(out, ERR_BAD_TYP, "expect zset");
        }
    }

    const string &name = cmd[3];
    bool added = zset_insert(&ent->zset, name.data(), name.size(), score);
    return out_int(out, (int64_t)added);
}

Zset *expect_zset(string s){
    LookupKey key;
    key.key.swap(s);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *hnode = hm_lookup(&g_data.db, &key.node, &entry_eq);
    if (!hnode) {   
        return (Zset *)&k_empty_zset;
    }
    Entry *ent = container_of(hnode, Entry, node);
    return ent->type == T_ZSET ? &ent->zset : NULL;
}

void do_zrem(vector<string>&cmd, Buffer &out){
    Zset *zset = expect_zset(cmd[1]);
    if(!zset){
        return out_err(out, ERR_BAD_TYP, "expect zset");
    }
    const string &name = cmd[2];
    ZNode *znode = zset_lookup(zset, name.data(), name.size());
    if (znode) {
        zset_delete(zset, znode);
    }
    return out_int(out, znode ? 1 : 0);
}

void do_zscore(vector<string>&cmd, Buffer &out){
    Zset *zset = expect_zset(cmd[1]);
    if(!zset){
        return out_err(out, ERR_BAD_TYP, "expect zset");
    }
    const string &name = cmd[2];
}

void do_zquery(vector<string>&cmd, Buffer &out){
    double score =0;
    if(!str2dbl(cmd[2], score)){
        return out_err(out, ERR_BAD_ARG, "expect fp number");
    }
    const string &name = cmd[3];
    int64_t offset =0, limit =0;
    
    if(!str2int(cmd[4], offset) || !str2int(cmd[5], limit)){
        return out_err(out, ERR_BAD_ARG, "expect int");
    }
    Zset *zset = expect_zset(cmd[1]);
    if(!zset){
        return out_err(out, ERR_BAD_TYP, "expect zset");
    }

    if(limit <= 0){
        return out_arr(out, 0);
    }
    
    ZNode *znode = zset_seekge(zset, score, name.data(), name.size());
    znode = znode_offset(znode, offset);

    size_t  ctx = out_begin_arr(out);
    int64_t n =0;
    while(znode && n < limit){
        out_str(out, znode->name, znode->len);
        out_dbl(out, znode->score);
        znode = znode_offset(znode, +1);
        n +=2;
    }
    out_end_arr(out, ctx, (uint32_t)n);
}
