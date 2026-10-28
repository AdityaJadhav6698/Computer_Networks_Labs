// FHSS Server (receiver, simple)   compile: gcc -o server server.c   run FIRST: ./server
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// same fixed mapping as client
int FREQ[8] = {2400, 2410, 2420, 2430, 2440, 2450, 2460, 2470};

typedef struct
{
    int t, seq, freq;
} Packet;

int main()
{
    // wait for client
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(5000);
    a.sin_addr.s_addr = inet_addr("127.0.0.1");
    bind(srv, (struct sockaddr *)&a, sizeof a);
    listen(srv, 1);
    printf("Waiting for client...\n");
    int c = accept(srv, NULL, NULL);

    char data[100][4]; // data[seq] = 3-bit data
    int n = 0;         // total chunks (comes in the end marker)
    Packet p;

    // receive packets
    while (recv(c, &p, sizeof p, MSG_WAITALL) == sizeof p)
    {
        if (p.t == -1)
        {
            n = p.seq;
            break;
        } // end marker

        int v; // freq -> data
        for (v = 0; v < 8; v++)
            if (FREQ[v] == p.freq)
                break;
        if (v == 8 || p.seq < 0 || p.seq >= 100)
            continue; // bad packet

        data[p.seq][0] = ((v >> 2) & 1) + '0';
        data[p.seq][1] = ((v >> 1) & 1) + '0';
        data[p.seq][2] = (v & 1) + '0';
        data[p.seq][3] = '\0';
        printf("t%d: got seq=%d freq=%d -> %s\n", p.t, p.seq, p.freq, data[p.seq]);
    }

    // put chunks back in order using seq
    printf("\nOriginal data: ");
    for (int i = 0; i < n; i++)
        printf("%s", data[i]);
    printf("\n");

    close(c);
    close(srv);
    return 0;
}