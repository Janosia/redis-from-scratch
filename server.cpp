#include <errno.h>

#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include <iostream>
#include <cassert>
#include <string>
#include <cstring>
#include <vector>
#include <map>


#include "hashtable.h"
using namespace std;

/* struct sockaddr_in6 {
    uint16_t        sin6_family;    AF_INET6
    uint16_t        sin6_port;     port in big-endian
    uint32_t        sin6_flowinfo; ignore
    struct in6_addr sin6_addr;     IPv6
    uint32_t        sin6_scope_id; ignore
};

struct in6_addr {
    vector<uint8_t> s6_addr(16);    IPv6
};*/

#define container_of(ptr, T, member) \
    ((T *)( (char *)ptr - offsetof(T, member) ))


struct Conn {
    int fd = -1;
    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    vector<uint8_t>incoming;
    vector<uint8_t>outgoing;
};

struct Response{
    uint32_t status =0;
    vector<uint8_t>data;
};

enum {
    RES_OK = 0,
    RES_ERR = 1,    // error
    RES_NX = 2,     // key not found
};

static struct {HMap db;}g_data; // kv store

struct Entry{
    struct HNode node;
    string key;
    string val;
};

struct LookupKey{
    HNode node;
    string key;
};

static bool equality(HNode *lhs, HNode *rhs){
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key==re->key;
}

static void msg(const char *msg) { cerr << msg <<endl;}

static void msg_errno(const char *msg){cerr<< errno << " "<<msg<<endl;}

static void die(const char *msg) {
    int err = errno;
    cerr << err << " "<< msg <<endl;
    abort();
}

static void fd_set_nb(int fd){
    errno =0;
    int flags = fcntl(fd, F_GETFL, 0);
    if(errno){
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno=0;
    (void)fcntl(fd, F_SETFL, flags);
    
    if(errno) die("fcntl error");
}

const size_t k_max_msg = 32 <<20, k_max_args = 200 *1000;



static void buf_append(vector<uint8_t> &buf, const uint8_t*data, size_t len) {
    buf.insert(buf.end(), data, data+len);}

static void buf_consume(vector<uint8_t> &buf, size_t n) {
    buf.erase(buf.begin(), buf.begin()+n);}

static Conn * handle_accept(int fd){
    struct sockaddr_in6 client_addr={};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
    if(connfd <0 ){
        msg_errno("accept() error");
        return NULL;
    }
    // Using ipv6
    char ip_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &client_addr.sin6_addr, ip_str, sizeof(ip_str));
    cerr << "new client from %u.%u.%u.%u.%u "<<ip_str <<ntohs(client_addr.sin6_port)<<endl;  
    fd_set_nb(connfd); // NON-BLOCKING MODE

    Conn *conn = new Conn();
    conn->fd = connfd;
    conn->want_read = true;
    return conn;
}

//helper function
static bool read_u32(const uint8_t *&cur, const uint8_t *end, uint32_t &out){
    if(cur+4 >end) return false;
    memcpy(&out, cur, 4);
    cur+=4;
    return true;
}

static bool read_str(const uint8_t *&cur, const uint8_t *end,size_t n, string &out){
    if(cur+n>end) return false;
    
    out.assign(cur, cur+n);
    cur+=n;
    return true;
}

static int32_t parse_req(const uint8_t *data, size_t size, vector<string>&out){
    const uint8_t *end = data +size;
    uint32_t nstr=0;
    if(!read_u32(data, end, nstr)) return -1;

    if(nstr>k_max_args) return -1;

    while(out.size() <nstr){
        uint32_t len =0;
        if(!read_u32(data, end, len)) return -1;
        out.push_back(string());
        if(!read_str(data, end,len, out.back())) return -1;
    }
    if(data !=end) return -1;
    return 0;
}

static bool entry_eq(HNode *lhs, HNode *rhs) {
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key == re->key;
}

static uint64_t  str_hash(const uint8_t *data, size_t len){
    uint32_t h = 0x811C9DC5;
    for(size_t i=0; i<len; i++){
        h= (h+data[i])*0x01000193;
    }
    return h;
}
static void do_get(vector<string>&cmd, Response &out){
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    if(!node){
        out.status = RES_NX;
        return;
    }
    string &val = container_of(node, Entry, node)->val;
    assert(val.size() <=k_max_msg);
    out.data.assign(val.begin(), val.end());
}

static void do_set(vector<string>&cmd, Response &){
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    if(!node){
       container_of(node, Entry, node)->val.swap(cmd[2]);
    }else{
        Entry *ent = new Entry();
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        ent->val.swap(cmd[2]);
        hm_insert(&g_data.db, &ent->node);
    }
}

static void do_del(vector<string> &cmd, Response &) {
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *node = hm_delete(&g_data.db, &key.node, &entry_eq);
    if (node) { 
        delete container_of(node, Entry, node);
    }
}

