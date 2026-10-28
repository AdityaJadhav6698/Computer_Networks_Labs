#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main()
{
    int sock;
    struct sockaddr_in server_addr;

    int window_size;
    int total_packets;
    int loss_n;

    int total_transmissions = 0;
    int total_retransmissions = 0;

    /*
        base = first unacknowledged packet
    */
    int base = 1;

    /*
        next_new = next packet which has
        never been transmitted before
    */
    int next_new = 1;

    /*
        The packet where the next loss occurs.

        Example:
        n = 5
        first loss = 5
    */
    int lost_packet;

    /* Create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket error");
        exit(1);
    }

    /* Server address */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr =
        inet_addr("127.0.0.1");

    /* Connect */
    if (connect(sock,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("Connection error");
        exit(1);
    }

    /* INPUT */

    printf("Enter GBN window size: ");
    scanf("%d", &window_size);

    printf("Enter number of packets: ");
    scanf("%d", &total_packets);

    printf("Enter every nth packet to be lost: ");
    scanf("%d", &loss_n);

    /*
        First lost packet
    */
    lost_packet = loss_n;

    /*
        Send inputs to server
    */

    send(sock, &window_size,
         sizeof(window_size), 0);

    send(sock, &total_packets,
         sizeof(total_packets), 0);

    send(sock, &loss_n,
         sizeof(loss_n), 0);

    printf("\n====================================\n");
    printf("             GBN ARQ\n");
    printf("====================================\n");

    printf("Window size       : %d\n", window_size);
    printf("Total packets     : %d\n", total_packets);
    printf("Every nth lost    : %d\n", loss_n);

    /*
        --------------------------------------------------
        INITIAL TRANSMISSION
        --------------------------------------------------

        Before the timeout of the lost packet occurs,
        ACKs for packets before it allow the window
        to slide.

        Therefore, if packet 'loss_n' is lost, packets
        up to:

            loss_n + window_size - 1

        can already have been transmitted.
    */

    int last_sent =
        loss_n + window_size - 1;

    if (last_sent > total_packets)
        last_sent = total_packets;

    printf("\nInitial transmission:\n");

    for (int i = 1; i <= last_sent; i++)
    {
        printf("%d ", i);

        send(sock, &i, sizeof(i), 0);

        int type = 1;       // new transmission

        send(sock, &type, sizeof(type), 0);

        total_transmissions++;
    }

    printf("\n");

    /*
        --------------------------------------------------
        GBN RETRANSMISSION CYCLES
        --------------------------------------------------
    */

    while (lost_packet <= total_packets)
    {
        /*
            The current lost packet may be beyond
            the packets that have already been sent.

            If so, there is no more loss.
        */

        if (lost_packet > last_sent)
            break;

        printf("\nPacket %d is LOST\n",
               lost_packet);

        /*
            GBN retransmits from the lost packet
            to the LAST PACKET ALREADY TRANSMITTED.
        */

        printf("Retransmitting: ");

        for (int i = lost_packet;
             i <= last_sent;
             i++)
        {
            printf("%d ", i);

            send(sock, &i, sizeof(i), 0);

            int type = 2;       // retransmission

            send(sock, &type, sizeof(type), 0);

            total_transmissions++;
            total_retransmissions++;
        }

        printf("\n");

        /*
            ------------------------------------------------
            WINDOW SLIDES
            ------------------------------------------------

            After the retransmission, ACKs arrive and
            the sender can transmit new packets.

            The next loss is:

                lost_packet + window_size - 1
        */

        int next_loss =
            lost_packet + window_size - 1;

        /*
            New packets can now be sent until the
            next lost packet + window_size - 1.

            Example:

            loss = 5
            W = 5

            next loss = 9

            New packets:
            10 11 12 13
        */

        int new_last =
            next_loss + window_size - 1;

        if (new_last > total_packets)
            new_last = total_packets;

        /*
            Send only packets which are NEW.
        */

        if (last_sent < new_last)
        {
            printf("New packets sent: ");

            for (int i = last_sent + 1;
                 i <= new_last;
                 i++)
            {
                printf("%d ", i);

                send(sock, &i, sizeof(i), 0);

                int type = 1;

                send(sock, &type, sizeof(type), 0);

                total_transmissions++;
            }

            printf("\n");
        }

        /*
            Update last packet sent.
        */

        last_sent = new_last;

        /*
            Move to next loss.
        */

        lost_packet = next_loss;
    }

    /*
        --------------------------------------------------
        FINAL RETRANSMISSION
        --------------------------------------------------

        When the last loss occurs, retransmit from the
        lost packet to the final packet.
    */

    if (lost_packet <= total_packets)
    {
        printf("\nPacket %d is LOST\n",
               lost_packet);

        printf("Final retransmission: ");

        for (int i = lost_packet;
             i <= total_packets;
             i++)
        {
            printf("%d ", i);

            send(sock, &i, sizeof(i), 0);

            int type = 2;

            send(sock, &type, sizeof(type), 0);

            total_transmissions++;
            total_retransmissions++;
        }

        printf("\n");
    }

    /*
        Send completion signal
    */

    int end_signal = -1;

    send(sock,
         &end_signal,
         sizeof(end_signal), 0);

    /*
        FINAL RESULT
    */

    printf("\n====================================\n");
    printf("              RESULT\n");
    printf("====================================\n");

    printf("Window size                  : %d\n",
           window_size);

    printf("Total packets                : %d\n",
           total_packets);

    printf("Every nth packet lost        : %d\n",
           loss_n);

    printf("Total transmissions required : %d\n",
           total_transmissions);

    printf("Total retransmissions        : %d\n",
           total_retransmissions);

    printf("====================================\n");

    close(sock);

    return 0;
}