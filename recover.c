#include <stdio.h>
#include <string.h>

void printUsage(void);

int main(int argc, char **argv){

/**
* Milestone 1: Detecting valid arguments
*/
	switch (argc)
	{
		case 4:
			if (strcmp(argv[1], "-d") == 0){
				if (strcmp(argv[3], "-i") == 0){
					printf("-i detected\n");
					// Example: ./revover -d fat32.disk -i
					// do something
					return 0;
				}
				else if (strcmp(argv[3], "-l") == 0){
					printf("-l detected\n");
					// Example: ./recover -d fat32.disk -l
					// do something
					return 0;
				}
			}
			break;

		case 5:
			if ((strcmp(argv[1], "-d") == 0)&&(strcmp(argv[3], "-r") == 0)){
				printf("-r detected\n");
				// Example; ./recover -d fat32.disk -r [filename]
				// do something
				return 0;
			}
			break;
	}

	printUsage();

	return 0;
}

void printUsage(void){
	printf("Usage: ./recover -d [device filename] [other arugments]\n");
	printf("-i			Print boot sector information\n");
	printf("-l			List all the directory entries\n");
	printf("-r filename [-m md5]	File recovery\n");
}
