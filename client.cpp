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
    fprintf(stderr, "[%d] %s\n", err, msg);
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
    
    // listening 
    rv = listen(fd, SOMAXCONN);
    if (rv) die("listen()"); 

    char msg[] = "hello";
    write(fd, msg, strlen(msg));

    char rbuf[64] = {};
    ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
    
    if (n < 0) die("read");
    
    cout << "server says: " << rbuf << "\n";
    close(fd);
    return 0;
}