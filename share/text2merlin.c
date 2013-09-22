#include <stdio.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	int fout;
	char c;

	if (argc > 1)
		fout = open(argv[1], O_WRONLY|O_CREAT, 0644);
	else
		fout = 1;
	while ((c = getc(stdin)) != EOF)
	{
		if (c == '\n')
			c = '\r';
		if (c == '\t')
			c = ' ';
		c |= 0x80;
		write(fout, &c, 1);
	}
	close(fout);
	return (0);
}
