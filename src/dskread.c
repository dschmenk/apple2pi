#include "a2lib.c"

char online[] = {
// ORG $301
    0x20, 0x00, 0xBF, // JSR $BF00 (PRODOS)
    0xC5,             // DB ON_LINE
    0x08, 0x03,       // DW PARAMS
    0x60,             // RTS
// PARAMS @ $308
    0x02,             // PARAM_COUNT
    0x60,             // UNIT_NUM = DRIVE 0, SLOT 6
    0x00, 0x20        // DATA_BUFFER = $2000
};        
char readblk[] = {
// ORG $301
    0x20, 0x00, 0xBF, // JSR $BF00 (PRODOS)
    0x80,             // DB READ_BLOCK
    0x08, 0x03,       // DW PARAMS
    0x60,             // RTS
// PARAMS @ $308
    0x03,             // PARAM_COUNT
    0x60,             // UNIT_NUM = DRIVE 0, SLOT 6
    0x00, 0x20,       // DATA_BUFFER = $2000
    0x00, 0x00        // BLOCK_NUM
};
#define ORG             0x0301
#define BLOCK_NUM       0x030C
#define DATA_BUFFER     0x2000
char dsk[280][512];

int main(int argc, char **argv)
{
    FILE *dskfile;
    char count[2], volname[21];
    int i, result, fd;
    int pifd = a2open(argc > 1 ? argv[1] : "127.0.0.1");
    if (pifd < 0)
    {
	perror("Unable to connect to Apple II Pi");
	exit(EXIT_FAILURE);
    }
    a2write(pifd, ORG, sizeof(online),  online);
    a2call(pifd, ORG, &result);
    a2read(pifd, DATA_BUFFER, 16, volname);
    volname[(volname[0] & 0x0F) + 1] = '\0';
    printf("Volume name:%s\n", volname + 1);
    strcat(volname + 1, ".PO");
    a2write(pifd, ORG, sizeof(readblk), readblk);
    for (i = 0; i < 280; i++)
    {
	printf("Reading block #%d\r", i);
	fflush(stdout);
	count[0] = i;
	count[1] = i >> 8;
	a2write(pifd, BLOCK_NUM, 2, count);
	a2call(pifd, ORG, &result);
	a2read(pifd, DATA_BUFFER, 512, dsk[i]);
    }        
    a2close(pifd);
    if ((dskfile = fopen(volname + 1, "wb")))
    {
	fwrite(dsk, 1, 280*512, dskfile);
	fclose(dskfile);
    }
    return EXIT_SUCCESS;
}
