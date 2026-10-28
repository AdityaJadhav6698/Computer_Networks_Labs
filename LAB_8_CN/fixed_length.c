#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

int main(void) {
    char input[32];
    uint32_t address, network, mask;
    unsigned int prefix, subnet_count, borrowed, new_prefix;
    uint64_t subnet_size, i;

    printf("FIXED-LENGTH SUBNETTING (FLSM)\n");
    printf("Enter network in CIDR notation (example: 192.168.1.0/24): ");
    if (scanf("%31s", input) != 1) return 1;

    {
        char *slash = NULL;
        slash = (char *)0;
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
        prefix = (unsigned int)strtoul(slash + 1, NULL, 10);
        if (prefix > 32 || !parse_ipv4(input, &address)) {
            printf("Invalid IPv4 address or prefix.\n");
            return 1;
        }
    }

    printf("Enter number of equal-sized subnets: ");
    if (scanf("%u", &subnet_count) != 1 || subnet_count == 0) {
        printf("Number of subnets must be positive.\n");
        return 1;
    }

    borrowed = bits_required(subnet_count);
    new_prefix = prefix + borrowed;
    if (new_prefix > 32) {
        printf("Not enough host bits for %u subnets in /%u.\n", subnet_count, prefix);
        return 1;
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
    return 0;
}
