#include <stdio.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	int fin, comment, charpos;
	char c;

	if (argc > 1)
	{
		fin = open(argv[1], O_RDONLY, 0);
	}
	else
		fin = 0;
	comment = charpos = 0;
	while (read(fin, &c, 1) == 1)
	{
		c &= 0x7f;
		if ((charpos++ == 0 && c == '*') || c == ';')
			comment = 1;
		if (c == '\r')
		{
			comment = charpos = 0;
			c = '\n';
		}
		if (c == ' ' && !comment)
			c = '\t';
		putc(c & 0x7f, stdout);
	}
	return (0);
}
