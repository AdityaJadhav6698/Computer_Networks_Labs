#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

struct flsm_request {
    char cidr[32];
    unsigned int subnet_count;
};

static int parse_ipv4(const char *text, uint32_t *address) {
    unsigned int a, b, c, d;
    char extra;
    if (sscanf(text, "%u.%u.%u.%u%c", &a, &b, &c, &d, &extra) != 4 ||
        a > 255 || b > 255 || c > 255 || d > 255) {
        return 0;
    }
    *address = (a << 24) | (b << 16) | (c << 8) | d;
    return 1;
}

static void print_ipv4(uint32_t address) {
    printf("%u.%u.%u.%u", (address >> 24) & 255, (address >> 16) & 255,
           (address >> 8) & 255, address & 255);
}

static uint32_t mask_for_prefix(unsigned int prefix) {
    return prefix == 0 ? 0 : UINT32_MAX << (32 - prefix);
}

static unsigned int bits_required(unsigned int value) {
    unsigned int bits = 0;
    unsigned int capacity = 1;
    while (capacity < value) {
        capacity <<= 1;
        ++bits;
    }
    return bits;
}

void handle_client(int client_fd) {
    struct flsm_request req;
    char ack = 'K'; // Acknowledgment byte
    
    if (recv(client_fd, &req, sizeof(req), 0) <= 0) {
        close(client_fd);
        return;
    }

    printf("\n--- Processing Request from Client ---\n");
    printf("Requested CIDR: %s\n", req.cidr);
    printf("Requested Subnets: %u\n", req.subnet_count);

    uint32_t address, network, mask;
    unsigned int prefix, borrowed, new_prefix;
    uint64_t subnet_size, i;

    char *slash = strchr(req.cidr, '/');
    if (slash == NULL || slash == req.cidr || slash[1] == '\0') {
        printf("Error: Invalid CIDR notation.\n");
        send(client_fd, &ack, sizeof(ack), 0);
        close(client_fd);
        return;
    }
    *slash = '\0';
    prefix = (unsigned int)strtoul(slash + 1, NULL, 10);
    
    if (prefix > 32 || !parse_ipv4(req.cidr, &address)) {
        printf("Error: Invalid IPv4 address or prefix.\n");
        send(client_fd, &ack, sizeof(ack), 0);
        close(client_fd);
        return;
    }

    if (req.subnet_count == 0) {
        printf("Error: Number of subnets must be positive.\n");
        send(client_fd, &ack, sizeof(ack), 0);
        close(client_fd);
        return;
    }

    borrowed = bits_required(req.subnet_count);
    new_prefix = prefix + borrowed;
    if (new_prefix > 32) {
        printf("Error: Not enough host bits for %u subnets in /%u.\n", req.subnet_count, prefix);
        send(client_fd, &ack, sizeof(ack), 0);
        close(client_fd);
        return;
    }

    mask = mask_for_prefix(new_prefix);
    network = address & mask_for_prefix(prefix);
    subnet_size = UINT64_C(1) << (32 - new_prefix);

    printf("\nRequired borrowed bits: %u\n", borrowed);
    printf("Actual subnets created: %u\n", 1u << borrowed);
    printf("New prefix: /%u\n", new_prefix);
    printf("Subnet mask: ");
    print_ipv4(mask);
    printf("\n\n");

    for (i = 0; i < (UINT64_C(1) << borrowed); ++i) {
        uint32_t first = network + (uint32_t)(i * subnet_size);
        uint32_t last = first + (uint32_t)subnet_size - 1;
        
        printf("Subnet %llu\n", (unsigned long long)(i + 1));
        printf("  First IP       : ");
        print_ipv4(first);
        printf("\n  Last IP        : ");
        print_ipv4(last);
        printf("\n  Usable IP range: ");
        
        if (subnet_size <= 2) {
            printf("No usable host addresses");
        } else {
            print_ipv4(first + 1);
            printf(" - ");
            print_ipv4(last - 1);
        }
        printf("\n  Subnet mask    : ");
        print_ipv4(mask);
        printf(" (/%u)\n\n", new_prefix);
    }
    
    printf("--- Finished Processing ---\n\n");

    /* Send acknowledgment to client so it can close */
    send(client_fd, &ack, sizeof(ack), 0);
    close(client_fd);
}

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);

    printf("FLSM Server listening on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd >= 0) {
            handle_client(client_fd);
        }
    }
    return 0;
}