static void do_request(std::vector<std::string> &cmd, Response &out) {
    if (cmd.size() == 2 && cmd[0] == "get") {
        return do_get(cmd, out);
    } else if (cmd.size() == 3 && cmd[0] == "set") {
        return do_set(cmd, out);
    } else if (cmd.size() == 2 && cmd[0] == "del") {
        return do_del(cmd, out);
    } else {
        out.status = RES_ERR;      
    }
}

static void make_response(const Response &resp, vector<uint8_t>&out){
    uint32_t resp_len = 4+(uint32_t)resp.data.size();
    buf_append(out, (const uint8_t *)&resp_len, 4);
    buf_append(out, (const uint8_t *)&resp.status, 4);
    buf_append(out, resp.data.data(), resp.data.size());
}

static bool try_one_request(Conn *conn){
    if(conn->incoming.size() <4) return false;

    uint32_t  len =0;
    memcpy(&len, conn->incoming.data(), 4);
    if(len > k_max_msg){
        msg("too long");
        conn->want_close = true;
        return false;
    }

    if(4+len > conn->incoming.size()) return false;
    const uint8_t *request = &conn->incoming[4];
    cout << "client says "<< len <<" "; 
    len = len<100?len:100;
    cout << request <<endl;

    buf_append(conn->outgoing, (const uint8_t *)&len , 4);
    buf_append(conn->outgoing, request, len);

    buf_consume(conn->incoming , 4+len);

    vector<string>cmd;
    if(parse_req(request, len, cmd)<0){
        conn->want_close = true;
        return false;
    }
    Response resp;
    do_request(cmd, resp);
    make_response(resp, conn->outgoing);

    return true;
}
static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const uint8_t *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static void handle_write(Conn *conn){
    assert(conn->outgoing.size()>0);
    ssize_t rv = write(conn->fd, &conn->outgoing[0], conn->outgoing.size());
    if(rv<0 && errno == EAGAIN) return ;
    if(rv<0){
        msg_errno("write() error");
        conn->want_close = true;
        return;
    }
    buf_consume(conn->outgoing, (size_t)rv);

    if(conn->outgoing.size() == 0){
        conn->want_read = true;
        conn->want_write = false;
    }
}

static void handle_read(Conn *conn){
    uint8_t  buf[64*1024];
    ssize_t rv = read(conn->fd, buf, sizeof(buf));

    if(rv<0 && errno == EAGAIN) return ;

    else if(rv<0){
        msg_errno("read() error");
        conn->want_close = true;
        return;
    }
    else if(!rv){
        if(!conn->incoming.size())msg("client closed");
        else msg("unexpected EOF");
        conn->want_close = true;
        return ;
    }
    else {
        buf_append(conn->incoming, buf, (size_t)rv);
        while(try_one_request(conn)){} // pipeling

        if(conn->outgoing.size() >0){
            // we have a response
            conn->want_read = false;
            conn->want_write = true;
            return handle_write(conn);
        }
    }
}


int main() {
    int fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (fd < 0) die("socket()");
    

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind

    struct sockaddr_in6 addr = {};
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(1234);
    addr.sin6_addr = in6addr_any;    // 0.0.0.0
    
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    
    if (rv) die("bind()");

    fd_set_nb(fd);
    
    rv = listen(fd, SOMAXCONN);
    if (rv) die("listen()");
    

    vector<Conn *>fd2Conn;

    vector<struct pollfd> poll_args;
    while (true) {

        poll_args.clear();
        struct pollfd pfd = {fd, POLLIN,0};
        poll_args.push_back(pfd);

        for( auto conn : fd2Conn){
            if(!conn) continue;
            struct pollfd pfd ={conn->fd, POLLERR, 0};
            if(conn->want_read) pfd.events |=POLLIN;
            if(conn->want_write) pfd.events |=POLLOUT;
            poll_args.push_back(pfd);
        }

        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), -1);
        if(rv<0 && errno == EINTR) continue;
        if(rv<0) die("poll");

        if(poll_args[0].revents){
            if(Conn *conn = handle_accept(fd)){
                if(fd2Conn.size() <=(size_t)conn->fd) fd2Conn.resize(conn->fd+1);
                assert(!fd2Conn[conn->fd]);
                fd2Conn[conn->fd] = conn;
            }
        }

        for(size_t i=1; i<poll_args.size(); ++i){
            uint32_t ready = poll_args[i].revents;
            if(ready==0){
                continue;
            }
            Conn *conn = fd2Conn[poll_args[i].fd];

            if(ready &POLLIN){
                assert(conn->want_read);
                handle_read(conn);
            }
            if (ready &POLLOUT){
                assert(conn->want_write);
                handle_write(conn);
            }
            if ((ready & POLLERR) || conn->want_close) {
                (void)close(conn->fd);
                fd2Conn[conn->fd]=NULL;
                delete conn;
            }
        }
    }
    return 0;
}
