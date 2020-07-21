#define _POSIX_C_SOURCE 200112L

#include <errno.h>
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

#define CONTROL_BUFLEN 8192

void print_ipv6_addr(struct sockaddr_in6 *addr);

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

    sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    // Initialize address structure
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin6_family = AF_INET6;
    sock_addr.sin6_port = htons(TARGET_PORT);
    // ignore sin6_flowinfo and sin6_scope_id for simplicity

    // Initialize hints and resolve DNS address mapping
    memset(&sock_hints, 0, sizeof(struct addrinfo));
    sock_hints.ai_flags = AI_CANONNAME;
    sock_hints.ai_family = AF_INET6;
    sock_hints.ai_socktype = SOCK_DGRAM;
    sock_hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo *res = NULL;
    err = getaddrinfo(argv[1], target_service, &sock_hints, &res);
    if (err != 0) {
        const char *error = gai_strerror(err);
        printf("(%d) %s\n", err, error);
        return EXIT_FAILURE;
    }

    // Count addresses
    {
        int num = 0;
        struct addrinfo *iter = res;
        while (iter != NULL) {
            printf("Address found: %s\n", iter->ai_canonname);
            num += 1;
            iter = iter->ai_next;
        }

        printf("Number of addresses found: %d\n", num);
        if (num == 0) {
            return EXIT_FAILURE;
        }
    }

    // TODO: Test for all services found
    struct sockaddr_in6 *ipv6_addr = (struct sockaddr_in6*) res[0].ai_addr;
    sock_addr = *ipv6_addr;

    //print_ipv6_addr(ipv6_addr);
    print_ipv6_addr(&sock_addr);

    // Connect, do not bind, we are a client!
    err = connect(sock, (struct sockaddr*) &sock_addr, sizeof(sock_addr));
    if (err == -1) {
        perror("connect");
        return EXIT_FAILURE;
    }

    // "Rule": Use int everywhere
    // Critical here, since with a wrong optval value the EINVAL error on occur
    int opt = 1;
    err = setsockopt(sock, IPPROTO_IPV6, IPV6_RECVERR, &opt, sizeof(int));
    if (err == -1) {
        perror("setsockopt");
        return EXIT_FAILURE;
    }

    // We use the equivalent concept of maximum hops in IPv6
    printf("Sending up to %d dummy packets with size of %d bits each\n", MAX_HOPS, PACKET_SIZE);
    printf("hops - address\n");
    char *packet = calloc(PACKET_SIZE, 1);

    void *control_buf = calloc(CONTROL_BUFLEN, 1);
    int hops = 1;
    while (hops <= MAX_HOPS) {
        err = setsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hops, sizeof(int));
        if (err == -1) {
            perror("setsockopt");
            printf("%d\n", errno);
            return EXIT_FAILURE;
        }

        //int test = 0;
        //socklen_t test_len = sizeof(int);
        //err = getsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &test, &test_len);
        //if (err == -1) {
        //    perror("getsockopt");
        //    printf("%d\n", errno);
        //    return EXIT_FAILURE;
        //}
        //printf("hops = %d\n", hops);
        //printf("test = %d\n", test);

        err = send(sock, (const void*) packet, PACKET_SIZE, 0);
        if (err == -1) {
            perror("send");
            return EXIT_FAILURE;
        }

        // Initialize control message buffer
        memset(control_buf, 0, CONTROL_BUFLEN);

        int name_buflen = sizeof(struct sockaddr_in6);
        char name_buf[name_buflen];
        memset(name_buf, 0, name_buflen);
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_name = name_buf;
        msg.msg_namelen = name_buflen;
        msg.msg_control = control_buf;
        msg.msg_controllen = CONTROL_BUFLEN;

        // Wait a little for the network
        sleep(1);
        err = recvmsg(sock, &msg, MSG_ERRQUEUE);
        if (err == -1) {
            printf("errno = %d\n", errno);
            perror("recvmsg");
            continue;
        }

        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        while (cmsg != NULL) {
            printf("%d\n", cmsg->cmsg_type);
            printf("%d\n", msg.msg_namelen);
            print_ipv6_addr((struct sockaddr_in6*) &msg.msg_name);
            cmsg = CMSG_NXTHDR(&msg, cmsg);
        }

        //printf("%4d - %s\n", hops, (char*) msg.msg_name);
        printf("%4d\n", hops);
        hops += 1;
    }
    free(control_buf);

    free(packet);
    close(sock);
    freeaddrinfo(res);
    return EXIT_SUCCESS;
}

void print_ipv6_addr(struct sockaddr_in6 *addr) {
    char addr_str[INET6_ADDRSTRLEN];
    memset(addr_str, 0, INET6_ADDRSTRLEN);
    inet_ntop(AF_INET6, &addr->sin6_addr, addr_str, INET6_ADDRSTRLEN);
    printf("IPv6 Adress: %s\n", addr_str);
}
