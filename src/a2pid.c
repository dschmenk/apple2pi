#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <linux/uinput.h>

#define BAUDRATE B115200
#define A2DEVICE "/dev/ttyAMA0"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define die(str, args...) do { \
        prlog(str); \
        exit(-1); \
    } while(0)
/*
 * ASCII to scancode conversion
 */
#define MOD_ALT         0x40
#define MOD_CTRL        0x8000
#define MOD_SHIFT       0x4000

int scancode[256] = {
        /*
         * normal scancode
         */
        MOD_CTRL | MOD_SHIFT | KEY_2,           // CTRL-@ code 00
        MOD_CTRL |             KEY_A,           // CTRL-A code 01
        MOD_CTRL |             KEY_B,           // CTRL-B code 02
        MOD_CTRL |             KEY_C,           // CTRL-C code 03
        MOD_CTRL |             KEY_D,           // CTRL-D code 04
        MOD_CTRL |             KEY_E,           // CTRL-E code 05
        MOD_CTRL |             KEY_F,           // CTRL-F code 06
        MOD_CTRL |             KEY_G,           // CTRL-G code 07
                               KEY_LEFT,        // CTRL-H code 08
                               KEY_TAB,         // CTRL-I code 09
                               KEY_DOWN,        // CTRL-J code 0A
                               KEY_UP,          // CTRL-K code 0B
        MOD_CTRL |             KEY_L,           // CTRL-L code 0C
                               KEY_ENTER,       // CTRL-M code 0D
        MOD_CTRL |             KEY_N,           // CTRL-N code 0E
        MOD_CTRL |             KEY_O,           // CTRL-O code 0F
        MOD_CTRL |             KEY_P,           // CTRL-P code 10
        MOD_CTRL |             KEY_Q,           // CTRL-Q code 11
        MOD_CTRL |             KEY_R,           // CTRL-R code 12
        MOD_CTRL |             KEY_S,           // CTRL-S code 13
        MOD_CTRL |             KEY_T,           // CTRL-T code 14
                               KEY_RIGHT,       // CTRL-U code 15
        MOD_CTRL |             KEY_V,           // CTRL-V code 16
        MOD_CTRL |             KEY_W,           // CTRL-W code 17
        MOD_CTRL |             KEY_X,           // CTRL-X code 18
        MOD_CTRL |             KEY_Y,           // CTRL-Y code 19
        MOD_CTRL |             KEY_Z,           // CTRL-Z code 1A
                               KEY_ESC,         // ESCAPE code 1B
        MOD_CTRL |             KEY_BACKSLASH,   // CTRL-\ code 1C
        MOD_CTRL |             KEY_RIGHTBRACE,  // CTRL-] code 1D
        MOD_CTRL |             KEY_6,           // CTRL-6 code 1E
        MOD_CTRL |             KEY_MINUS,       // CTRL-- code 1F
                               KEY_SPACE,       // ' '    code 20
                   MOD_SHIFT | KEY_1,           // !      code 21
                   MOD_SHIFT | KEY_APOSTROPHE,  // "      code 22
                   MOD_SHIFT | KEY_3,           // #      code 23
                   MOD_SHIFT | KEY_4,           // $      code 24
                   MOD_SHIFT | KEY_5,           // %      code 25
                   MOD_SHIFT | KEY_7,           // &      code 26
                               KEY_APOSTROPHE,  // '      code 27
                   MOD_SHIFT | KEY_9,           // (      code 28
                   MOD_SHIFT | KEY_0,           // )      code 29
                   MOD_SHIFT | KEY_8,           // *      code 2A
                   MOD_SHIFT | KEY_EQUAL,       // +      code 2B
                               KEY_COMMA,       // ,      code 2C
                               KEY_MINUS,       // -      code 2D
                               KEY_DOT,         // .      code 2E
                               KEY_SLASH,       // /      code 2F
                               KEY_0,           // 0      code 30
                               KEY_1,           // 1      code 31
                               KEY_2,           // 2      code 32
                               KEY_3,           // 3      code 33
                               KEY_4,           // 4      code 34
                               KEY_5,           // 5      code 35
                               KEY_6,           // 6      code 36
                               KEY_7,           // 7      code 37
                               KEY_8,           // 8      code 38
                               KEY_9,           // 9      code 39
                   MOD_SHIFT | KEY_SEMICOLON,   // :      code 3A
                               KEY_SEMICOLON,   // ;      code 3B
                   MOD_SHIFT | KEY_COMMA,       // <      code 3C
                               KEY_EQUAL,       // =      code 3D
                   MOD_SHIFT | KEY_DOT,         // >      code 3E
                   MOD_SHIFT | KEY_SLASH,       // ?      code 3F
                   MOD_SHIFT | KEY_2,           // @      code 40
                   MOD_SHIFT | KEY_A,           // A      code 41
                   MOD_SHIFT | KEY_B,           // B      code 42
                   MOD_SHIFT | KEY_C,           // C      code 43
                   MOD_SHIFT | KEY_D,           // D      code 44
                   MOD_SHIFT | KEY_E,           // E      code 45
                   MOD_SHIFT | KEY_F,           // F      code 46
                   MOD_SHIFT | KEY_G,           // G      code 47
                   MOD_SHIFT | KEY_H,           // H      code 48
                   MOD_SHIFT | KEY_I,           // I      code 49
                   MOD_SHIFT | KEY_J,           // J      code 4A
                   MOD_SHIFT | KEY_K,           // K      code 4B
                   MOD_SHIFT | KEY_L,           // L      code 4C
                   MOD_SHIFT | KEY_M,           // M      code 4D
                   MOD_SHIFT | KEY_N,           // N      code 4E
                   MOD_SHIFT | KEY_O,           // O      code 4F
                   MOD_SHIFT | KEY_P,           // P      code 50
                   MOD_SHIFT | KEY_Q,           // Q      code 51
                   MOD_SHIFT | KEY_R,           // R      code 52
                   MOD_SHIFT | KEY_S,           // S      code 53
                   MOD_SHIFT | KEY_T,           // T      code 54
                   MOD_SHIFT | KEY_U,           // U      code 55
                   MOD_SHIFT | KEY_V,           // V      code 56
                   MOD_SHIFT | KEY_W,           // W      code 57
                   MOD_SHIFT | KEY_X,           // X      code 58
                   MOD_SHIFT | KEY_Y,           // Y      code 59
                   MOD_SHIFT | KEY_Z,           // Z      code 5A
                               KEY_LEFTBRACE,   // [      code 5B
                               KEY_BACKSLASH,   // \      code 5C
                               KEY_RIGHTBRACE,  // ]      code 5D
                   MOD_SHIFT | KEY_6,           // ^      code 5E
                   MOD_SHIFT | KEY_MINUS,       // _      code 5F
                               KEY_GRAVE,       // `      code 60
                               KEY_A,           // a      code 61
                               KEY_B,           // b      code 62
                               KEY_C,           // c      code 63
                               KEY_D,           // d      code 64
                               KEY_E,           // e      code 65
                               KEY_F,           // f      code 66
                               KEY_G,           // g      code 67
                               KEY_H,           // h      code 68
                               KEY_I,           // i      code 69
                               KEY_J,           // j      code 6A
                               KEY_K,           // k      code 6B
                               KEY_L,           // l      code 6C
                               KEY_M,           // m      code 6D
                               KEY_N,           // n      code 6E
                               KEY_O,           // o      code 6F
                               KEY_P,           // p      code 70
                               KEY_Q,           // q      code 71
                               KEY_R,           // r      code 72
                               KEY_S,           // s      code 73
                               KEY_T,           // t      code 74
                               KEY_U,           // u      code 75
                               KEY_V,           // v      code 76
                               KEY_W,           // w      code 77
                               KEY_X,           // x      code 78
                               KEY_Y,           // y      code 79
                               KEY_Z,           // z      code 7A
                   MOD_SHIFT | KEY_LEFTBRACE,   // {      code 7B
                   MOD_SHIFT | KEY_BACKSLASH,   // |      code 7C
                   MOD_SHIFT | KEY_RIGHTBRACE,  // }      code 7D
                   MOD_SHIFT | KEY_GRAVE,       // ~      code 7E
                               KEY_BACKSPACE,   // BS     code 7F                   
        /*
         * w/ closed apple scancodes
         */
        MOD_CTRL | MOD_SHIFT | KEY_2,           // CTRL-@ code 00
        MOD_CTRL |             KEY_A,           // CTRL-A code 01
        MOD_CTRL |             KEY_B,           // CTRL-B code 02
        MOD_CTRL |             KEY_C,           // CTRL-C code 03
        MOD_CTRL |             KEY_D,           // CTRL-D code 04
        MOD_CTRL |             KEY_E,           // CTRL-E code 05
        MOD_CTRL |             KEY_F,           // CTRL-F code 06
        MOD_CTRL |             KEY_G,           // CTRL-G code 07
                               KEY_HOME,        // CTRL-H code 08
                               KEY_INSERT,      // CTRL-I code 09
                               KEY_PAGEDOWN,    // CTRL-J code 0A
                               KEY_PAGEUP,      // CTRL-K code 0B
        MOD_CTRL |             KEY_L,           // CTRL-L code 0C
                               KEY_LINEFEED,    // CTRL-M code 0D
        MOD_CTRL |             KEY_N,           // CTRL-N code 0E
        MOD_CTRL |             KEY_O,           // CTRL-O code 0F
        MOD_CTRL |             KEY_P,           // CTRL-P code 10
        MOD_CTRL |             KEY_Q,           // CTRL-Q code 11
        MOD_CTRL |             KEY_R,           // CTRL-R code 12
        MOD_CTRL |             KEY_S,           // CTRL-S code 13
        MOD_CTRL |             KEY_T,           // CTRL-T code 14
                               KEY_END,         // CTRL-U code 15
        MOD_CTRL |             KEY_V,           // CTRL-V code 16
        MOD_CTRL |             KEY_W,           // CTRL-W code 17
        MOD_CTRL |             KEY_X,           // CTRL-X code 18
        MOD_CTRL |             KEY_Y,           // CTRL-Y code 19
        MOD_CTRL |             KEY_Z,           // CTRL-Z code 1A
                               KEY_ESC,         // ESCAPE code 1B
        MOD_CTRL |             KEY_BACKSLASH,   // CTRL-\ code 1C
        MOD_CTRL |             KEY_RIGHTBRACE,  // CTRL-] code 1D
        MOD_CTRL |             KEY_6,           // CTRL-6 code 1E
        MOD_CTRL |             KEY_MINUS,       // CTRL-- code 1F
                               KEY_SPACE,       // ' '    code 20
                   MOD_SHIFT | KEY_1,           // !      code 21
                   MOD_SHIFT | KEY_APOSTROPHE,  // "      code 22
                   MOD_SHIFT | KEY_3,           // #      code 23
                   MOD_SHIFT | KEY_4,           // $      code 24
                   MOD_SHIFT | KEY_5,           // %      code 25
                   MOD_SHIFT | KEY_7,           // &      code 26
                               KEY_APOSTROPHE,  // '      code 27
                   MOD_SHIFT | KEY_9,           // (      code 28
                   MOD_SHIFT | KEY_0,           // )      code 29
                   MOD_SHIFT | KEY_8,           // *      code 2A
                   MOD_SHIFT | KEY_EQUAL,       // +      code 2B
                               KEY_COMMA,       // ,      code 2C
                               KEY_MINUS,       // -      code 2D
                               KEY_DOT,         // .      code 2E
                               KEY_SLASH,       // /      code 2F
                               KEY_F10,         // 0      code 30
                               KEY_F1,          // 1      code 31
                               KEY_F2,          // 2      code 32
                               KEY_F3,          // 3      code 33
                               KEY_F4,          // 4      code 34
                               KEY_F5,          // 5      code 35
                               KEY_F6,          // 6      code 36
                               KEY_F7,          // 7      code 37
                               KEY_F8,          // 8      code 38
                               KEY_F9,          // 9      code 39
                   MOD_SHIFT | KEY_SEMICOLON,   // :      code 3A
                               KEY_SEMICOLON,   // ;      code 3B
                   MOD_SHIFT | KEY_COMMA,       // <      code 3C
                               KEY_EQUAL,       // =      code 3D
                   MOD_SHIFT | KEY_DOT,         // >      code 3E
                   MOD_SHIFT | KEY_SLASH,       // ?      code 3F
                   MOD_SHIFT | KEY_2,           // @      code 40
                   MOD_SHIFT | KEY_A,           // A      code 41
                   MOD_SHIFT | KEY_B,           // B      code 42
                   MOD_SHIFT | KEY_C,           // C      code 43
                   MOD_SHIFT | KEY_D,           // D      code 44
                   MOD_SHIFT | KEY_E,           // E      code 45
                   MOD_SHIFT | KEY_F,           // F      code 46
                   MOD_SHIFT | KEY_G,           // G      code 47
                   MOD_SHIFT | KEY_H,           // H      code 48
                   MOD_SHIFT | KEY_I,           // I      code 49
                   MOD_SHIFT | KEY_J,           // J      code 4A
                   MOD_SHIFT | KEY_K,           // K      code 4B
                   MOD_SHIFT | KEY_L,           // L      code 4C
                   MOD_SHIFT | KEY_M,           // M      code 4D
                   MOD_SHIFT | KEY_N,           // N      code 4E
                   MOD_SHIFT | KEY_O,           // O      code 4F
                   MOD_SHIFT | KEY_P,           // P      code 50
                   MOD_SHIFT | KEY_Q,           // Q      code 51
                   MOD_SHIFT | KEY_R,           // R      code 52
                   MOD_SHIFT | KEY_S,           // S      code 53
                   MOD_SHIFT | KEY_T,           // T      code 54
                   MOD_SHIFT | KEY_U,           // U      code 55
                   MOD_SHIFT | KEY_V,           // V      code 56
                   MOD_SHIFT | KEY_W,           // W      code 57
                   MOD_SHIFT | KEY_X,           // X      code 58
                   MOD_SHIFT | KEY_Y,           // Y      code 59
                   MOD_SHIFT | KEY_Z,           // Z      code 5A
                               KEY_LEFTBRACE,   // [      code 5B
                               KEY_BACKSLASH,   // \      code 5C
                               KEY_RIGHTBRACE,  // ]      code 5D
                   MOD_SHIFT | KEY_6,           // ^      code 5E
                   MOD_SHIFT | KEY_MINUS,       // _      code 5F
                               KEY_GRAVE,       // `      code 60
                               KEY_A,           // a      code 61
                               KEY_B,           // b      code 62
                               KEY_C,           // c      code 63
                               KEY_D,           // d      code 64
                               KEY_E,           // e      code 65
                               KEY_F,           // f      code 66
                               KEY_G,           // g      code 67
                               KEY_H,           // h      code 68
                               KEY_I,           // i      code 69
                               KEY_J,           // j      code 6A
                               KEY_K,           // k      code 6B
                               KEY_L,           // l      code 6C
                               KEY_M,           // m      code 6D
                               KEY_N,           // n      code 6E
                               KEY_O,           // o      code 6F
                               KEY_P,           // p      code 70
                               KEY_Q,           // q      code 71
                               KEY_R,           // r      code 72
                               KEY_S,           // s      code 73
                               KEY_T,           // t      code 74
                               KEY_U,           // u      code 75
                               KEY_V,           // v      code 76
                               KEY_W,           // w      code 77
                               KEY_X,           // x      code 78
                               KEY_Y,           // y      code 79
                               KEY_Z,           // z      code 7A
                   MOD_SHIFT | KEY_LEFTBRACE,   // {      code 7B
                   MOD_SHIFT | KEY_BACKSLASH,   // |      code 7C
                   MOD_SHIFT | KEY_RIGHTBRACE,  // }      code 7D
                   MOD_SHIFT | KEY_GRAVE,       // ~      code 7E
                               KEY_DELETE       // DELETE code 7F
};
int accel[16] = { 0,  1,  4,  8,  8,  8,  8,  8,
                 -8, -8, -8, -8, -8, -8, -4, -1}; 
