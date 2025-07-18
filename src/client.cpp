// C imports
#include <errno.h>
#include <unistd.h>

// Socket API import
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

// C++ imports
#include <cassert>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>

using namespace std;

static void die(const char *msg) {
    int err = errno;
    cerr << "[" << err <<"] "<< msg <<endl; 
    abort();
}

static void msg(const char *msg) { cerr << msg <<endl;}

static int32_t read_full(int fd, uint8_t *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        
        if (rv <= 0) return -1;  // error, or unexpected EOF
        
        assert((size_t)rv <= n);
        
        n -= (size_t)rv;
        
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const uint8_t *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) return -1;  // error
        
        assert((size_t)rv <= n);
        
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

const size_t k_max_msg = 32<<20;

static void buf_append(vector<uint8_t> &buf, const uint8_t *data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

static int32_t send_req(int fd, const uint8_t *text, size_t len) {
    if (len > k_max_msg) return -1;

    vector<uint8_t>wbuf;
    buf_append(wbuf, (const uint8_t *)&len, 4); // assume little endian
    buf_append(wbuf, text, len);
    return write_all(fd, wbuf.data(), wbuf.size());
}

static int32_t read_res(int fd) {
    vector<uint8_t>rbuf;
    rbuf.resize(4);
    errno = 0;

    int32_t err = read_full(fd, &rbuf[0], 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    uint32_t len =0;
    memcpy(&len, rbuf.data(), 4);  // assume little endian
    
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // reply body
    rbuf.resize(4+len);
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }
    cout << "Length : "<< len << " data " << &rbuf[4];
    return 0;
}

int main() {
    //AF_INET ->IPv4 SOCK_DGRAM -UDP
    int fd = socket(AF_INET6, SOCK_STREAM, 0);
    
    if (fd < 0) {
        die("socket()");
    }
    
    struct sockaddr_in6 addr = {};
    // binding to an address
    // network to host 
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(1234); 
    addr.sin6_addr =in6addr_loopback;  // 127.0.0.1 Ipv6 already has 16 bytes address so ig no need to change 
    
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    
    if (rv) die("connect");
    
    vector<string> query_list = {
        "hello1", 
        "hello2", 
        "hello3",
        string(k_max_msg, 'z'),
        "hello5"};
    
    for(const string &s: query_list){
        int32_t err = send_req(fd, (uint8_t *)s.data(), s.size());
        if (err) goto L_DONE;
    }
    for (size_t i = 0; i < query_list.size(); ++i) {
        int32_t err = read_res(fd);
        if (err) goto L_DONE;
    }
    
L_DONE:
    close(fd);
    return 0;
}