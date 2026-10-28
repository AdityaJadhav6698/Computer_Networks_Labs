#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

int main()
{
    int sock;
    struct sockaddr_in receiver,transmitter;
    int packet;
    int max_packets;
    int tt,tp;
    int expected = 0;

    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(9000);
    receiver.sin_addr.s_addr = INADDR_ANY;

    sock = socket(AF_INET,SOCK_DGRAM,0);
    if(sock < 0) { perror("Socket creation failed"); return 1; }

    bind(sock,(struct sockaddr *)&receiver,sizeof(receiver));

    socklen_t len = sizeof(transmitter);

    recvfrom(sock,&max_packets,sizeof(max_packets),0,(struct sockaddr *)&transmitter,&len);
    recvfrom(sock,&tt,sizeof(tt),0,(struct sockaddr *)&transmitter,&len);
    recvfrom(sock,&tp,sizeof(tp),0,(struct sockaddr *)&transmitter,&len);

    printf("Receiver Started\n");
    printf("Receiver Window Size: 1\n\n");

    while(expected < max_packets)
    {
        recvfrom(sock,&packet,sizeof(packet),0,(struct sockaddr *)&transmitter,&len);

        if(packet == expected)
        {
            printf("Packet %d received\n",packet);
            sendto(sock,&packet,sizeof(packet),0,(struct sockaddr *)&transmitter,len);
            printf("ACK %d sent\n\n",packet);
            expected++;
        }
        else
        {
            printf("Packet %d rejected (Expected %d)\n\n",packet,expected);
        }
    }

    printf("All packets received successfully.\n");
    close(sock);
    return 0;
}