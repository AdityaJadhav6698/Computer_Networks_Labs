#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main(void) {
    int sock;
    struct sockaddr_in server_addr;
    char cidr[32];
    unsigned int subnet_count, i;
    unsigned int *requested_hosts;
    char ack;

    /* Socket Creation */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* User Input */
    printf("VARIABLE-LENGTH SUBNETTING (VLSM)\n");
    printf("Enter network in CIDR notation (example: 192.168.1.0/24): ");
    if (scanf("%31s", cidr) != 1) return 1;

    printf("Enter number of variable-sized subnets: ");
    if (scanf("%u", &subnet_count) != 1 || subnet_count == 0) {
        printf("Number of subnets must be positive.\n");
        return 1;
    }

    requested_hosts = (unsigned int *)malloc(subnet_count * sizeof(unsigned int));
    if (!requested_hosts) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    for (i = 0; i < subnet_count; ++i) {
        printf("Hosts required in subnet %u (usable hosts): ", i + 1);
        if (scanf("%u", &requested_hosts[i]) != 1 || requested_hosts[i] == 0) {
            printf("Host count must be positive.\n");
            free(requested_hosts);
            return 1;
        }
    }

    /* Connect and Send */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        free(requested_hosts);
        return 1;
    }

    send(sock, cidr, sizeof(cidr), 0);
    send(sock, &subnet_count, sizeof(subnet_count), 0);
    send(sock, requested_hosts, subnet_count * sizeof(unsigned int), 0);

    /* Wait for server acknowledgment */
    recv(sock, &ack, sizeof(ack), 0);
    
    printf("\nData successfully sent to the server!\n");
    printf("Please check the server console for the VLSM allocation output.\n");

    free(requested_hosts);
    close(sock);
    return 0;
}