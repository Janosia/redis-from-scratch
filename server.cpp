#include <stdint.h>
#include <stdlib.h>

#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>


#include <iostream>
#include <cassert>
#include <string>
#include <cstring>
#include <vector>
using namespace std;

// struct sockaddr_in6 {
//     uint16_t        sin6_family;   // AF_INET6
//     uint16_t        sin6_port;     // port in big-endian
//     uint32_t        sin6_flowinfo; // ignore
//     struct in6_addr sin6_addr;     // IPv6
//     uint32_t        sin6_scope_id; // ignore
// };

// struct in6_addr {
//     vector<uint8_t> s6_addr(16);   // IPv6
// };


static void msg(const char *msg) { cerr << msg <<endl;}

static void die(const char *msg) {
    int err = errno;
    cerr << err << " "<<msg <<endl;
    abort();
}

const size_t k_max_msg = 4096;

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

static int32_t write_all(int fd, const char *buf, size_t n) {
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

static int32_t one_request(int connfd) {
    // 4 bytes header
    char rbuf[4 + k_max_msg];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    cerr<< "client says: "<< &rbuf[4]<<endl;
    
    
    // reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}

int main() {
    int fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (fd < 0) die("socket()");
    

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in6 addr = {};
    addr.sin6_family = AF_INET6;
    addr.sin6_port = ntohs(1234);
    // addr.sin6_addr.s6_addr = ntohs(0);    // wildcard address 0.0.0.0
    
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    
    if (rv) die("bind()");

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv) die("listen()");
    
    while (true) {
        // accept
        struct sockaddr_in6 client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd < 0) continue;  
    
        while(true){
            int32_t  err = one_request(connfd);
            if(err) break;
        }
        close(connfd);
    }
    return 0;
}
