#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<math.h>

int min(int x,int y)
{
    return (x<y ? x : y);
}

typedef struct Node
{
    int data;
    int cycle;
    int relseq;
    struct Node *next;
} node;

typedef struct Queue
{
    node *front;
    node *rear;
} queue;

int isEmpty(queue *q) { return q->rear == NULL; }

queue *createQueue()
{
    queue *q = (queue *)malloc(sizeof(queue));
    q->front = NULL;
    q->rear = NULL;
    return q;
}

node *createNode(int data,int cycle,int relseq)
{
    node *newnode = (node *)malloc(sizeof(node));
    newnode->data = data;
    newnode->cycle = cycle;
    newnode->relseq = relseq;
    newnode->next = NULL;
    return newnode;
}

void Enqueue(queue *q,int data,int cycle,int relseq)
{
    node *newnode = createNode(data,cycle,relseq);
    if(isEmpty(q)) { q->front = newnode; q->rear = newnode; }
    else { q->rear->next = newnode; q->rear = newnode; }
}

int Dequeue(queue *q)
{
    if(isEmpty(q)) return -1;
    node *temp = q->front;
    q->front = q->front->next;
    if(q->front == NULL) q->rear = NULL;
    int data = temp->data;
    free(temp);
    return data;
}

void printQueue(queue *q)
{
    node *temp = q->front;
    printf("Window: ");
    while(temp != NULL)
    {
        printf("%d(C%d:%d) ",temp->data,temp->cycle,temp->relseq);
        temp = temp->next;
    }
    printf("\n");
}

int main()
{
    int sock;
    struct sockaddr_in receiver;
    int tp,tt,total_time=0;

    printf("Enter Transmission Time: ");
    scanf("%d",&tt);
    printf("Enter Propagation Time: ");
    scanf("%d",&tp);

    int a = tp / tt;
    int max_packets = 1 + 2 * a;
    int no_of_sequence_bits = (int)floor(log2(max_packets));
    int window_size = min((int)pow(2,no_of_sequence_bits), max_packets);

    printf("\nMaximum number of Packets: %d\n",max_packets);
    printf("Number of bits in sequence number: %d\n",no_of_sequence_bits);
    printf("Window Size: %d\n\n",window_size);

    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(9000);
    receiver.sin_addr.s_addr = inet_addr("127.0.0.1");

    sock = socket(AF_INET,SOCK_DGRAM,0);
    if(sock < 0) { perror("Socket creation failed"); return 1; }

    sendto(sock,&max_packets,sizeof(max_packets),0,(struct sockaddr *)&receiver,sizeof(receiver));
    sendto(sock,&tt,sizeof(tt),0,(struct sockaddr *)&receiver,sizeof(receiver));
    sendto(sock,&tp,sizeof(tp),0,(struct sockaddr *)&receiver,sizeof(receiver));

    queue *window = createQueue();
    int *ack_time = (int *)malloc(sizeof(int) * max_packets);

    int next_seq      = 0;
    int transmitted    = 0;
    int acknowledged   = 0;
    int outstanding    = max_packets;
    int cycle          = 1;
    int c1count        = 0;
    int c2count        = 0;

    while(acknowledged < max_packets)
    {
        while(transmitted < window_size && outstanding > 0)
        {
            int finish;
            int relseq;

            if(acknowledged == 0)
            {
                sleep(tt);
                total_time += tt;
                finish = total_time;
                cycle  = 1;
                relseq = c1count++;
            }
            else
            {
                sleep(tt);
                total_time += tt;       // FIX: update simulated time
                finish = total_time;
                cycle  = 2;
                relseq = c2count++;
            }

            ack_time[next_seq] = finish + tt + 2*tp; // FIX: complete ACK timing

            sendto(sock,&next_seq,sizeof(next_seq),0,
                   (struct sockaddr *)&receiver,sizeof(receiver));

            Enqueue(window,next_seq,cycle,relseq);
            transmitted++;
            outstanding--;

            printf("Timer: %d\tCycle: %d\tTransmitted: %d\tAcknowledged: %d\tOutstanding: %d\tSent packet %d\n",
                   total_time,cycle,transmitted,acknowledged,outstanding,next_seq);
            printQueue(window);

            next_seq++;
        }

        if(acknowledged >= max_packets) break;

        int head_seq = window->front->data;
        int t_ack    = ack_time[head_seq];

        if(t_ack > total_time)
        {
            sleep(t_ack - total_time);
            total_time = t_ack;
        }

        int ack;
        socklen_t rlen = sizeof(receiver);
        recvfrom(sock,&ack,sizeof(ack),0,(struct sockaddr *)&receiver,&rlen);

        Dequeue(window);
        transmitted--;
        acknowledged++;

        printf("Timer: %d\tCycle: %d\tTransmitted: %d\tAcknowledged: %d\tOutstanding: %d\tACK %d received\n",
               total_time,cycle,transmitted,acknowledged,outstanding,ack);
        printQueue(window);
    }

    printf("\nAll packets transmitted successfully.\n");
    printf("Total time required: %d\n",total_time);

    free(ack_time);
    close(sock);
    return 0;
}