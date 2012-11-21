#include <stdio.h>

int main(int argc, char **argv){

	int i;
/**
* Milestone 1: Detecting valid arguments
*/
	switch (argc)
	{
		case 3:
			break;
		case 4:
			break;
		default:
			printf("Usage: ./recover -d [device filename] [other arugments]\n");
			printf("-i			Print boot sector information\n");
			printf("-l			List all the directory entries\n");
			printf("-r filename [-m md5]	File recovery\n");
			return 0;
	}	
	if (argc == 3)
		printf("hellow world\n");
	for (i = 0; i < argc; i ++){
		printf("%s\n", argv[i]);
	}

	return 0;
}
