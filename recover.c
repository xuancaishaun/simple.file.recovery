#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define debugFlag 1

void initBootInfo(char* filePath); // Initialize Boot Sector Information. IMPORTANT.
void printUsage(void);
void printSysInfo(char* filePath); // print info. about Boot Sector.
int  printOneInfo(char* filePath, int id, int startByte); // print info. about a single File or Folder.
void printAllInfo(char* filePath); // print info. about File/Folder under Root and sub-root Directories.
int  getClusNum(char hi, char lo);
int  getStartByte(int clusterNum);


#pragma pack(push,1)
struct BootEntry{
	unsigned char BS_jmpBoot[3];
	unsigned char BS_OEMName[8];
	unsigned short BPB_BytesPerSec;	/*Bytes per sector. Allowed values include 512
	******				 1024, 2048 and 4096.*/
	unsigned char BPB_SecPerClus;	/*Sectors per cluster (data unit). Allowed values
	******				 are powers of 2, but the cluster size must be 32KB
					 or smaller.*/
	unsigned short BPB_RsvdSecCnt;  /*Size in sectors of the reserved areas.*/
	//******
	unsigned char BPB_NumFATs; 	/*Number of FATs*/
	//******
	unsigned short BPB_RootEntCnt;	/*Maximum number of files in the root directory for
					 FAT12 and FAT16. This is 0 for FAT32.*/
	unsigned short BPB_TotSec16;	/*16-bit values of number of sectors in file system.*/
	//******
	unsigned char BPB_Media;	/*Media Type*/
	unsigned short BPB_FATSz16;	/*16-bit size in sectors of each FAT for FAT12 and FAT16.
					 For FAT32, this field is 0.*/
	unsigned short BPB_SecPerTrk;	/*Sectors per track of storage device.*/
	unsigned short BPB_NumHeads;	/*Number of heads in storage device.*/
	unsigned long BPB_HiddSec;	/*Number of sectors before the start of the partition.*/
	unsigned long BPB_TotSec32;	/*32-bit value of number of sectors in file system.
	******				 Either this value or the 16-bit value above must be 0.*/
	unsigned long BPB_FATSz32;	/*32-bit */
	unsigned short BPB_ExtFlags;	/*A flag for FAT*/
	unsigned short BPB_FSVer;	/*The major and minor version number*/
	unsigned long BPB_RootClus;	/*Cluster where the root directory can be found.*/
	unsigned short BPB_FAInfo;	/*Sector where FSINFO structure can be found*/
	unsigned short BPB_BkBootSec;	/*Sector where backup copy of boot sector is located*/
	unsigned char BPB_Reserved[12];	/*Reserved*/
	unsigned char BS_DrvNum;	/*BIOS INT13h drive number*/
	unsigned char BS_Reserved1;	/*Not used*/
	unsigned char BS_BootSig;	/*Extended boot signature to identify if the next three
					 values are valid*/
	unsigned long BS_VolID;		/*Volume serial number*/
	unsigned char BS_VolLab[11];	/*Volume label in ASCII. User defines when ceating the file system*/
	unsigned char BS_FilSysType[8];	/*File system type label in ASCII*/
};
#pragma pack(pop)

#pragma pack(push,1)
struct DirEntry{
	unsigned char DIR_Name[11];	/* File name*/
	unsigned char DIR_Attr;		/* File attributes*/
	unsigned char DIR_NTRes;
	unsigned char DIR_CrtTimeTenth;
	unsigned short DIR_CrtTime;
	unsigned short DIR_CrtDate;
	unsigned short DIR_LatAccDate;
	unsigned short DIR_FstClusHI;	/* High 2 bytes of the first cluster address*/
	unsigned short DIR_WrtTime;
	unsigned short DIR_WrtDate;
	unsigned short DIR_FstClusLO; 	/* Low 2 bytes of the first cluster address*/
	unsigned long DIR_FileSize;	/* File size in bytes. (0 for directories)*/
};
#pragma pack(pop)

#define BOOT_SIZE (int)sizeof(struct BootEntry)
#define DIRENT_SIZE (int)sizeof(struct DirEntry)
#define FLAG_EOF -2
#define FLAG_Not_Printed -1
#define FLAG_FILE 0 

