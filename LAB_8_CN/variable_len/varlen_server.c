#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

typedef struct {
    unsigned int requested_hosts;
    unsigned int prefix;
    uint64_t block_size;
} Requirement;

static int parse_ipv4(const char *text, uint32_t *address) {
    unsigned int a, b, c, d;
    char extra;
    if (sscanf(text, "%u.%u.%u.%u%c", &a, &b, &c, &d, &extra) != 4 ||
        a > 255 || b > 255 || c > 255 || d > 255) return 0;
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

static int compare_requirements(const void *left, const void *right) {
    const Requirement *a = (const Requirement *)left;
    const Requirement *b = (const Requirement *)right;
    if (a->block_size < b->block_size) return 1;
    if (a->block_size > b->block_size) return -1;
    return 0;
}

static uint64_t block_for_hosts(unsigned int hosts, unsigned int *prefix) {
    uint64_t block = 4;
    *prefix = 30;
    while (block - 2 < hosts && *prefix > 0) {
        block <<= 1;
        --*prefix;
    }
    return block;
}

void handle_client(int client_fd) {
    char cidr[32];
    unsigned int subnet_count;
    unsigned int *requested_hosts = NULL;
    char ack = 'K';

    /* Receive Inputs */
    if (recv(client_fd, cidr, sizeof(cidr), 0) <= 0) {
        close(client_fd);
        return;
    }
    recv(client_fd, &subnet_count, sizeof(subnet_count), 0);
    
    requested_hosts = (unsigned int *)malloc(subnet_count * sizeof(unsigned int));
    recv(client_fd, requested_hosts, subnet_count * sizeof(unsigned int), 0);

    printf("\n--- Processing Request from Client ---\n");
    printf("Requested CIDR: %s\n", cidr);
    printf("Requested Subnets: %u\n", subnet_count);

    uint32_t address, network, base_mask;
    unsigned int base_prefix, i;
    Requirement *requirements;
    uint64_t total_addresses = 0, capacity;

    char *slash = strchr(cidr, '/');
    if (slash == NULL || slash == cidr || slash[1] == '\0') {
        printf("Error: Invalid CIDR notation.\n");
        goto cleanup;
    }
    *slash = '\0';
    base_prefix = (unsigned int)strtoul(slash + 1, NULL, 10);
    if (base_prefix > 30 || !parse_ipv4(cidr, &address)) {
        printf("Error: Use a valid IPv4 network prefix from /0 through /30.\n");
        goto cleanup;
    }

    requirements = (Requirement *)calloc(subnet_count, sizeof(Requirement));
    if (requirements == NULL) {
        printf("Error: Memory allocation failed.\n");
        goto cleanup;
    }

    for (i = 0; i < subnet_count; ++i) {
        requirements[i].requested_hosts = requested_hosts[i];
        if (requirements[i].requested_hosts == 0) {
            printf("Error: Host count must be positive.\n");
            free(requirements);
            goto cleanup;
        }
        requirements[i].block_size = block_for_hosts(requirements[i].requested_hosts, &requirements[i].prefix);
        if (requirements[i].prefix < base_prefix) {
            printf("Error: Subnet %u is too large for the supplied network.\n", i + 1);
            free(requirements);
            goto cleanup;
        }
        total_addresses += requirements[i].block_size;
    }

    capacity = UINT64_C(1) << (32 - base_prefix);
    if (total_addresses > capacity) {
        printf("Error: Requested subnets require %llu addresses, but the network has %llu.\n",
               (unsigned long long)total_addresses, (unsigned long long)capacity);
        free(requirements);
        goto cleanup;
    }

    qsort(requirements, subnet_count, sizeof(Requirement), compare_requirements);
    base_mask = mask_for_prefix(base_prefix);
    network = address & base_mask;

    printf("\nVLSM Allocation (Largest Subnet First)\n\n");
    for (i = 0; i < subnet_count; ++i) {
        uint32_t first = network;
        uint32_t last;
        uint32_t mask = mask_for_prefix(requirements[i].prefix);
        uint32_t usable_first, usable_last;

        last = first + (uint32_t)requirements[i].block_size - 1;
        usable_first = first + 1;
        usable_last = last - 1;
        
        printf("Subnet %u (Requested usable hosts: %u)\n", i + 1, requirements[i].requested_hosts);
        printf("  First IP       : ");
        print_ipv4(first);
        printf("\n  Last IP        : ");
        print_ipv4(last);
        printf("\n  Usable IP range: ");
        print_ipv4(usable_first);
        printf(" - ");
        print_ipv4(usable_last);
        printf("\n  Subnet mask    : ");
        print_ipv4(mask);
        printf(" (/%u)\n\n", requirements[i].prefix);
        
        network = last + 1;
    }

    free(requirements);

cleanup:
    free(requested_hosts);
    printf("--- Finished Processing ---\n\n");
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

    printf("VLSM Server listening on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd >= 0) {
            handle_client(client_fd);
        }
    }
    return 0;
}