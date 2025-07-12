// C imports
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>

// Socket API imports
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

// C++ imports
#include <iostream>
#include <cassert>
#include <string>
#include <cstring>
#include <vector>
#include <map>

// Project imports
#include "hashtable.h"
#include "zset.h"
#include "common.h"
#include "server_helper.h"
#include "zset_cmd.h"
#include "dl_list.h"
#include "timer.h"

using namespace std;


static void msg(const char *msg) { cerr << msg <<endl;}

static void msg_errno(const char *msg){cerr<< errno << " "<<msg<<endl;}

static void die(const char *msg) {
    int err = errno;
    cerr << err << " "<< msg <<endl;
    abort();
}

static void fd_set_nb(int fd){
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    
    if( errno ){
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    
    (void)fcntl(fd, F_SETFL, flags);
    
    if(errno) die("fcntl error");
}

// remove from front 
static void buf_consume(Buffer &buf, size_t n) {
    buf.erase(buf.begin(), buf.begin()+n);}

// listening socket is ready    
static int32_t handle_accept(int fd){
    struct sockaddr_in6 client_addr={};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
    
    if( connfd < 0 ){
        msg_errno("accept() error");
        return NULL;
    }
    // Using ipv6
    char ip_str[INET6_ADDRSTRLEN];
    
    inet_ntop(AF_INET6, &client_addr.sin6_addr, ip_str, sizeof(ip_str));
    
    cerr << "new client from "<< ip_str << " "<<ntohs(client_addr.sin6_port)<<endl;  

    fd_set_nb(connfd); // connection set to NON-BLOCKING MODE

    //Creating new connection
    Conn *conn = new Conn();
    conn->fd = connfd;
    conn->want_read = true;
    conn->last_active_ms = get_monotonic_msec();
    dlist_init(&g_data.idle_list);     
    dlist_init(&conn->idle_node); 
    dlist_insert(&g_data.idle_list, &conn->idle_node);
    if(g_data.fd2conn.size() <= (size_t)connfd){
        g_data.fd2conn.resize(conn->fd+1);
    }
    
    assert(!g_data.fd2conn[conn->fd]);
    g_data.fd2conn[conn->fd] = conn; 
    
    return 0;
}
/*@brief : destroying a socket*/
static void conn_destroy(Conn *conn){
    (void)close(conn->fd);
    g_data.fd2conn[conn->fd] = NULL;
    dlist_detach(&conn->idle_node);
    delete(conn);
}

static bool hnode_same(HNode *node, HNode *key) {
    return node == key;
}

void process_timers(){
    uint64_t now_ms = get_monotonic_msec();
    // handling idle timers using doubly linked list
    while(!dlist_empty(&g_data.idle_list)){
        Conn *conn = container_of(g_data.idle_list.next, Conn, idle_node);
        uint64_t next_ms = conn->last_active_ms =k_idle_timeout_ms;
        if(next_ms >=now_ms){
            break;
        }
        cerr << "removing idle connection "<< conn->fd<<endl;
        conn_destroy(conn);
    }
    // handling TTL timers using Heap
    vector<HeapItem>&heap = g_data.heap;
    const int k_max_works = 2000; // arbitrary constant to ensure that server is not too busy deleting stuff
    int next_works = 0;
    while(!heap.empty() && heap[0].val < now_ms && next_works++ < k_max_works){
        Entry *ent = container_of(heap[0].ref, Entry, h_indx);
        hm_delete(&g_data.db, &ent->node, &hnode_same);
        entry_del(ent); // delete the key
    }
}

// Helper Functions
static bool read_u32(const uint8_t *&cur, const uint8_t *end, uint32_t &out){
    if( cur+4 >end) return false;
    memcpy( &out, cur, 4);
    cur += 4;
    return true;
}

static bool read_str(const uint8_t *&cur, const uint8_t *end,size_t n, string &out){
    if( cur+n > end) return false;
    
    out.assign(cur, cur+n);
    cur += n;
    return true;
}

static int32_t parse_req(const uint8_t *data, size_t size, vector<string>&out){
    const uint8_t *end = data +size;
    uint32_t nstr = 0;
    
    if( !read_u32(data, end, nstr) ) return -1;

    if( nstr > k_max_args) return -1;

    while( out.size() < nstr ){
        uint32_t len = 0;
        if( !read_u32(data, end, len) ) return -1;
        out.push_back(string());
        if( !read_str(data, end,len, out.back()) ) return -1;
    }
    if(data != end) return -1;
    return 0;
}

static void do_get(vector<string>&cmd, Buffer &out){
    // dummy entry 
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    // lookup 
    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    
    if(!node) return out_nil(out);
    
    // copy value and send back
    Entry *ent = container_of(node, Entry, node);
    if(ent->type != T_STR){
        return out_err(out, ERR_BAD_TYP, "not a string value");
    }
    return out_str(out, ent->str.data(), ent->str.size());
}

static void do_set(vector<string>&cmd, Buffer &out){
    // dummy entry
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    
    // lookup 
    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    
    if( node ){
        // update the obtained value from lookup
        Entry *ent = container_of(node, Entry, node);
        if (ent->type != T_STR) {
            return out_err(out, ERR_BAD_TYP, "a non-string value exists");
        }
        ent->str.swap(cmd[2]);
    }else{ // nothing found 
        // create and insert new pair
        Entry *ent = new Entry();
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        ent->str.swap(cmd[2]);
        // insertion
        hm_insert(&g_data.db, &ent->node);
    }
    return out_nil(out);
}

static void do_del(vector<string> &cmd, Buffer &out) {
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *node = hm_delete(&g_data.db, &key.node, &entry_eq);
    
    if (node) { 
        // found pair now deallocate the memory
        delete container_of(node, Entry, node);
    }
    
    return out_int(out, node ? 1 :0); //number of deleted keys
}

static bool cb_keys(HNode *node, void *arg) {
    Buffer &out = *(Buffer *)arg;
    const string &key = container_of(node, Entry, node)->key;
    out_str(out, key.data(), key.size());
    return true;
}

static void do_keys(vector<string> &, Buffer &out) {
    
    out_arr(out, (uint32_t)hm_size(&g_data.db));
    
    hm_foreach(&g_data.db, &cb_keys, (void *)&out);
}

static void response_begin(Buffer &out, size_t *header) {
    *header = out.size();       // messege header position
    buf_append_u32(out, 0);     // reserve space
}
static size_t response_size(Buffer &out, size_t header) {
    return out.size() - header - 4;
}
static void response_end(Buffer &out, size_t header) {
    size_t msg_size = response_size(out, header);
    if (msg_size > k_max_msg) {
        out.resize(header + 4);
        out_err(out, ERR_TOO_BIG, "response is too big.");
        msg_size = response_size(out, header);
    }
    uint32_t len = (uint32_t)msg_size;
    memcpy(&out[header], &len, 4);
}


/*@brief Request Commands
@param cmd  vector of cmds to perform
@param out  Buffer where output is displayed*/
static void do_request(vector<string> &cmd, Buffer &out) {
    if (cmd.size() == 2 && cmd[0] == "get") {
        return do_get(cmd, out);
    } else if (cmd.size() == 3 && cmd[0] == "set") {
        return do_set(cmd, out);
    } else if (cmd.size() == 2 && cmd[0] == "del") {
        return do_del(cmd, out);
    } else if (cmd.size() == 1 && cmd[0] == "keys"){
        return do_keys(cmd, out);      
    }else if (cmd.size() == 4 && cmd[0] == "zadd") {
        return do_zadd(cmd, out);
    } else if (cmd.size() == 3 && cmd[0] == "zrem") {
        return do_zrem(cmd, out);
    } else if (cmd.size() == 3 && cmd[0] == "zscore") {
        return do_zscore(cmd, out);
    } else if (cmd.size() == 6 && cmd[0] == "zquery") {
        return do_zquery(cmd, out);
    }else{
        return out_err(out, ERR_UNKNOWN, "unknown command ");
    }
}


/*@brief TTL Commands
@param cmd  vector of cmds to perform
@param out  Buffer where output is displayed*/
static void do_expire(vector<string> &cmd, Buffer &out) {
    // parse args
    int64_t ttl_ms = 0;
    if (!str2int(cmd[2], ttl_ms)) {
        return out_err(out, ERR_BAD_ARG, "expect int64");
    }
    // lookup the key
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    // set TTL
    if (node) {
        Entry *ent = container_of(node, Entry, node);
        set_ttl(ent, ttl_ms);
    }
    return out_int(out, node ? 1: 0);
}

static bool try_one_request(Conn *conn){
    if(conn->incoming.size() <4) return false;
    size_t header_pos = 0;
    uint32_t  len = 0;
    memcpy(&len, conn->incoming.data(), 4);
    if(len > k_max_msg){
        msg("too long");
        conn->want_close = true;
        return false;
    }

    if ( 4 + len > conn->incoming.size() ) return false;
    
    const uint8_t *request = &conn->incoming[4];
    cout << "client says "<< len <<" "; 
    len = len < 100 ? len : 100;
    cout << request <<endl;

    buf_append( conn->outgoing, (const uint8_t *)&len , 4 );
    buf_append( conn->outgoing, request, len );

    buf_consume( conn->incoming , 4 + len );

    vector<string>cmd;

    if( parse_req( request, len, cmd ) < 0 ){
        conn->want_close = true;
        return false;
    }
    
    response_begin(conn->outgoing, &header_pos);
    do_request(cmd, conn->outgoing);
    response_end(conn->outgoing, header_pos);

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
    
    // Intialization
    dlist_init(&g_data.idle_list);
    thread_pool_init(&g_data.thread_pool, 4);
    
    int fd = socket(AF_INET6, SOCK_STREAM, 0);
    if ( fd < 0 ) die("socket()");
    
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

    // event loop
    vector<struct pollfd> poll_args;
    while (true) {
        
        poll_args.clear();
        struct pollfd pfd = {fd, POLLIN,0};
        poll_args.push_back(pfd);

        for( Conn *conn : g_data.fd2conn){
            
            if(!conn) continue;
            
            struct pollfd pfd ={conn->fd, POLLERR, 0};
            
            if(conn->want_read) pfd.events |=POLLIN;
            
            if(conn->want_write) pfd.events |=POLLOUT;
            
            poll_args.push_back(pfd);
        }

        // waiting for readiness 
        int32_t timeout_ms = next_timer_ms();
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), -1);
        
        if( rv < 0 && errno == EINTR) continue;
        
        if( rv < 0 ) die("poll");

        if( poll_args[0].revents ){
            handle_accept(fd);
        }

        for(size_t i=1; i<poll_args.size(); ++i){
            
            uint32_t ready = poll_args[i].revents;
            
            if( ready == 0) continue;
            
            Conn *conn = g_data.fd2conn[poll_args[i].fd];

            conn->last_active_ms = get_monotonic_msec();
            dlist_detach(&conn->idle_node);
            dlist_insert(&g_data.idle_list, &conn->idle_node);
            
            // handle IO
            if( ready & POLLIN ){
                assert(conn->want_read);
                handle_read(conn);
            }
            if ( ready & POLLOUT ){
                assert(conn->want_write);
                handle_write(conn);
            }

            // closing socket either encountered ERROR or following cmd
            if ( (ready & POLLERR) || conn->want_close ) {
                (void)close(conn->fd);
                g_data.fd2conn[conn->fd]=NULL;
                delete conn;
            }
        }
        process_timers();
    }
    return 0;
}
