#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

struct Data {
    int num1;
    int num2;
    char op;
};

int main() {

    int clientSocket;
    struct sockaddr_in serverAddr;
    socklen_t len = sizeof(serverAddr);

    struct Data d;
    int result;
    int choice;

    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    do {
        printf("\n--- Calculator Menu ---\n");
        printf("1. Addition\n");
        printf("2. Subtraction\n");
        printf("3. Multiplication\n");
        printf("4. Division\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                d.op = '+';
                break;
            case 2:
                d.op = '-';
                break;
            case 3:
                d.op = '*';
                break;
            case 4:
                d.op = '/';
                break;
            case 5:
                printf("Exiting calculator.\n");
                continue;
            default:
                printf("Invalid choice. Please try again.\n");
                continue;
        }

        printf("Enter first number: ");
        scanf("%d", &d.num1);

        printf("Enter second number: ");
        scanf("%d", &d.num2);

        sendto(clientSocket, &d, sizeof(d), 0,
               (struct sockaddr*)&serverAddr, len);

        recvfrom(clientSocket, &result, sizeof(result), 0,
                 (struct sockaddr*)&serverAddr, &len);

        printf("Result = %d\n", result);
    } while (choice != 5);

    close(clientSocket);
    return 0;
}