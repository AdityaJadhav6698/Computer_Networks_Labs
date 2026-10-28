// FHSS Client (sender, simple)   compile: gcc -o client client.c   run: ./client
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// fixed mapping: index = 3-bit value (000..111), entry = freq
int FREQ[8] = {2400, 2410, 2420, 2430, 2440, 2450, 2460, 2470};

typedef struct
{
    int t, seq, freq;
} Packet; // t = time slot

int main()
{
    char bits[300];
    printf("Enter bitstream (multiple of 3 bits): ");
    scanf("%299s", bits);

    int len = strlen(bits);
    if (len == 0 || len % 3 != 0)
    {
        printf("Length must be a multiple of 3\n");
        return 1;
    }
    for (int i = 0; i < len; i++)
        if (bits[i] != '0' && bits[i] != '1')
        {
            printf("Only 0 and 1 allowed\n");
            return 1;
        }

    // 1. make the 2 arrays: data[] and freq[]
    int n = len / 3;
    char data[100][4];
    int val[100], freq[100], sent[100] = {0};
    for (int i = 0; i < n; i++)
    {
        strncpy(data[i], bits + i * 3, 3);
        data[i][3] = '\0';
        val[i] = (bits[i * 3] - '0') * 4 + (bits[i * 3 + 1] - '0') * 2 + (bits[i * 3 + 2] - '0');
        freq[i] = FREQ[val[i]]; // data -> freq (fixed mapping)
    }
    printf("data: ");
    for (int i = 0; i < n; i++)
        printf("%s ", data[i]);
    printf("\nfreq: ");
    for (int i = 0; i < n; i++)
        printf("%d ", freq[i]);
    printf("\n\n");

    // 2. connect to server
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(5000);
    a.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(s, (struct sockaddr *)&a, sizeof a) < 0)
    {
        perror("connect");
        return 1;
    }

    // 3. send slot by slot: same freq twice in a slot -> second one goes to next slot
    int left = n;
    for (int t = 0; left > 0; t++)
    {
        int used[8] = {0}; // channels already used in this slot
        for (int i = 0; i < n; i++)
        {
            if (sent[i] || used[val[i]])
                continue; // already sent / freq busy
            used[val[i]] = 1;
            sent[i] = 1;
            left--;
            Packet p = {t, i, freq[i]}; // send seq + freq (not the data)
            send(s, &p, sizeof p, 0);
            printf("t%d: sent seq=%d freq=%d\n", t, i, freq[i]);
        }
        sleep(1);
    }

    Packet end = {-1, n, 0}; // end marker (t = -1, seq = total chunks)
    send(s, &end, sizeof end, 0);
    close(s);
    return 0;
}