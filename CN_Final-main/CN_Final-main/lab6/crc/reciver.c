
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define KEY "1011"
#define PORT 9000

int main()
{
    int receiversocket, clientsocket;
    struct sockaddr_in serveraddress = {0};

    receiversocket = socket(AF_INET, SOCK_STREAM, 0);

    if (receiversocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(PORT);
    serveraddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(receiversocket, (struct sockaddr *)&serveraddress, sizeof(serveraddress)) < 0)
    {
        perror("Binding failed");
        close(receiversocket);
        return 1;
    }

    printf("Binding successful\n");

    if (listen(receiversocket, 3) < 0)
    {
        perror("Listen failed");
        close(receiversocket);
        return 1;
    }

    printf("Waiting for sender connection...\n");

    clientsocket = accept(receiversocket, NULL, NULL);

    if (clientsocket < 0)
    {
        perror("Accept failed");
        close(receiversocket);
        return 1;
    }

    printf("Sender connected successfully.\n\n");

    /* RECEIVE CODEWORD */
    char received[150];

    int bytes = recv(clientsocket, received, sizeof(received) - 1, 0);

    if (bytes <= 0)
    {
        printf("Failed to receive data.\n");
        close(clientsocket);
        close(receiversocket);
        return 1;
    }

    received[bytes] = '\0';

    printf("--- CRC RECEIVER ---\n");
    printf("Generator (key): %s\n", KEY);
    printf("Received Codeword: %s\n", received);

    int received_len = strlen(received);
    int k = strlen(KEY);
    int i, j;

    /* CRC DIVISION */
    for (i = 0; i <= received_len - k; i++)
    {
        if (received[i] == '1')
        {
            for (j = 0; j < k; j++)
            {
                if (received[i + j] == KEY[j])
                    received[i + j] = '0';
                else
                    received[i + j] = '1';
            }
        }
    }

    /* CHECK REMAINDER */
    int error = 0;

    for (i = received_len - k + 1; i < received_len; i++)
    {
        if (received[i] == '1')
        {
            error = 1;
            break;
        }
    }

    /* DISPLAY RESULT */
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

    /* SEND RESULT TO CLIENT */
    send(clientsocket, resultMsg, strlen(resultMsg) + 1, 0);

    close(clientsocket);
    close(receiversocket);

    return 0;
}
