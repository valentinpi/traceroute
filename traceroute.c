#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Task sheet specifies 33434 as service to be targetted
#define TARGET_PORT 33434
const char target_service[] = "33434";

int main(int argc, char *argv[])
{
    int sock = 0;
    struct sockaddr_in6 sock_addr;
    struct addrinfo sock_hints;
    int err = 0;

    if (argc < 2) {
        printf("Usage: traceroute <address>\n");
        return EXIT_FAILURE;
    }

    // Create socket
    // Assume that protocol = 0 will choose UDP
    sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    // Initialize address structure
    memset(&sock_addr, 0, sizeof(struct sockaddr_in6));
    sock_addr.sin6_family = AF_INET6;
    sock_addr.sin6_port = htons(TARGET_PORT);
    // ignore sin6_flowinfo and sin6_scope_id for simplicity

    // Initialize hints and resolve DNS address mapping
    memset(&sock_hints, 0, sizeof(struct addrinfo));
    sock_hints.ai_family = AF_INET6;
    sock_hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo *res = NULL;
    err = getaddrinfo(argv[1], target_service, &sock_hints, &res);
    if (err != 0) {
        const char *error = gai_strerror(err);
        printf("(%d) %s\n", err, error);
        return EXIT_FAILURE;
    }
    // TODO: Test for all services found
    int num = 1;
    struct addrinfo *iter = res;
    while (iter != NULL) {
        num += 1;
        iter = iter->ai_next;
    }
    printf("Number of addresses found: %d\n", num);
    memcpy(&sock_addr.sin6_addr, res[0].ai_addr, sizeof(sock_addr.sin6_addr));

    // Connect, do not bind, we are a client!
    connect(sock, (struct sockaddr*) &sock_addr, sizeof(sock_addr));

    close(sock);

    freeaddrinfo(res);
    return EXIT_SUCCESS;
}
