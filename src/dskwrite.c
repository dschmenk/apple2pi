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
char online[] = {
// ORG $300
        0x20, 0x00, 0xBF, // JSR $BF00 (PRODOS)
        0xC5,             // DB ON_LINE
        0x08, 0x03,       // DW PARAMS
        0x60,             // RTS
        0xEA,
// PARAMS @ $308
        0x02,             // PARAM_COUNT
        0x60,             // UNIT_NUM = DRIVE 0, SLOT 6
        0x00, 0x20        // DATA_BUFFER = $2000
};        
char writeblk[] = {
// ORG $300
        0x20, 0x00, 0xBF, // JSR $BF00 (PRODOS)
        0x81,             // DB WRITE_BLOCK
        0x08, 0x03,       // DW PARAMS
        0x60,             // RTS
        0xEA,
// PARAMS @ $308
        0x03,             // PARAM_COUNT
        0x60,             // UNIT_NUM = DRIVE 0, SLOT 6
        0x00, 0x20,       // DATA_BUFFER = $2000
        0x00, 0x00        // BLOCK_NUM
};
#define ORG             0x0300
#define BLOCK_NUM       0x030C
#define DATA_BUFFER     0x2000
char dsk[280][512];

int main(int argc, char **argv)
{
	FILE *dskfile;
        char count[2], volname[21];
        int i, result, fd;
        int pifd = a2open(argc > 2 ? argv[2] : "127.0.0.1");
        if (pifd < 0)
        {
                perror("Unable to connect to Apple II Pi");
                exit(EXIT_FAILURE);
        }
        if (argc < 2)
        {
                perror("Usage: dskwrite <filename> [ip address]\n");
                exit(EXIT_FAILURE);
        }
        a2write(pifd, ORG, sizeof(online),  online);
        a2call(pifd, ORG, &result);
        if (result == 0)
        {
                a2read(pifd, DATA_BUFFER, 16, volname);
                volname[(volname[0] & 0x0F) + 1] = '\0';
                printf("Are you sure you want to overwrite volume :%s?", volname + 1);
                fflush(stdout);
                fgets(count, 2, stdin);
                if (count[0] != 'y' && count[0] != 'Y')
                {
                        a2close(pifd);
                        exit(EXIT_FAILURE);
                }
        }
        if ((dskfile = fopen(argv[1], "rb")))
	{
	        fread(dsk, 1, 280*512, dskfile);
		fclose(dskfile);
        }
        else
        {
                perror("Unable to read .PO file\n");
                a2close(pifd);
                exit(EXIT_FAILURE);
        }
        a2write(pifd, ORG, sizeof(writeblk), writeblk);
        for (i = 0; i < 280; i++)
        {
                printf("Writing block #%d\r", i);
                fflush(stdout);
                count[0] = i;
                count[1] = i >> 8;
                a2write(pifd, DATA_BUFFER, 512, dsk[i]);
                a2write(pifd, BLOCK_NUM, 2, count);
                a2call(pifd, ORG, &result);
        }        
        a2close(pifd);
        return EXIT_SUCCESS;
}