#include "a2lib.c"

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