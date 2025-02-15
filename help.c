#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	printf("Usage:\n");
	printf("  > add <FILENAME> [OPTION]\n");
	printf("    -d : add directory recursive\n");
	printf("  > remove <FILENAME> [OPTION]\n");
	printf("    -a : remove all file(recursive)\n");
	printf("    -c : clear backup directory\n");
	printf("  > recover <FILENAME> [OPTION]\n");
	printf("    -d : recover directory recursive\n");
	printf("    -n <NEWNAME> : recover file with new name\n");
	printf("  > ls\n  > vi\n  > vim\n  > help\n  > exit\n\n\n");
	exit(0);
}
