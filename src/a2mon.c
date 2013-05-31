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
        piaddr.sin_port = htons(6502);
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
        callpkt[0] = 0x94; // call
        callpkt[1] = address;
        callpkt[2] = address >> 8;
        write(fd, callpkt, 3);
        read(fd, callpkt, 2);
        if (result)
                *result = (unsigned char)callpkt[1];
        return ((unsigned char)callpkt[0] == 0x9E);
}
void prbytes(int address, int count, char *data)
{
        int i;
        
        printf("%04X:", address);
        for (i = 0; i < count; i++)
                printf(" %02X", (unsigned char)data[i]);
        printf("\n");
}
void exec(int fd, int cmd, int *address, int value, char *data, int *index)
{
        int a, c;

        switch (cmd)
        {
                case 0x00:
                        if (value != -1)
                                *address = value;
                        break;
                case 0x90: /* read */
                        if (value < *address)
                                value = *address;
                        for (a = *address; a <= value; a += 16)
                        {
                                c = a + 16 > value ? value - a + 1 : 16;
                                a2read(fd, a, c, data);
                                prbytes(a, c, data);                                
                        }
                        *address = value + 1;
                        break;
                case 0x92: /* write */
                        if (*index)
                                a2write(fd, *address, *index, data);
                        *address += *index;
                        *index = 0;
                        break;
                case 0x94: /* call */
                        a2call(fd, *address, NULL);
                        break;
        }
}
int parsestr(int fd, char *cmdstr)
{
        char databuf[1024];
        static int addr = 0;
        int index = 0;
        int parseval = -1;
        int cmd =0 ;
        
        while (1)
        {
                switch (*cmdstr)
                {
                        case ':': /* write bytes */
                                exec(fd, cmd, &addr, parseval, databuf, &index);
                                cmd = 0x92;
                                parseval = -1;
                                break;
                        case '.': /* read address range */
                                exec(fd, cmd, &addr, parseval, databuf, &index);
                                cmd = 0x90;
                                parseval = -1;
                                break;
                        case 'R': /* run */
                        case 'r':
                                exec(fd, cmd, &addr, parseval, databuf, &index);
                                cmd = 0x94;
                                break;
                        case 'a':
                        case 'b':
                        case 'c':
                        case 'd':
                        case 'e':
                        case 'f':
                                *cmdstr -= 'a' - 'A';
                        case 'A':
                        case 'B':
                        case 'C':
                        case 'D':
                        case 'E':
                        case 'F':
                                *cmdstr -= 'A' - '9' - 1;
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                                if (parseval == -1)
                                        parseval = 0;
                                parseval = parseval * 16 + *cmdstr - '0';
                                break;
                        case ' ':
                                if (parseval != -1)
                                {
                                        if (cmd == 0x92)
                                                databuf[index++] = parseval;
                                        else
                                                exec(fd, cmd, &addr, parseval, databuf, &index);
                                        parseval = -1;
                                }
                                break;
                        case '\n':
                        case '\0':
                                if (parseval != -1)
                                {
                                        if (cmd == 0x92)
                                                databuf[index++] = parseval;
                                        else if (cmd == 0)
                                        {
                                                addr = parseval;
                                                cmd = 0x90;
                                        }
                                }
                                exec(fd, cmd, &addr, parseval, databuf, &index);
                                return 1;
                                break;
                        case 'Q':
                        case 'q':
                                return 0;
                        default:
                                return 1;
                }
                cmdstr++;
        }
        return 1;
}
int main(int argc, char **argv)
{
        char instr[256];
        int pifd = a2open(argc > 1 ? argv[1] : "127.0.0.1");
        if (pifd < 0)
        {
                perror("Unable to connect to Apple II Pi");
                exit(EXIT_FAILURE);
        }
        while (fgets(instr, 254, stdin) != NULL)
        {
                if (!parsestr(pifd, instr))
                        break;
        }
        a2close(pifd);
        return EXIT_SUCCESS;
}