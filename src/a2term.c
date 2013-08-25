#include "a2lib.c"
#include <termios.h>
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
    int pifd = a2open(argc > 1 ? argv[1] : "127.0.0.1");
    if (pifd < 0)
    {
	perror("Unable to connect to Apple II Pi");
	exit(EXIT_FAILURE);
    }
    tcgetattr(STDIN_FILENO, &oldtio); /* save current port settings */
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag     = /*BAUDRATE | CRTSCTS |*/ CS8 | CLOCAL | CREAD;
    newtio.c_iflag     = IGNPAR;
    newtio.c_oflag     = 0;
    newtio.c_lflag     = 0; /* set input mode (non-canonical, no echo,...) */
    newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
    newtio.c_cc[VMIN]  = 1; /* blocking read until 1 char received */
    tcsetattr(STDIN_FILENO, TCSANOW, &newtio);
    FD_ZERO(&openset);
    FD_SET(pifd,  &openset);
    FD_SET(STDIN_FILENO, &openset);
    /*
     * Tell a2pid that we want Apple II character output.
     */
    iopkt[0] = 0x98;
    write(pifd, iopkt, 1);
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
				state = STOP;
			}
		    }
		    else if (iopkt[0] == 0x7F)
			iopkt[0] = 0x88;
		    if (cin == 0)
		    {
			cin= iopkt[0] | 0x80;
			a2cin(pifd, cin);
		    }
		    /*
		     * else drop the character!
		     */
		}
	    }
	}
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldtio);
    a2close(pifd);
    return EXIT_SUCCESS;
}