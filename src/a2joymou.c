/*
 * Copyright 2013, David Schmenk
 */
#include "a2lib.c"
#include <fcntl.h>
#include <time.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <signal.h>
#define	FALSE		0
#define	TRUE		(!FALSE)
#define	POLL_HZ		100	/* must be greater than 1 */
struct timespec tv;
struct input_event evkey, evrelx, evrely, evsync;
#define	BTTN_IO		0xC061
#define	READGP0		0x320
#define	READGP1		0x328
char readgp[] = {
    0xA2, 0x00,       // LDX #PADDLE
    0x78,             // SEI
    0x20, 0x1E, 0xFB, // JSR PREAD
    0x98,             // TYA
    0x60,             // RTS
};
int accel[20] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 5, 7, 9, 10};
/*
 * Error handling.
 */
int isdebug, stop = FALSE;
#define prdbg(s)	do{if(isdebug)fprintf(stderr,(s));}while(0)
#define die(str, args...) do { \
    prdbg(str); \
    exit(-1); \
} while(0)
static void sig_bye(int signo)
{
    stop = TRUE;
}

void main(int argc, char **argv)
{
    struct uinput_user_dev uidev;
    int a2fd, joyfd, relx, rely, cntrx, cntry, gptoggle;
    unsigned char prevbttns[2], bttns[2];
    
    int pifd = a2open("127.0.0.1");
    if (pifd < 0)
    {
	perror("Unable to connect to Apple II Pi");
	exit(EXIT_FAILURE);
    }
    /*
     * Are we running debug?
     */
    if ((argc > 1) && (strcmp(argv[1], "-D") == 0))
    {
	isdebug = TRUE;	
    }
    else
    {
	if (daemon(0, 0) != 0)
	    die("a2joy: daemon() failure");
	isdebug = FALSE;
    }
    /*
     * Create joystick input device
     */
    prdbg("a2joy: Create joystick input device\n");
    joyfd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (joyfd < 0)
	die("error: uinput open");
    if (ioctl(joyfd, UI_SET_EVBIT, EV_KEY) < 0)
	die("error: uinput ioctl EV_KEY");
    if (ioctl(joyfd, UI_SET_KEYBIT, BTN_LEFT) < 0)
	die("error: uinput ioctl BTN_LEFT");
    if (ioctl(joyfd, UI_SET_KEYBIT, BTN_RIGHT) < 0)
	die("error: uinput ioctl BTN_RIGHT");
    if (ioctl(joyfd, UI_SET_EVBIT, EV_REL) < 0)
	die("error: ioctl EV_rel");
    if (ioctl(joyfd, UI_SET_RELBIT, REL_X) < 0)
	die("error: ioctl rel_X");
    if (ioctl(joyfd, UI_SET_RELBIT, REL_Y) < 0)
	die("error: ioctl rel_Y");
    if (ioctl(joyfd, UI_SET_EVBIT, EV_SYN) < 0)
	die("error: ioctl EV_SYN");
    bzero(&uidev, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Apple2 Pi Joystick");
    uidev.id.bustype = BUS_RS232;
    uidev.id.vendor  = 0x05ac;      /* apple */
    uidev.id.product = 0x2e;
    uidev.id.version = 1;
    write(joyfd, &uidev, sizeof(uidev));
    if (ioctl(joyfd, UI_DEV_CREATE) < 0)
	die("error: ioctl DEV_CREATE");
    /*
     * Initialize event structures.
     */
    bzero(&evkey,  sizeof(evkey));
    bzero(&evsync, sizeof(evsync));
    bzero(&evrelx, sizeof(evrelx));
    bzero(&evrely, sizeof(evrely));
    evkey.type  = EV_KEY;
    evrelx.type = EV_REL;
    evrelx.code = REL_X;
    evrely.type = EV_REL;
    evrely.code = REL_Y;
    evsync.type = EV_SYN;
    /*
     * Add signal handlers.
     */
    if (signal(SIGINT, sig_bye) == SIG_ERR)
	die("signal");
    if (signal(SIGHUP, sig_bye) == SIG_ERR)
	die("signal");
    /*
     * Set poll rate.
     */
    tv.tv_sec = 0;
    tv.tv_nsec = isdebug ? 1E+9/2 : 1E+9/POLL_HZ;
    /*
     * Download 6502 code.
     */
    readgp[1] = 0;
    a2write(pifd, READGP0, sizeof(readgp), readgp);
    readgp[1] = 1;
    a2write(pifd, READGP1, sizeof(readgp), readgp);
    evrelx.value = 0;
    evrely.value = 0;
    a2quickcall(pifd, READGP0, &cntrx);
    a2read(pifd, BTTN_IO, 2, prevbttns);
    a2quickcall(pifd, READGP1, &cntry);
    gptoggle = 0;
    /*
     * Poll joystick loop.
     */
    prdbg("a2joymou: Enter poll loop\n");
    while (!stop)
    {
	if (gptoggle)
	{
	    a2quickcall(pifd, READGP0, &relx);
#if 0
	    if (relx >= cntrx + 20)		
		evrelx.value = (relx - cntrx) / 2;
	    else if (relx >= cntrx)		
		evrelx.value = accel[relx - cntrx];
	    else if (relx <= cntrx - 20)		
		evrelx.value = (relx - cntrx) / 2;
	    else		
		evrelx.value = -accel[cntrx - relx];
#else
            evrelx.value = (relx - cntrx) / 12;
            if (evrelx.value < 0)
                evrelx.value *= -evrelx.value;
            else
                evrelx.value *= evrelx.value;
#endif
	    write(joyfd, &evrelx, sizeof(evrelx));
	    write(joyfd, &evrely, sizeof(evrely));
	    write(joyfd, &evsync, sizeof(evsync));
	    if (isdebug) fprintf(stderr, "a2joymou (%d, %d) [%d %d]\n", relx, rely, bttns[0] >> 7, bttns[1] >> 7);
	}
	else
	{
	    a2quickcall(pifd, READGP1, &rely);
#if 0            
	    if (rely >= cntry + 20)		
		evrely.value = (rely - cntry) / 2;
	    else if (rely >= cntry)		
		evrely.value = accel[rely - cntry];
	    else if (rely <= cntry - 20)		
		evrely.value = (rely - cntry) / 2;
	    else		
		evrely.value = -accel[cntry - rely];
#else
            evrely.value = (rely - cntry) / 12;
            if (evrely.value < 0)
                evrely.value *= -evrely.value;
            else
                evrely.value *= evrely.value;
#endif
        }
	gptoggle ^= 1;
	a2read(pifd, BTTN_IO, 2, bttns);
	if ((bttns[0] & 0x80) != prevbttns[0]) 
	{
	    if (!evkey.value)
		evkey.code = (bttns[1] & 0x80) ? BTN_RIGHT : BTN_LEFT;
	    evkey.value    = bttns[0] >> 7;
	    write(joyfd, &evkey, sizeof(evkey));
	    write(joyfd, &evsync, sizeof(evsync));
	    prevbttns[0] = bttns[0] & 0x80;
	    prevbttns[1] = bttns[1] & 0x80;
	}
	nanosleep(&tv, NULL);
    }
    a2close(pifd);
    ioctl(joyfd, UI_DEV_DESTROY);
    close(joyfd);
    prdbg("\na2joymou: Exit\n");
}
