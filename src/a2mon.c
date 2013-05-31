#include "a2lib.c"

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