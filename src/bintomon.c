#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	FILE *obj;
	int start_addr;
	unsigned char b;

	if (argc < 2)
	{
		fprintf(stderr, "Include start address on command line.\n");
		exit(1);
	}
	if (argc < 3)
	{
		fprintf(stderr, "Include file to convert on command line.\n");
		exit(1);
	}
	sscanf(argv[1], "%x", &start_addr);
	if ((obj = fopen(argv[2], "rb")))
	{
		printf("%04X:", start_addr);
		while (fread(&b, 1, 1, obj) == 1)
		{
			printf(" %02X", b);
			if (!(++start_addr & 0x07))
				printf("\n:");
		}
		printf("\n");
		fclose(obj);
	}
	return (0);
}