// A list of global variables of Boot Sector
static int 		global_initFlag = 0; // false means it has not been initialized.	
static unsigned short	global_BytesPerSec;
static unsigned char 	global_SecPerClus;
static unsigned short	global_RsvdSecCnt;
static unsigned char	global_NumFATs;
static unsigned long	global_HiddSec;
static unsigned long	global_FATSz32;
static unsigned long	global_RootClus;
static unsigned long 	global_RootStartByte;
static unsigned long 	global_FATStartByte;
static unsigned long 	global_FATTotalByte;
static int		global_fd;

int main(int argc, char **argv){

	/**
	* Milestone 1: Detecting valid arguments
	*/
	switch (argc)
	{
		case 4:
			if (strcmp(argv[1], "-d") == 0){
				if (strcmp(argv[3], "-i") == 0){
					/**
					* Milestone 2: Printing file system information
					*/	
					// Example: ./revover -d fat32.disk -i
					initBootInfo(argv[2]);
					printSysInfo(argv[2]);
					return 0;
				}
				else if (strcmp(argv[3], "-l") == 0){
					/**
					* Milestone 3: Listing all directory entries
					*/
					// Example: ./recover -d fat32.disk -l
					initBootInfo(argv[2]);
					printAllInfo(argv[2]);
					return 0;
				}
			}
			break;

		case 5:
			if ((strcmp(argv[1], "-d") == 0)&&(strcmp(argv[3], "-r") == 0)){
				printf("-r detected\n");
				// Example; ./recover -d fat32.disk -r [filename]
				// do something
				initBootInfo(argv[2]);
				return 0;
			}
			break;
	}

	printUsage();

	return 0;
}

void initBootInfo(char * filePath){	// Used to initialize information about boot sector
	// Must be called
	struct BootEntry * bootEntry = (struct BootEntry *) malloc(BOOT_SIZE);
	if((global_fd = open((const char *) filePath, O_RDONLY)) == -1) { perror("Error: fail to read info. about Boot Sector"); exit(-1);}
	if((int)read(global_fd, (void *)bootEntry, BOOT_SIZE) == -1) {perror("Error: fail to read info. about Boot Sector"); exit(-1);}
	global_initFlag    = 1;
	global_BytesPerSec = bootEntry->BPB_BytesPerSec;
	global_SecPerClus  = bootEntry->BPB_SecPerClus;
	global_RsvdSecCnt  = bootEntry->BPB_RsvdSecCnt;
	global_NumFATs     = bootEntry->BPB_NumFATs;
	global_HiddSec     = bootEntry->BPB_HiddSec;
	global_FATSz32     = bootEntry->BPB_FATSz32;
	global_RootClus    = bootEntry->BPB_RootClus;
	global_RootStartByte = global_BytesPerSec * (global_HiddSec + global_RsvdSecCnt + global_NumFATs * global_FATSz32);
	global_FATStartByte  = global_RsvdSecCnt * global_BytesPerSec;
	global_FATTotalByte  = global_FATSz32 * global_BytesPerSec;

	if (debugFlag) printf("RootStartByte = %ld\n", global_RootStartByte);
	if (debugFlag) printf("FATStartByte = %ld\n", global_FATStartByte);
	if (debugFlag) printf("FATTotalByte = %ld\n", global_FATTotalByte);
}

void printUsage(void){
	printf("Usage: ./recover -d [device filename] [other arugments]\n");
	printf("-i			Print boot sector information\n");
	printf("-l			List all the directory entries\n");
	printf("-r filename [-m md5]	File recovery\n");
}

void printSysInfo(char* filePath){
	if(!global_initFlag) initBootInfo(filePath);	
	printf("Number of FATs = %d\n", global_NumFATs);
	printf("Number of bytes per sector = %d\n", global_BytesPerSec);
	printf("Number of sectors per cluster = %d\n", global_SecPerClus);
	printf("Number of reserved sectors = %d\n", global_RsvdSecCnt);
}

