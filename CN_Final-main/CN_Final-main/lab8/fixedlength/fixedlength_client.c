#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

struct flsm_request {
    char cidr[32];
    unsigned int subnet_count;
};

int main(void) {
    int sock;
    struct sockaddr_in server_addr;
    struct flsm_request req;
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
    printf("FIXED-LENGTH SUBNETTING (FLSM)\n");
    printf("Enter network in CIDR notation (example: 192.168.1.0/24): ");
    if (scanf("%31s", req.cidr) != 1) return 1;

    printf("Enter number of equal-sized subnets: ");
    if (scanf("%u", &req.subnet_count) != 1) return 1;

    /* Connect and Send */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    send(sock, &req, sizeof(req), 0);

    /* Receive Acknowledgment */
    recv(sock, &ack, sizeof(ack), 0);
    
    printf("\nData successfully sent to the server!\n");
    printf("Please check the server console for the subnetting output.\n");

    close(sock);
    return 0;
}