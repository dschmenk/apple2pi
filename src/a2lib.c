#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int a2open(char *ipaddr)
{
    struct sockaddr_in piaddr;
    int res;
    int fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0)
    {
        perror("Cannot create socket");
        return -1;
    }
    memset(&piaddr, 0, sizeof(piaddr));
    piaddr.sin_family = AF_INET;
    piaddr.sin_port = htons(6551);
    res = inet_pton(AF_INET, ipaddr, &piaddr.sin_addr);
    if (res < 0)
    {
        perror("First parameter is not a valid address family");
        close(fd);
        return -1;
    }
    else if (res == 0)
    {
        perror("Char string (second parameter does not contain valid ipaddress)");
        close(fd);
        return -1;
    }
    if (connect(fd, (struct sockaddr *)&piaddr, sizeof(piaddr)) < 0)
    {
        perror("Connect failed");
        close(fd);
        return -1;
    }
    return fd;
}
void a2close(int fd)
{
    char closepkt;
    closepkt = 0xFF;
    write(fd, &closepkt, 1);
    shutdown(fd, SHUT_RDWR);
    close(fd);
}
int a2read(int fd, int address, int count, char *buffer)
{
    char readpkt[8];
    readpkt[0] = 0x90; // read
    readpkt[1] = address;
    readpkt[2] = address >> 8;
    readpkt[3] = count;
    readpkt[4] = count >> 8;
    write(fd, readpkt, 5);
    read(fd, buffer, count);
    read(fd, readpkt, 2);
    return ((unsigned char)readpkt[0] == 0x9E);
}
int a2write(int fd, int address, int count, char *buffer)
{
    char writepkt[8];
    writepkt[0] = 0x92; // write
    writepkt[1] = address;
    writepkt[2] = address >> 8;
    writepkt[3] = count;
    writepkt[4] = count >> 8;
    write(fd, writepkt, 5);
    write(fd, buffer, count);
    read(fd, writepkt, 2);
    return ((unsigned char)writepkt[0] == 0x9E);
}
int a2call(int fd, int address, int *result)
{
    char callpkt[4];
    callpkt[0] = 0x9A; // call with keyboard flush
    callpkt[1] = address;
    callpkt[2] = address >> 8;
    write(fd, callpkt, 3);
    read(fd, callpkt, 2);
    if (result)
        *result = (unsigned char)callpkt[1];
    return ((unsigned char)callpkt[0] == 0x9E);
}
int a2quickcall(int fd, int address, int *result)
{
    char callpkt[4];
    callpkt[0] = 0x94; // call
    callpkt[1] = address;
    callpkt[2] = address >> 8;
    write(fd, callpkt, 3);
    read(fd, callpkt, 2);
    if (result)
        *result = (unsigned char)callpkt[1];
    return ((unsigned char)callpkt[0] == 0x9E);
}