volatile int stop = FALSE, isdaemon = FALSE;
struct input_event evkey, evrelx, evrely, evsync;

void sendkey(int fd, int mod, int key)
{
        int code = scancode[(mod & 0x80) | (key & 0x7F)];
        /*
         * press keys
         */
        if (mod & MOD_ALT)
        {
                evkey.code  = KEY_LEFTALT;
                evkey.value = 1;
                write(fd, &evkey, sizeof(evkey));
        }
        if (code & MOD_CTRL)
        {
                evkey.code  = KEY_LEFTCTRL;
                evkey.value = 1;
                write(fd, &evkey, sizeof(evkey));
        }
        if (code & MOD_SHIFT)
        {
                evkey.code  = KEY_LEFTSHIFT;
                evkey.value = 1;
                write(fd, &evkey, sizeof(evkey));
        }
        evkey.code  = code & 0x3FF;
        evkey.value = 1;
        write(fd, &evkey,  sizeof(evkey));
        write(fd, &evsync, sizeof(evsync));
        /*
         * release keys
         */
        evkey.code  = code & 0x3FF;
        evkey.value = 0;
        write(fd, &evkey, sizeof(evkey));
        if (code & MOD_SHIFT)
        {
                evkey.code  = KEY_LEFTSHIFT;
                evkey.value = 0;
                write(fd, &evkey, sizeof(evkey));
        }
        if (code & MOD_CTRL)
        {
                evkey.code  = KEY_LEFTCTRL;
                evkey.value = 0;
                write(fd, &evkey, sizeof(evkey));
        }        
        if (mod & MOD_ALT)
        {
                evkey.code  = KEY_LEFTALT;
                evkey.value = 0;
                write(fd, &evkey, sizeof(evkey));
        }
        write(fd, &evsync, sizeof(evsync));
}
void sendbttn(int fd, int mod, int bttn)
{
        static int lastbtn = 0;
        
        if (bttn)
        {
                lastbtn        =
                evkey.code  = (mod == 0)   ? BTN_LEFT 
                            : (mod & 0x40) ? BTN_RIGHT
                                           : BTN_MIDDLE;
                evkey.value = 1;
        }
        else
        {
                evkey.code  = lastbtn;
                evkey.value = 0;
        }
        write(fd, &evkey, sizeof(evkey));
        write(fd, &evsync, sizeof(evsync));
}
void sendrelxy(int fd, int x, int y)
{
        evrelx.value = x;
        evrely.value = y;
        write(fd, &evrelx, sizeof(evrelx));
        write(fd, &evrely, sizeof(evrely));
        write(fd, &evsync, sizeof(evsync));
}
void prlog(char *str)
{
        if (!isdaemon)
                printf(str);
}
void main(int argc, char **argv)
{
        struct uinput_user_dev uidev;
        struct termios oldtio,newtio;
        char a2event[4];
        int i, lastbtn;
        int a2fd, kbdfd, moufd;

        /*
         * Are we running as a isdaemon?
         */
        if ((argc > 1)/* && (strcmp(argv[1], "--daemon") == 8)*/)
        {
                pid_t pid, sid; /* our process ID and Session ID */
                
                pid = fork();   /* fork off the parent process */
                if (pid < 0)
                        die("a2pid: fork() failure");
                /*
                 * If we got a good PID, then
                 * we can exit the parent process
                 */
                if (pid > 0)
                        exit(EXIT_SUCCESS);
                umask(0);       /* change the file mode mask */
                /*
                 * Open any logs here
                 */
                sid = setsid(); /* create a new SID for the child process */
                if (sid < 0)
                        die("a2pid: setsid() failure");
                if ((chdir("/")) < 0)   /* change the current working directory */
                        die("a2pid: chdir() failure");
                /*
                 * Close out the standard file descriptors
                 */
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                isdaemon = TRUE;
        }
        /*
         * Create keyboard input device
         */
        prlog("a2pid: Create keyboard input device\n");
        kbdfd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (kbdfd < 0)
                die("error: uinput open");
        if (ioctl(kbdfd, UI_SET_EVBIT, EV_KEY) < 0)
                die("error: uinput ioctl EV_KEY");
        for (i = KEY_ESC; i <= KEY_F10; i++)
                if (ioctl(kbdfd, UI_SET_KEYBIT, i) < 0)
                        die("error: uinput ioctl SET_KEYBITs");        
        for (i = KEY_HOME; i <= KEY_DELETE; i++)
                if (ioctl(kbdfd, UI_SET_KEYBIT, i) < 0)
                        die("error: uinput ioctl SET_KEYBITs");
        if (ioctl(kbdfd, UI_SET_EVBIT, EV_SYN) < 0)
                die("error: ioctl EV_SYN");
        bzero(&uidev, sizeof(uidev));
        snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Apple2 Pi Keyboard");
        uidev.id.bustype = BUS_RS232;
        uidev.id.vendor  = 0x05ac;      /* apple */
        uidev.id.product = 0x2e;
        uidev.id.version = 1;
        write(kbdfd, &uidev, sizeof(uidev));
        if (ioctl(kbdfd, UI_DEV_CREATE) < 0)
                die("error: ioctl DEV_CREATE");
        /*
         * Create mouse input device
         */
        prlog("a2pid: Create mouse input device\n");
        moufd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (moufd < 0)
                die("error: uinput open");
        if (ioctl(moufd, UI_SET_EVBIT, EV_KEY) < 0)
                die("error: uinput ioctl EV_KEY");
        if (ioctl(moufd, UI_SET_KEYBIT, BTN_LEFT) < 0)
                die("error: uinput ioctl BTN_LEFT");
        if (ioctl(moufd, UI_SET_KEYBIT, BTN_RIGHT) < 0)
                die("error: uinput ioctl BTN_RIGHT");
        if (ioctl(moufd, UI_SET_KEYBIT, BTN_MIDDLE) < 0)
                die("error: uinput ioctl BTN_MIDDLE");
        if (ioctl(moufd, UI_SET_EVBIT, EV_REL) < 0)
                die("error: ioctl EV_REL");
        if (ioctl(moufd, UI_SET_RELBIT, REL_X) < 0)
                die("error: ioctl REL_X");
        if (ioctl(moufd, UI_SET_RELBIT, REL_Y) < 0)
                die("error: ioctl REL_Y");
        if (ioctl(moufd, UI_SET_EVBIT, EV_SYN) < 0)
                die("error: ioctl EV_SYN");
        bzero(&uidev, sizeof(uidev));
        snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Apple2 Pi Mouse");
        uidev.id.bustype = BUS_RS232;
        uidev.id.vendor  = 0x05ac;      /* apple */
        uidev.id.product = 0x2e;
        uidev.id.version = 1;
        write(moufd, &uidev, sizeof(uidev));
        if (ioctl(moufd, UI_DEV_CREATE) < 0)
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
         * Open serial port
         */
        prlog("a2pid: Open serial port\n");
        a2fd = open(A2DEVICE, O_RDWR | O_NOCTTY);
        if (a2fd < 0)
                die("error: serial port open");
        tcgetattr(a2fd, &oldtio); /* save current port settings */
        bzero(&newtio, sizeof(newtio));
        newtio.c_cflag     = BAUDRATE /*| CRTSCTS*/ | CS8 | CLOCAL | CREAD;
        newtio.c_iflag     = IGNPAR;
        newtio.c_oflag     = 0;
        newtio.c_lflag     = 0; /* set input mode (non-canonical, no echo,...) */
        newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
        newtio.c_cc[VMIN]  = 1; /* blocking read until 1 char received */
        tcsetattr(a2fd, TCSANOW, &newtio);
        prlog("a2pid: Waiting...\n");
        tcflush(a2fd, TCIFLUSH);
        newtio.c_cc[VMIN]  = 3; /* blocking read until 3 chars received */
        tcsetattr(a2fd, TCSANOW, &newtio);
        if (read(a2fd, a2event, 1) == 1)
        {
                if (a2event[0] == 0x80)     /* receive sync */
                {
                        prlog("a2pid: Connected.\n");
                        a2event[0] = 0x81;  /* acknowledge */
                        write(a2fd, a2event, 1);
                        tcflush(a2fd, TCIFLUSH);
                }
                else
                        stop = TRUE;
        }
        /*
         * Event loop
         */
        while (!stop)
        {
                if (read(a2fd, a2event, 3) == 3)
                {
                        // printf("a2pi: [0x%02X] [0x%02X] [0x%02X] ", a2event[0], a2event[1], a2event[2]);
                        switch (a2event[0])
                        {
                                case 0x80: /* sync */
                                        prlog("a2pid: Re-Connected.\n");
                                        a2event[0] = 0x81;  /* acknowledge */
                                        write(a2fd, a2event, 1);
                                        tcflush(a2fd, TCIFLUSH);
                                        break;                                
                                case 0x82: /* keyboard event */
                                        // printf("Keyboard Event: 0x%02X:%c\n", a2event[1], a2event[2] & 0x7F);
                                        sendkey(kbdfd, a2event[1], a2event[2]);
                                        if (a2event[2] == 0x9B && a2event[1] & 0x80)
                                                stop = TRUE;
                                        break;
                                case 0x84: /* mouse move event */
                                        // printf("Mouse XY Event: %d,%d\n", (signed char)a2event[1], (signed char)a2event[2]);
                                        sendrelxy(moufd, accel[a2event[1] & 0x0F], accel[a2event[2] & 0x0F]);
                                        break;
                                case 0x86: /* mouse button event */
                                        // printf("Mouse Button %s Event 0x%02X\n", a2event[2] ? "[PRESS]" : "[RELEASE]", a2event[1]);
                                        sendbttn(moufd, a2event[1], a2event[2]);
                                        break;
                                default:
                                        prlog("a2pid: Unknown Event\n");
                                        stop = TRUE;
                        }
                }
                else
                {
                        prlog("a2pid: ????\n");
                        stop = TRUE;
                }
        }
        tcsetattr(a2fd, TCSANOW, &oldtio);
        tcflush(a2fd, TCIFLUSH);
        close(a2fd);
        ioctl(moufd, UI_DEV_DESTROY);
        ioctl(kbdfd, UI_DEV_DESTROY);
        close(moufd);
        close(kbdfd);
}