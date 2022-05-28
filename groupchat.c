#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>

#define MULTICAST_PORT 4321
#define MULTICAST_GROUP "225.0.0.1"

pthread_t s1, r;

void *sender(void *arg)
{
    int cid;
    cid = *((int *)arg);

    struct sockaddr_in c;
    c.sin_family = AF_INET;
    c.sin_port = htons(MULTICAST_PORT);
    inet_aton(MULTICAST_GROUP, &(c.sin_addr));

    while (1)
    {
        char mess[50];
        fgets(mess, sizeof(mess), stdin);
        int l = sizeof(c);
        sendto(cid, mess, sizeof(mess), 0, (struct sockaddr *)&c,
               sizeof(c));
        if (strncmp(mess, "bye", 3) == 0)
        {
            pthread_cancel(r);
            pthread_exit(NULL);
        }
    }
}

void *reciever(void *arg)
{
    int cid;
    cid = *((int *)arg);

    struct sockaddr_in c;

    /* Disable loopback so you do not receive your own datagrams. */
    /*int loopch = 0;
    if (setsockopt(cid, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0)
        perror("IP_MULTICAST_LOOP");*/

    while (1)
    {
        char mess[50];
        char SIP[16];
        short SPORT;
        int l = sizeof(c);
        recvfrom(cid, mess, sizeof(mess), 0, (struct sockaddr *)&c, &l);
        strcpy(SIP, inet_ntoa(c.sin_addr));
        SPORT = ntohs(c.sin_port);

        fprintf(stderr, "\nRecieved message from %s:%d -> %s\n", SIP, SPORT, mess);

        if (strncmp(mess, "bye", 3) == 0)
        {
            pthread_cancel(s1);
            pthread_exit(NULL);
        }
    }
}

int main(int ac, char **av)
{
    int sid = socket(AF_INET, SOCK_DGRAM, 0);
    perror("socket");

    /* Enable SO_REUSEADDR to allow multiple instances of this application */
    /* to receive copies of the multicast datagrams using the same address. */
    int reuse = 1;
    if (setsockopt(sid, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse)) < 0)
        perror("SO_REUSEADDR");

    struct sockaddr_in s;
    s.sin_family = AF_INET;
    s.sin_addr.s_addr = htonl(INADDR_ANY);
    s.sin_port = htons(MULTICAST_PORT);
    bind(sid, (struct sockaddr *)&s, sizeof(s));
    perror("bind");

    struct ip_mreq m;
    m.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
    m.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(sid, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&m, sizeof(m));
    perror("IP_ADD_MEMBERSHIP");

    printf("START\n");

    pthread_create(&s1, NULL, sender, &sid);
    pthread_create(&r, NULL, reciever, &sid);
    pthread_join(s1, NULL);
    pthread_join(r, NULL);

    return 0;
}
