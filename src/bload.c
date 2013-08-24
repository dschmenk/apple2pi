#include "a2lib.c"
#define	MAXSIZE	65536

unsigned char membuff[MAXSIZE];
int main(int argc, char **argv)
{
    FILE *binfile;
    unsigned char *memptr;
    int i, addr, len, result, fd;
    int pifd = a2open(argc > 3 ? argv[3] : "127.0.0.1");
    if (pifd < 0)
    {
	perror("Unable to connect to Apple II Pi");
	exit(EXIT_FAILURE);
    }
    if (argc < 3)
    {
	perror("Usage: bload <filename> <address> [ip address]\n");
	exit(EXIT_FAILURE);
    }
    addr = (int)strtol(argv[2], NULL, 0);
    if (addr > MAXSIZE)
    {
	perror("Address out of range\n");
	a2close(pifd);
	exit(EXIT_FAILURE);
    }
    sleep(1);
    fflush(stdin);
    if ((binfile = fopen(argv[1], "rb")))
    {
	len = fread(membuff, 1, MAXSIZE, binfile);
	fclose(binfile);
    }
    else
    {
	perror("Unable to read file\n");
	a2close(pifd);
	exit(EXIT_FAILURE);
    }
    if (addr + len > MAXSIZE)
	len = MAXSIZE - addr;
    memptr = membuff;
    printf("Loading");
    while (len)
    {
	printf(".");
	fflush(stdout);
	if (len > 512)
	{
	    a2write(pifd, addr, 512, memptr);
	    addr += 512;
	    memptr += 512;
	    len -= 512;
	}
	else
	{
	    a2write(pifd, addr, len, memptr);
	    len = 0;
	}
    }        
    printf("\n");
    a2close(pifd);
    return EXIT_SUCCESS;
}