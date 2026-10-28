#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

struct Data {
    int num1;
    int num2;
    char op;
};

int main() {
    int serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t len = sizeof(clientAddr);

    struct Data d;
    int result;

    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    printf("Server is running...\n");

    while (1) {

        recvfrom(serverSocket, &d, sizeof(d), 0,
                 (struct sockaddr*)&clientAddr, &len);

        switch(d.op) {
            case '+':
                result = d.num1 + d.num2;
                break;

            case '-':
                result = d.num1 - d.num2;
                break;

            case '*':
                result = d.num1 * d.num2;
                break;

            case '/':
                if(d.num2 != 0)
                    result = d.num1 / d.num2;
                else
                    result = 0;
                break;

            default:
                result = -1;
        }

        sendto(serverSocket, &result, sizeof(result), 0,
               (struct sockaddr*)&clientAddr, len);
    }

    close(serverSocket);
    return 0;
}