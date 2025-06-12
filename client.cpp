#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

using namespace std;

static void die(const char *msg) {
    int err = errno;
    // fprintf(stderr, "[%d] %s\n", err, msg);
    cerr << "[ " << err <<" ] "<< msg <<endl; 
    abort();
}
// struct sockaddr_in6 {
//     uint16_t        sin6_family;   // AF_INET6
//     uint16_t        sin6_port;     // port in big-endian
//     uint32_t        sin6_flowinfo; // ignore
//     struct in6_addr sin6_addr;     // IPv6
//     uint32_t        sin6_scope_id; // ignore
// };

// struct in6_addr {
//     uint8_t         s6_addr[16];   // IPv6
// };
static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) return -1;  // error, or unexpected EOF
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

const size_t k_max_msg = 4096;

static int32_t query(int fd, const char *text) {
    uint32_t len = (uint32_t)strlen(text);
    
    if (len > k_max_msg) return -1;

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);  // assume little endian
    memcpy(&wbuf[4], text, len);
    
    if (int32_t err = write_all(fd, wbuf, 4 + len)) return err;
    

    // 4 bytes header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    // printf("server says: %.*s\n", len, &rbuf[4]);
    cout << "server says: "<< string(&rbuf[4], len)<<endl;
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
    addr.sin_family = AF_INET6;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1
    
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    
    if (rv) die("connect");
    
    // char msg[] = "hello";
    // write(fd, msg, strlen(msg));

    // char rbuf[64] = {};
    // ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
    
    // if (n < 0) die("read");
    
    // cout << "server says: " << rbuf << "\n";
    // multiple requests
    int32_t err = query(fd, "hello1");
    if (err) goto L_DONE;
    
    err = query(fd, "hello2");
    if (err) goto L_DONE;
    
    err = query(fd, "hello3");
    if (err) goto L_DONE;
L_DONE:
    close(fd);
    return 0;
}