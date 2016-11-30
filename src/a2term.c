#include <fcntl.h>
#include <termios.h>
#include "a2lib.c"
#define	RUN	0
#define	STOP	1

int a2cin(int fd, char cin)
{
    unsigned char cinpkt[2];
    cinpkt[0] = 0x96; // keyboard input
    cinpkt[1] = cin;
    return (write(fd, cinpkt, 2));
}
int main(int argc, char **argv)
{
    struct termios oldtio,newtio;
    fd_set readset, openset;
    unsigned char iopkt[2];
    int state = RUN, cin = 0;
    int pifd, echofd = -1;
    char *a2host = "127.0.0.1";
    if (!isatty(STDIN_FILENO))
        echofd = STDIN_FILENO;
    /*
     * Parse arguments.
     */
    while (argc > 1)
    {
        if (argc > 2 && argv[1][0] == '-' && argv[1][1] == 'f')
        {
            echofd = open(argv[2], O_RDONLY);
            argc -= 2;
            argv += 2;
        }
        else if (argv[1][0] != '-')
        {
            a2host = argv[1];
            argc--;
            argv++;
        }
        else
        {
            fprintf(stderr, "Invalid option: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }
    if ((pifd = a2open(a2host)) < 0)
    {
	fprintf(stderr, "Unable to connect to Apple II Pi\n");
	exit(EXIT_FAILURE);
    }
    /*
     * Tell a2pid that we want Apple II character output.
     */
    iopkt[0] = 0x98;
    write(pifd, iopkt, 1);
    if (echofd >= 0)
    {
	/*
	 * Echo input file making sure to not drop characters.
	 */
	while (read(echofd, &cin, 1) == 1)
	{
	    if (cin == '\n')
		cin = 0x0D;
	    cin |= 0x80;
	    a2cin(pifd, cin);
	    while (read(pifd, iopkt, 2) == 2)
	    {
		if (iopkt[0] == 0x98)
		{
		    /*
		     * Echo output.
		     */
		    putchar(iopkt[1] == 0x8D ? '\n' : iopkt[1] & 0x7F);
		}
		else if (iopkt[0] == 0x9E)
		{
		    if (iopkt[1] == cin)
			break;
		    else
			fprintf(stderr, "\nInput character mismatch!\n");
		}
	    }
	}
        if (echofd != STDIN_FILENO)
        {
            close(echofd);
            putchar('\n');
        }
        fflush(stdout);
    }
    /*
     * Are we running interactively?
     */
    if (isatty(STDIN_FILENO))
    {
	/*
	 * Give a little instruction to the user.
	 */
	fprintf(stderr, "\nPress ESC-Q to quit, ESC-ESC sends \'ESCAPE\'.\n\n");
	fprintf(stderr, "Warning: Do NOT 'BYE' out of AppleSoft or run a system program.\n");
	fprintf(stderr, "Apple II Pi client will be disabled and you will lose keyboard and mouse.\n");
	/*
	 * Change input setting to work better as a terminal.
	 */
	tcgetattr(STDIN_FILENO, &oldtio); /* save current port settings */
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag     = /*BAUDRATE | CRTSCTS |*/ CS8 | CLOCAL | CREAD;
	newtio.c_iflag     = IGNPAR;
	newtio.c_oflag     = 0;
	newtio.c_lflag     = 0; /* set input mode (non-canonical, no echo,...) */
	newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
	newtio.c_cc[VMIN]  = 1; /* blocking read until 1 char received */
	tcsetattr(STDIN_FILENO, TCSANOW, &newtio);
	/*
	 * Start off with a carriage return.
	 */
	cin = 0x8D;
	a2cin(pifd, cin);
	/*
	 * Prepare for select().
	 */
	FD_ZERO(&openset);
	FD_SET(pifd,  &openset);
	FD_SET(STDIN_FILENO, &openset);
	/*
	 * Event loop
	 */
	while (state == RUN)
	{
	    memcpy(&readset, &openset, sizeof(openset));
	    if (select(pifd + 1, &readset, NULL, NULL, NULL) > 0)
	    {
		/*
		 * Apple II character output.
		 */
		if (FD_ISSET(pifd, &readset))
		{
		    if (read(pifd, iopkt, 2) == 2)
		    {
			if (iopkt[0] == 0x98)
			{
			    putchar(iopkt[1] & 0x7F);
			    if (iopkt[1] == 0x8D)
				putchar('\n');
			    fflush(stdout);
			}
			else if (iopkt[0] == 0x9E)
			{
			    if (iopkt[1] == cin)
				cin = 0;
			    else
			    {
				fprintf(stderr, "\nInput character mismatch!\n");
				state = STOP;
			    }
			}
		    }
		    else
			/*
			 * Some kind of client/server error.
			 */
			state = STOP;
		}
		if (FD_ISSET(STDIN_FILENO, &readset))
		{
		    if (read(STDIN_FILENO, iopkt, 1) == 1)
		    {
			if (iopkt[0] == 0x1B)
			{
			    if (read(STDIN_FILENO, iopkt, 1) == 1)
			    {
				if (iopkt[0] == 0x5B && read(STDIN_FILENO, iopkt, 1) == 1)
				{
				    switch (iopkt[0])
				    {
					case 0x44: // left arrow
					    iopkt[0] = 0x88;
					    break;
					case 0x43: // right arrow
					    iopkt[0] = 0x95;
					    break;
					case 0x42: // down arrow
					    iopkt[0] = 0x8A;
					    break;
					case 0x41: // up arrow
					    iopkt[0] = 0x9B;
					    break;
					default:
					    iopkt[0] = 0xA0;
				    }
				}
				else if (iopkt[0] == 'q' || iopkt[0] == 'Q')
				{
				    cin = 0x1B;
				    state = STOP;
				    break;
				}
			    }
			}
			else if (iopkt[0] == 0x7F)
			    iopkt[0] = 0x88;
			if (cin == 0)
			{
			    cin = iopkt[0] | 0x80;
			    a2cin(pifd, cin);
			}
			/*
			 * else drop the character (we are interactive)
			 */
		    }
		    else
			/*
			 * stdin probably closed.
			 */
			state = STOP;
		}
	    }
	}
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldtio);
    a2close(pifd);
    fprintf(stderr, "\n");
    return EXIT_SUCCESS;
}