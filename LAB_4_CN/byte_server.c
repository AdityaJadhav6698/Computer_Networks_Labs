#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8082

#define FLAG '$'
#define ESC '#'

void destuff(char stuffed[], char data[])
{
    int i, j = 0;

    for(i = 0; stuffed[i] != '\0'; i++)
    {
        if(stuffed[i] == ESC)
        {
            i++;
        }

        data[j++] = stuffed[i];
    }

    data[j] = '\0';
}

int main()
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrSize = sizeof(clientAddr);

    char stuffed[2048];
    char data[1024];

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(serverSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if(bind(serverSocket,
            (struct sockaddr*)&serverAddr,
            sizeof(serverAddr)) < 0)
    {
        perror("Bind failed");
        return 1;
    }

    listen(serverSocket, 5);

    printf("Byte Stuffing Server waiting...\n");

    clientSocket = accept(serverSocket,
                           (struct sockaddr*)&clientAddr,
                           &addrSize);

    printf("Client connected.\n");

    recv(clientSocket,
         stuffed,
         sizeof(stuffed),
         0);

    printf("\nStuffed data received: %s\n",
           stuffed);

    destuff(stuffed, data);

    printf("Destuffed data: %s\n",
           data);

    close(clientSocket);
    close(serverSocket);

    return 0;
}