
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_BITS 100

int main()
{
    int serversocket, clientsocket;
    struct sockaddr_in serveraddress = {0};
    struct sockaddr_in clientaddress = {0};
    socklen_t addr_len;

    int codeword[MAX_BITS];
    int data[MAX_BITS];
    int m, r;

    /* SOCKET SETUP */
    serversocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serversocket < 0)
    {
        perror("Socket error");
        exit(1);
    }

    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(PORT);
    serveraddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serversocket, (struct sockaddr *)&serveraddress, sizeof(serveraddress)) < 0)
    {
        perror("Bind error");
        close(serversocket);
        exit(1);
    }

    listen(serversocket, 5);

    printf("Waiting for sender...\n");

    addr_len = sizeof(clientaddress);

    clientsocket = accept(serversocket, (struct sockaddr *)&clientaddress, &addr_len);

    if (clientsocket < 0)
    {
        perror("Accept error");
        close(serversocket);
        exit(1);
    }

    printf("Sender connected.\n\n");

    /* RECEIVE CODEWORD */
    recv(clientsocket, &m, sizeof(m), 0);
    recv(clientsocket, &r, sizeof(r), 0);
    recv(clientsocket, codeword, sizeof(int) * (m + r + 1), 0);

    printf("====================================\n");
    printf("       HAMMING CODE RECEIVER        \n");
    printf("====================================\n");

    printf("Received Codeword: ");

    for (int i = 1; i <= m + r; i++)
    {
        printf("%d ", codeword[i]);
    }

    printf("\n");

    /* ERROR DETECTION */
    int error_pos = 0;

    for (int i = 0; i < r; i++)
    {
        int pos = (1 << i);
        int parity = 0;

        for (int x = 1; x <= m + r; x++)
        {
            if ((x & pos) != 0)
            {
                parity ^= codeword[x];
            }
        }

        if (parity != 0)
        {
            error_pos += pos;
        }
    }

    /* ERROR CORRECTION */
    if (error_pos == 0)
    {
        printf("\nResult: No error detected in transmission.\n");
    }
    else
    {
        printf("\nResult: Error detected at position %d!\n", error_pos);

        if (error_pos <= m + r)
        {
            codeword[error_pos] ^= 1;

            printf("Correcting error...\n");
            printf("Corrected Codeword: ");

            for (int i = 1; i <= m + r; i++)
            {
                printf("%d ", codeword[i]);
            }

            printf("\n");
        }
        else
        {
            printf("Error position out of bounds (multiple bits may be corrupted).\n");
        }
    }

    /* EXTRACT ORIGINAL DATA */
    int j = 0;
    int k = 0;

    for (int i = 1; i <= m + r; i++)
    {
        if (i == (1 << k))
        {
            k++;
        }
        else
        {
            data[j++] = codeword[i];
        }
    }

    printf("\nExtracted Data Bits: ");

    for (int i = 0; i < m; i++)
    {
        printf("%d", data[i]);
    }

    printf("\n====================================\n");

    close(clientsocket);
    close(serversocket);

    return 0;
}
