#include "a2lib.c"

int main(int argc, char **argv)
{
    unsigned char cmd, ack;
    int pifd = a2open(argc > 3 ? argv[3] : "127.0.0.1");
    if (pifd < 0)
    {
	perror("Unable to connect to Apple II Pi");
	exit(EXIT_FAILURE);
    }
    if (argc < 2)
    {
	perror("Usage: a2picmd <cmd>\n");
	exit(EXIT_FAILURE);
    }
    cmd = (int)strtol(argv[1], NULL, 0); /* treat command as value */
    write(pifd, &cmd, 1);
    read(pifd, &ack, 1);
    a2close(pifd);
    return ack == (cmd + 1);
}
