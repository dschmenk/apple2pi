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
    int state = RUN, echochr = 0;
    int pifd = a2open(argc > 1 ? argv[1] : "127.0.0.1");
    if (pifd < 0)
    {
	perror("Unable to connect to Apple II Pi");
	exit(EXIT_FAILURE);
    }
    /*
     * Are we running interactively?
     */
    if (isatty(STDIN_FILENO))
    {
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
	 * Prepare for select().
	 */
	echofd = 0;
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
			/*
			 * Acknowledgement.
			 */
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
			/*
			 * Chain input character to child.
			 */
		    }
		    else
			/*
			 * stdin probably closed.
			 */
			state = STOP;
		}
		if (FD_ISSET(fdecho, &readset))
		{
		    if (read(STDIN_FILENO, iopkt, 1) == 1)
		    {
			/*
			 * Send stdout as cin to Apple II.
			 */
			if (opkt[0] == 0x04) // Ctrl-D
			    state = STOP;
			a2cin(pifd, iopkt[0] | 0x80);
		    }
		}
	    }
	}
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldtio);
    a2close(pifd);
    return EXIT_SUCCESS;
}