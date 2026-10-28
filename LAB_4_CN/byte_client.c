#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8082

#define FLAG '$'
#define ESC '#'

void stuff(char data[], char stuffed[])
{
    int i, j = 0;

    for(i = 0; data[i] != '\0'; i++)
    {
        if(data[i] == FLAG || data[i] == ESC)
        {
            stuffed[j++] = ESC;
        }

        stuffed[j++] = data[i];
    }

    stuffed[j] = '\0';
}

int main()
{
    int clientSocket;
    struct sockaddr_in serverAddr;

    char data[1024];
    char stuffed[2048];

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(clientSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(clientSocket,
               (struct sockaddr*)&serverAddr,
               sizeof(serverAddr)) < 0)
    {
        perror("Connection failed");
        return 1;
    }

    printf("Connected to server.\n");

    printf("Enter data: ");
    scanf("%s", data);

    stuff(data, stuffed);

    printf("\n--- Sender Side ---\n");
    printf("Original : %s\n", data);

    printf("Stuffed  : %c%s%c\n",
           FLAG, stuffed, FLAG);

    send(clientSocket,
         stuffed,
         strlen(stuffed) + 1,
         0);

    printf("Data sent to server.\n");

    close(clientSocket);

    return 0;
}