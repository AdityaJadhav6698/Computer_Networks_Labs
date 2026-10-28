
#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

void analyse_packet(unsigned char *args,
                    const struct pcap_pkthdr *header,
                    const unsigned char *packet)
{
    int length = header->caplen;

    if (length < (int)sizeof(struct ether_header))
        return;

    struct ether_header *eth;
    eth = (struct ether_header *)packet;

    printf("\n========== PACKET ==========\n");

    printf("\n--- Ethernet Header ---\n");

    printf("Source MAC      : %02x:%02x:%02x:%02x:%02x:%02x\n",
           eth->ether_shost[0], eth->ether_shost[1],
           eth->ether_shost[2], eth->ether_shost[3],
           eth->ether_shost[4], eth->ether_shost[5]);

    printf("Destination MAC : %02x:%02x:%02x:%02x:%02x:%02x\n",
           eth->ether_dhost[0], eth->ether_dhost[1],
           eth->ether_dhost[2], eth->ether_dhost[3],
           eth->ether_dhost[4], eth->ether_dhost[5]);

    int type = ntohs(eth->ether_type);

    printf("EtherType       : 0x%04x\n", type);

    if (type != ETHERTYPE_IP)
    {
        printf("Not IPv4. Skipping IP/TCP/UDP analysis.\n");
        return;
    }

    int offset = sizeof(struct ether_header);

    if (length < offset + 20)
        return;

    struct ip *ip;
    ip = (struct ip *)(packet + offset);

    int ip_len = ip->ip_hl * 4;

    if (ip->ip_v != 4 || ip_len < 20 ||
        length < offset + ip_len)
        return;

    char src[INET_ADDRSTRLEN];
    char dst[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &ip->ip_src, src, sizeof(src));
    inet_ntop(AF_INET, &ip->ip_dst, dst, sizeof(dst));

    printf("\n--- IP Header ---\n");
    printf("Source IP       : %s\n", src);
    printf("Destination IP  : %s\n", dst);
    printf("TTL             : %u\n", ip->ip_ttl);
    printf("Protocol        : %u\n", ip->ip_p);

    /* Ignore fragmented IPv4 packets in this simple program */
    if ((ntohs(ip->ip_off) & 0x3fff) != 0)
    {
        printf("Fragmented packet; skipping transport header.\n");
        return;
    }

    offset = offset + ip_len;

    if (ip->ip_p == IPPROTO_TCP)
    {
        if (length < offset + (int)sizeof(struct tcphdr))
            return;

        struct tcphdr *tcp;
        tcp = (struct tcphdr *)(packet + offset);

        printf("\n--- TCP Header ---\n");
        printf("Source Port     : %u\n", ntohs(tcp->th_sport));
        printf("Destination Port: %u\n", ntohs(tcp->th_dport));
        printf("Sequence Number : %u\n", ntohl(tcp->th_seq));
        printf("ACK Number      : %u\n", ntohl(tcp->th_ack));

        printf("Flags           : ");
        if (tcp->th_flags & TH_SYN) printf("SYN ");
        if (tcp->th_flags & TH_ACK) printf("ACK ");
        if (tcp->th_flags & TH_FIN) printf("FIN ");
        if (tcp->th_flags & TH_RST) printf("RST ");
        if (tcp->th_flags & TH_PUSH) printf("PSH ");
        printf("\n");
    }
    else if (ip->ip_p == IPPROTO_UDP)
    {
        if (length < offset + (int)sizeof(struct udphdr))
            return;

        struct udphdr *udp;
        udp = (struct udphdr *)(packet + offset);

        printf("\n--- UDP Header ---\n");
        printf("Source Port     : %u\n", ntohs(udp->uh_sport));
        printf("Destination Port: %u\n", ntohs(udp->uh_dport));
        printf("Length          : %u\n", ntohs(udp->uh_ulen));
        printf("Checksum        : 0x%04x\n", ntohs(udp->uh_sum));
    }
    else
    {
        printf("Transport protocol is neither TCP nor UDP.\n");
    }
}

int main(void)
{
    char error[PCAP_ERRBUF_SIZE];
    pcap_t *handle;

    handle = pcap_open_offline("capture.pcapng", error);
    if (handle == NULL)
    {
        printf("Cannot open capture file: %s\n", error);
        return 1;
    }

    if (pcap_datalink(handle) != DLT_EN10MB)
    {
        printf("This program expects Ethernet-format captures.\n");
        pcap_close(handle);
        return 1;
    }

    printf("Analysing capture.pcap...\n");

    int result = pcap_loop(handle, 0, analyse_packet, NULL);

    if (result == -1)
        printf("Capture read error: %s\n", pcap_geterr(handle));

    pcap_close(handle);

    printf("\nAnalysis completed.\n");

    return 0;
}
