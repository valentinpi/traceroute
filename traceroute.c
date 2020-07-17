#define _POSIX_C_SOURCE 200112L

#include <stdbool.h>
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

#define MAX_HOPS 30
#define PACKET_SIZE 64

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
    sock_hints.ai_flags = AI_CANONNAME;

    struct addrinfo *res = NULL;
    err = getaddrinfo(argv[1], target_service, &sock_hints, &res);
    if (err != 0) {
        const char *error = gai_strerror(err);
        printf("(%d) %s\n", err, error);
        return EXIT_FAILURE;
    }
    // TODO: Test for all services found
    int num = 0;
    struct addrinfo *iter = res;
    while (iter != NULL) {
        printf("Address found: %s \n", iter->ai_canonname);
        num += 1;
        iter = iter->ai_next;
    }
    printf("Number of addresses found: %d\n", num);
    memcpy(&sock_addr.sin6_addr, res[0].ai_addr, sizeof(sock_addr.sin6_addr));

    // Connect, do not bind, we are a client!
    connect(sock, (struct sockaddr*) &sock_addr, sizeof(sock_addr));

    // "Rule": Use int everywhere
    // Critical here, since with a wrong optval value the EINVAL error on occur
    int opt = 1;
    err = setsockopt(sock, IPPROTO_IPV6, IPV6_RECVERR, &opt, sizeof(int));
    if (err != 0) {
        perror("setsockopt");
        return EXIT_FAILURE;
    }

    // We use the equivalent concept of maximum hops in IPv6
    printf("Sending up to %d dummy packets with size %d each\n", MAX_HOPS, PACKET_SIZE);
    printf("hops - address\n");
    char *packet = calloc(PACKET_SIZE, 1);

    int hops = 1;
    while (hops <= MAX_HOPS) {
        // If the previous setsockopt one worked, we can safely assume that this will work
        setsockopt(sock, IPPROTO_IPV6, IPV6_HOPLIMIT, &hops, sizeof(int));
        err = send(sock, (const void*) packet, PACKET_SIZE, 0);

        if (err != -1) {
            printf("Reached %s\n", argv[1]);
            break;
        }

        struct msghdr msg;
        err = recvmsg(sock, &msg, 0);
        if (err == -1) {
            perror("recvmsg");
            return EXIT_FAILURE;
        }

        printf("%4d - %s\n", hops, (char*) msg.msg_name);
        hops += 1;
    }

    free(packet);
    close(sock);
    freeaddrinfo(res);
    return EXIT_SUCCESS;
}
