#include <stdio.h>

int main(void)
{
    int octet[4];
    int first;
    char class_name;
    char nid[16] = "N/A";
    char hid[16] = "N/A";
    const char *network_count = "N/A";
    const char *host_count = "N/A";
    char lba[16] = "N/A";
    char dba[16] = "N/A";
    char subnet_mask[16] = "N/A";
    int mask[4] = {0, 0, 0, 0};

    printf("Enter an IPv4 address (e.g., 192.168.1.1): ");
    if (scanf("%d.%d.%d.%d", &octet[0], &octet[1], &octet[2], &octet[3]) != 4) {
        printf("Invalid IP address format.\n");
        return 1;
    }

    for (int i = 0; i < 4; i++) {
        if (octet[i] < 0 || octet[i] > 255) {
            printf("Invalid IP address. Each octet must be between 0 and 255.\n");
            return 1;
        }
    }

    first = octet[0];

    if (first == 0) {
        class_name = '-';
    } else if (first <= 127) {
        class_name = 'A';
        mask[0] = 255;
        network_count = "126";
        host_count = "16,777,214";
    } else if (first <= 191) {
        class_name = 'B';
        mask[0] = 255;
        mask[1] = 255;
        network_count = "16,384";
        host_count = "65,534";
    } else if (first <= 223) {
        class_name = 'C';
        mask[0] = 255;
        mask[1] = 255;
        mask[2] = 255;
        network_count = "2,097,152";
        host_count = "254";
    } else if (first <= 239) {
        class_name = 'D';
    } else {
        class_name = 'E';
    }

    if (class_name == 'A' || class_name == 'B' || class_name == 'C') {
        int network[4];
        int host[4];
        int broadcast[4];
        int first_usable[4];
        int last_usable[4];

        for (int i = 0; i < 4; ++i) {
            network[i] = octet[i] & mask[i];
            host[i] = octet[i] & (~mask[i] & 255);
            broadcast[i] = network[i] | (~mask[i] & 255);
            first_usable[i] = network[i];
            last_usable[i] = broadcast[i];
        }
        first_usable[3]++;
        last_usable[3]--;

        snprintf(nid, sizeof(nid), "%d.%d.%d.%d",
                 network[0], network[1], network[2], network[3]);
        snprintf(hid, sizeof(hid), "%d.%d.%d.%d",
                 host[0], host[1], host[2], host[3]);
        snprintf(lba, sizeof(lba), "%d.%d.%d.%d",
                 first_usable[0], first_usable[1],
                 first_usable[2], first_usable[3]);
        snprintf(dba, sizeof(dba), "%d.%d.%d.%d",
                 broadcast[0], broadcast[1], broadcast[2], broadcast[3]);
        snprintf(subnet_mask, sizeof(subnet_mask), "%d.%d.%d.%d",
                 mask[0], mask[1], mask[2], mask[3]);
    }

    printf("\n+-------+----------------+----------------+----------------+----------------+----------------------+-----------------+-------------------+\n");
    printf("| Class | NID            | HID            | No. of Network | No. of Host    | LBA                  | DBA             | IP Type           |\n");
    printf("+-------+----------------+----------------+----------------+----------------+----------------------+-----------------+-------------------+\n");
    printf("| %-5c | %-14s | %-14s | %-14s | %-14s | %-20s | %-15s | %-17s |\n",
           class_name, nid, hid, network_count, host_count, lba, dba,
           first >= 224 && first <= 239 ? "Multicast" :
           first >= 240 ? "Reserved" :
           first == 127 ? "Loopback" :
           first == 0 && octet[1] == 0 && octet[2] == 0 && octet[3] == 0 ? "Unspecified" :
           first == 10 || (first == 172 && octet[1] >= 16 && octet[1] <= 31) ||
           (first == 192 && octet[1] == 168) ? "Private" : "Public");
    printf("+-------+----------------+----------------+----------------+----------------+----------------------+-----------------+-------------------+\n");
    printf("Classful subnet mask: %s\n", subnet_mask);

    return 0;
}