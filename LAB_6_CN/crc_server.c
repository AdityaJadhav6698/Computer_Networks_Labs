#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define KEY "1011"   // Generator polynomial — same in sender & receiver

int main()
{
    int receiverSocket, senderSocket;
    struct sockaddr_in receiverAddress;

    receiverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (receiverSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_port = htons(9000);
    receiverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(receiverSocket, (struct sockaddr *)&receiverAddress, sizeof(receiverAddress)) < 0)
    {
        perror("Binding failed");
        close(receiverSocket);
        return 1;
    }

    printf("Binding successful\n");

    if (listen(receiverSocket, 3) < 0)
    {
        perror("Listen failed");
        close(receiverSocket);
        return 1;
    }

    printf("Waiting for sender connection...\n");

    senderSocket = accept(receiverSocket, NULL, NULL);
    if (senderSocket < 0)
    {
        perror("Accept failed");
        close(receiverSocket);
        return 1;
    }

    printf("Sender connected successfully.\n\n");

    // Receive only the codeword — key is already known
    char received[150];
    int bytes = recv(senderSocket, received, sizeof(received), 0);
    if (bytes <= 0)
    {
        printf("Failed to receive data.\n");
        close(senderSocket);
        close(receiverSocket);
        return 1;
    }

    printf("--- CRC RECEIVER ---\n");
    printf("Generator (key):  %s\n", KEY);
    printf("Received Codeword: %s\n", received);

    int received_len = strlen(received);
    int k = strlen(KEY);
    int i, j;

    // CRC division at receiver
    for (i = 0; i <= received_len - k; i++)
    {
        if (received[i] == '1')
        {
            for (j = 0; j < k; j++)
            {
                received[i + j] = (received[i + j] == KEY[j]) ? '0' : '1';
            }
        }
    }

    // Check remainder — all zeros means no error
    int error = 0;
    for (i = received_len - k + 1; i < received_len; i++)
    {
        if (received[i] == '1')
        {
            error = 1;
            break;
        }
    }

    char resultMsg[50];
    if (error == 0)
    {
        printf("Result: No Error Detected\n\n");
        strcpy(resultMsg, "No Error Detected");
    }
    else
    {
        printf("Result: Error Detected\n\n");
        strcpy(resultMsg, "Error Detected");
    }

    send(senderSocket, resultMsg, strlen(resultMsg) + 1, 0);

    close(senderSocket);
    close(receiverSocket);
    return 0;
}