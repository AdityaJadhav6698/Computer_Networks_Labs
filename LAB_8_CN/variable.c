#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

int main(void) {
    char input[32];
    uint32_t address, network, base_mask;
    unsigned int base_prefix, subnet_count, i;
    Requirement *requirements;
    uint64_t total_addresses = 0, capacity;

    printf("VARIABLE-LENGTH SUBNETTING (VLSM)\n");
    printf("Enter network in CIDR notation (example: 192.168.1.0/24): ");
    if (scanf("%31s", input) != 1) return 1;
    {
        char *slash = NULL;
        for (char *p = input; *p; ++p) {
            if (*p == '/') {
                slash = p;
                break;
            }
        }
        if (slash == NULL || slash == input || slash[1] == '\0') {
            printf("Invalid CIDR notation.\n");
            return 1;
        }
        *slash = '\0';
        base_prefix = (unsigned int)strtoul(slash + 1, NULL, 10);
        if (base_prefix > 30 || !parse_ipv4(input, &address)) {
            printf("Use a valid IPv4 network prefix from /0 through /30.\n");
            return 1;
        }
    }

    printf("Enter number of variable-sized subnets: ");
    if (scanf("%u", &subnet_count) != 1 || subnet_count == 0) {
        printf("Number of subnets must be positive.\n");
        return 1;
    }

    requirements = (Requirement *)calloc(subnet_count, sizeof(Requirement));
    if (requirements == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }
    for (i = 0; i < subnet_count; ++i) {
        printf("Hosts required in subnet %u (usable hosts): ", i + 1);
        if (scanf("%u", &requirements[i].requested_hosts) != 1 ||
            requirements[i].requested_hosts == 0) {
            printf("Host count must be positive.\n");
            free(requirements);
            return 1;
        }
        requirements[i].block_size =
            block_for_hosts(requirements[i].requested_hosts, &requirements[i].prefix);
        if (requirements[i].prefix < base_prefix) {
            printf("Subnet %u is too large for the supplied network.\n", i + 1);
            free(requirements);
            return 1;
        }
        total_addresses += requirements[i].block_size;
    }

    capacity = UINT64_C(1) << (32 - base_prefix);
    if (total_addresses > capacity) {
        printf("Requested subnets require %llu addresses, but the network has %llu.\n",
               (unsigned long long)total_addresses, (unsigned long long)capacity);
        free(requirements);
        return 1;
    }

    qsort(requirements, subnet_count, sizeof(Requirement), compare_requirements);
    base_mask = mask_for_prefix(base_prefix);
    network = address & base_mask;

    printf("\nVLSM allocation (largest subnet first)\n\n");
    for (i = 0; i < subnet_count; ++i) {
        uint32_t first = network;
        uint32_t last;
        uint32_t mask = mask_for_prefix(requirements[i].prefix);
        uint32_t usable_first, usable_last;

        last = first + (uint32_t)requirements[i].block_size - 1;
        usable_first = first + 1;
        usable_last = last - 1;
        printf("Subnet %u (requested usable hosts: %u)\n", i + 1,
               requirements[i].requested_hosts);
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
    return 0;
}