int printOneInfo(char* filePath, int id, int startByte){
	// Example: 1, MAKEFILE, 21, 11
	struct DirEntry * tmp = (struct DirEntry *)malloc(DIRENT_SIZE);
	
//	if((fd = open((const char*)filePath, O_RDONLY)) == -1) {perror("Error: printOneInfo()"); exit(-1);}
	if(lseek(global_fd, startByte, 0) == -1) { perror("Error: printOneInfo()"); exit(-1);}
	if(read(global_fd, (void *) tmp, DIRENT_SIZE) == -1) { perror("Error: printOneInfo()"); exit(-1);}
	
	// Requirement A: check if this is the last entry in Root directory
	if(tmp->DIR_Name[0] == 0) {
		if(debugFlag) printf("FLAG_EOF reached \n");
		return FLAG_EOF;
	}

	// Requirement B: check if this entry is deleted or not.
	if(tmp->DIR_Name[0] == 229){	// 229 <==> 0xE5 ==> dot
		if(debugFlag) printf("FLAG_Not_Printed reached\n");
		return FLAG_Not_Printed; 
	}

	// Requirement C: check if this is LFN or not.
	if(tmp->DIR_Attr == 15){	// 15 <==> 0x0F ==> Long File Name, just ignore
		if(debugFlag) printf("LFN reached\n");
		return FLAG_Not_Printed;
	}

	// Requirement D: 
	// File => FileName.Extension
	int clusNum = getClusNum(tmp->DIR_FstClusHI, tmp->DIR_FstClusLO);	
	char fileName[12];
	int j = 0, k = 0;
	while(tmp->DIR_Name[j] != 32 && j < 8) fileName[k++] = tmp->DIR_Name[j++]; 
	if(tmp->DIR_Attr / 16 % 2 == 0) fileName[k++] = '.';
	j = 8;
	while(tmp->DIR_Name[j] != 32 && j < 11) fileName[k++] = tmp->DIR_Name[j++];
	fileName[k] = '\0';

	if(tmp->DIR_Attr / 16 % 2 == 0){ // xxx0 xxxx ==> this is a File.
		printf("%d, %s, %ld, %d\n", id, fileName, tmp->DIR_FileSize, clusNum);
		return FLAG_FILE;
	}else{	
		// Folder
		//    => Folder/, 0, StartClus
		//    => Folder/., 0, StartClus => printed later
		//    => Folder/.., 0, 0	=> printed later
		printf("%d, %s/, 0, %d\n", id, fileName, clusNum);
		return clusNum;
	}
	/* IMPORTANT Note for return value*/
	// return value:
	//	(a) FLAG_EOF: last entry in Root directory
	//	(b) FLAG_Not_Printed: this entry is not printed and do not increase id.
	//	(c) FLAG_FILE: this entry is a file
	//	(d) Values > 1: this is a sub-directory and its starting cluster is Return Value. 

	if (debugFlag) printf("WTF\n");
}

int check_file(char first_char, char file_attr){
	//printf("First Char of Filename: %d\t", first_char);
	//printf("File Attribute = %x\n", file_attr);
	if (first_char != 0 && first_char != -27 && (file_attr+1)%16 != 0){
		if(file_attr >= 16 && file_attr < 32)
			return 1;
		else if(file_attr >= 32)
			return 2;
	}
	else return 0;
}

int getClusNum(char hi, char lo){
	// Covert to Cluster Number from its Higher and Lower 2 bytes
	int f_cluster = 0;
	f_cluster = hi*256 + lo;
	return f_cluster;
}

int getStartByte(int clusternum){
	// Data clusters start at data area and the 1st cluster is Cluster 2.
	return (global_RootStartByte + (clusternum-2) * global_SecPerClus * global_BytesPerSec);
}

void printAllInfo(char* filePath){
	int current_byte = global_RootStartByte;
	int current_id = 1;
	int cluster_id = 2; // Root dir starts at cluster 2
	int ret_value;
	
//	printOneInfo(filePath, current_id, global_RootStartByte + 9 * 32);

	while(1){
		switch(ret_value = printOneInfo(filePath, current_id, current_byte)){
			case FLAG_EOF:
				if (debugFlag) printf("Last entry reached\n");
				return;
			case FLAG_Not_Printed:
				break;
			case FLAG_FILE:
				current_id ++;
				break;
			default:	// This is a folder; ret_val => starting address of cluster
				current_id ++;
				break;
		}
		current_byte += DIRENT_SIZE;
	}
	return;	

}



