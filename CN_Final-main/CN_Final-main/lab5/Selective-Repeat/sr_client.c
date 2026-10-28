#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define MAX_EVENTS 200000
#define MAX_PACKETS 1000

#define RX_EVENT 1
#define ACK_EVENT 2
#define TIMEOUT_EVENT 3

typedef struct {
    int window, total;
    int lost_count;
    int lost_packets[MAX_PACKETS];
    double tp, tt;
} Setup;

typedef struct {
    int packet, type; // 1 = new, 2 = retransmission
    double send_start;
} Packet;

typedef struct {
    int ack; // -2 means packet dropped by server
} Response;

typedef struct {
    double time;
    int type, packet, token;
} Event;

int recv_all(int fd, void *buf, size_t len) {
    char *p = buf;
    size_t done = 0;
    while (done < len) {
        ssize_t n = recv(fd, p + done, len - done, 0);
        if (n <= 0) return 0;
        done += n;
    }
    return 1;
}

int send_all(int fd, const void *buf, size_t len) {
    const char *p = buf;
    size_t done = 0;
    while (done < len) {
        ssize_t n = send(fd, p + done, len - done, 0);
        if (n <= 0) return 0;
        done += n;
    }
    return 1;
}

Event events[MAX_EVENTS];
int event_count = 0;

double now = 0;
double tp, tt, timeout;
double next_tx_time = 0;

int sock;
int base = 1, next_to_send = 1, highest_sent = 0;
int total, window, lost_count;
int lost_array[MAX_PACKETS];

int transmissions = 0, retransmissions = 0;

/* Selective Repeat requires independent state for each packet */
int acked[MAX_PACKETS] = {0};
int timer_generation[MAX_PACKETS] = {0};
double last_send_end[MAX_PACKETS] = {0};

int priority(int type) {
    if (type == RX_EVENT) return 0;
    if (type == ACK_EVENT) return 1;
    return 2;
}

int push_event(double time, int type, int packet, int token) {
    if (event_count >= MAX_EVENTS) {
        fprintf(stderr, "Event queue full.\n");
        return 0;
    }
    Event e = {time, type, packet, token};
    int i = event_count++;
    
    while (i > 0) {
        Event prev = events[i - 1];
        if (prev.time < e.time || (prev.time == e.time && priority(prev.type) <= priority(e.type)))
            break;
        events[i] = events[i - 1];
        i--;
    }
    events[i] = e;
    return 1;
}

Event pop_event(void) {
    Event e = events[0];
    for (int i = 1; i < event_count; i++)
        events[i - 1] = events[i];
    event_count--;
    return e;
}

void schedule_timer(int packet) {
    int token = ++timer_generation[packet];
    double deadline = last_send_end[packet] + timeout;
    if (deadline < now) deadline = now;
    push_event(deadline, TIMEOUT_EVENT, packet, token);
}

int transmit_packet(int packet, int type) {
    double start = fmax(now, next_tx_time);
    double end = start + tt;

    Packet p = {packet, type, start};

    printf("Sender: %s Packet %d, start=%.3f, end=%.3f\n",
           type == 1 ? "sending" : "retransmitting", packet, start, end);

    if (!send_all(sock, &p, sizeof(p))) return 0;
    
    /* Schedule the exact arrival time at the server */
    if (!push_event(end + tp, RX_EVENT, packet, 0)) return 0;

    next_tx_time = end;
    last_send_end[packet] = end;

    if (packet > highest_sent) highest_sent = packet;
    transmissions++;
    
    if (type == 2) retransmissions++;

    /* In Selective Repeat, every sent packet gets its own timer */
    schedule_timer(packet);
    
    return 1;
}

int fill_window(void) {
    while (next_to_send <= total && next_to_send < base + window) {
        if (!transmit_packet(next_to_send, 1)) return 0;
        next_to_send++;
    }
    return 1;
}

int main(void) {
    printf("Enter propagation time (tp): ");
    if (scanf("%lf", &tp) != 1) return 1;

    printf("Enter transmission time (tt): ");
    if (scanf("%lf", &tt) != 1) return 1;

    printf("Enter total packets: ");
    if (scanf("%d", &total) != 1) return 1;

    printf("Enter number of lost packets: ");
    if (scanf("%d", &lost_count) != 1) return 1;

    if (lost_count > 0) {
        printf("Enter lost packet numbers (space separated): ");
        for (int i = 0; i < lost_count; i++) {
            if (scanf("%d", &lost_array[i]) != 1) return 1;
        }
    }

    if (tp < 0 || tt <= 0 || total < 1 || total >= MAX_PACKETS) {
        printf("Invalid input.\n");
        return 1;
    }

    /* Optimal Window Size Calculation */
    window = (int)ceil(1.0 + 2.0 * tp / tt);
    if (window < 1) window = 1;

    timeout = 2.0 * tp + tt;

    printf("\nWindow size = ceil(1 + 2*tp/tt) = %d\n", window);
    printf("RTT = %.3f\n", 2.0 * tp);
    printf("Timeout = 2*tp + tt = %.3f\n\n", timeout);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); close(sock); return 1;
    }

    Setup s;
    s.window = window;
    s.total = total;
    s.lost_count = lost_count;
    s.tp = tp;
    s.tt = tt;
    for (int i = 0; i < lost_count; i++) s.lost_packets[i] = lost_array[i];

    if (!send_all(sock, &s, sizeof(s))) { close(sock); return 1; }
    if (!fill_window()) { close(sock); return 1; }

    while (base <= total && event_count > 0) {
        Event e = pop_event();
        now = e.time;

        if (e.type == RX_EVENT) {
            Response r;
            if (!recv_all(sock, &r, sizeof(r))) { close(sock); return 1; }
            
            // Server drops the packet if ack is -2. Otherwise schedule ACK return
            if (r.ack != -2) {
                push_event(now + tp, ACK_EVENT, r.ack, 0);
            }
        }

        if (e.type == ACK_EVENT) {
            printf("Sender: ACK %d arrives at t=%.3f\n", e.packet, now);
            
            if (e.packet >= base && e.packet <= total && !acked[e.packet]) {
                acked[e.packet] = 1;
                
                // If base is acked, slide window forward over all continuous acked packets
                if (e.packet == base) {
                    while (base <= total && acked[base]) {
                        base++;
                    }
                    printf("Sender: Window slides; new base=%d\n\n", base);
                    if (!fill_window()) { close(sock); return 1; }
                } else {
                    printf("Sender: Buffered out-of-order ACK %d\n\n", e.packet);
                }
            } else {
                printf("Sender: Duplicate/stale ACK %d ignored\n\n", e.packet);
            }
        }

        if (e.type == TIMEOUT_EVENT) {
            // Check if timer is still valid (packet not acked, and timer wasn't superseded)
            if (e.token == timer_generation[e.packet] && !acked[e.packet]) {
                printf("Sender: TIMEOUT for Packet %d at t=%.3f\n", e.packet, now);
                printf("Sender: Selective Repeat retransmits ONLY %d\n\n", e.packet);
                if (!transmit_packet(e.packet, 2)) { close(sock); return 1; }
            }
        }
    }

    Packet end = {-1, 0, 0};
    send_all(sock, &end, sizeof(end));

    printf("\n========== FINAL RESULT ==========\n");
    printf("Window size          : %d\n", window);
    printf("Total packets        : %d\n", total);
    printf("Lost packets array   : ");
    for(int i=0; i<lost_count; i++) printf("%d ", lost_array[i]);
    printf("\nTotal transmissions  : %d\n", transmissions);
    printf("Total retransmissions: %d\n", retransmissions);
    printf("Total logical time   : %.3f ms\n", now);
    printf("==================================\n");

    close(sock);
    return 0;
}