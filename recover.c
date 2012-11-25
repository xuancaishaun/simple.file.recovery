#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define debugFlag 0

void printUsage(void);
void printSysInfo(char* filePath);
void printDirInfo(char* filePath);

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
	unsigned char DIR_Name[11];		/* File name*/
	unsigned char DIR_Attr;
	unsigned char DIR_NTRes;
	unsigned char DIR_CrtTimeTenth;
	unsigned short DIR_CrtTime;
	unsigned short DIR_CrtDate;
	unsigned short DIR_LatAccDate;
	unsigned short DIR_FstClusHI;	/* High 2 bytes of the first cluster address*/
	unsigned short DIR_WrtTime;
	unsigned short DIR_WrtDate;
	unsigned short DIR_FstClusLO; 	/* Low 2 bytes of the first cluster address*/
	unsigned long DIR_FileSize;		/* File size in bytes. (0 for directories)*/
};
#pragma pack(pop)

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
					printSysInfo(argv[2]);
					return 0;
				}
				else if (strcmp(argv[3], "-l") == 0){
					/**
					* Milestone 3: Listing all directory entries
					*/
					// Example: ./recover -d fat32.disk -l
					printDirInfo(argv[2]);
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

void printSysInfo(char* filePath){
	int fd;
	struct BootEntry * bootEntry = (struct BootEntry *)malloc(sizeof(struct BootEntry));
	
	if((fd=open((const char *)filePath, O_RDONLY))==-1) perror("Error");
	if (debugFlag) printf("Device file: %s\n", filePath);
	if (debugFlag) printf("The size of BootEntry: %d\n", (int)sizeof(struct BootEntry));
	if((int)read(fd, (void *) bootEntry, sizeof(struct BootEntry))==-1) perror("Error");

	printf("Number of FATs = %d\n", bootEntry->BPB_NumFATs);
	printf("Number of bytes per sector = %d\n", bootEntry->BPB_BytesPerSec);
	printf("Number of sectors per cluster = %d\n", bootEntry->BPB_SecPerClus);
	printf("Number of reserved sectors = %d\n", bootEntry->BPB_RsvdSecCnt);
	
	
}

int check_zero(struct DirEntry *tmpDir){
	if(tmpDir->DIR_Name[0]==0 && tmpDir->DIR_Name[1]==0 && tmpDir->DIR_Attr==0 && tmpDir->DIR_NTRes==0 && tmpDir->DIR_CrtTimeTenth==0 && tmpDir->DIR_CrtTime==0)
		return 0;
	else return 1;
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

int file_cluster(char hi, char lo){
	//printf("%x\t%x\n", hi, lo);
	int f_cluster=0;
	f_cluster = hi*256 + lo;
	return f_cluster;
}

void printDirInfo(char* filePath){
	int dd, fd;
	struct BootEntry * bootEntry = (struct BootEntry *)malloc(sizeof(struct BootEntry));
	struct DirEntry * tmpDir = (struct DirEntry *)malloc(sizeof(struct DirEntry));
	
	// Read boot sector information
	if((fd=open((const char *)filePath, O_RDONLY))==-1) perror("Error");
	if((int)read(fd, (void *) bootEntry, sizeof(struct BootEntry))==-1) perror("Error");

	// Read directories	
	//if((dd=open(filePath, O_RDONLY)) == -1) { perror("Error"); return; }

	//lseek(dd, bootEntry->BPB_BytesPerSec * bootEntry->BPB_HiddSec	, SEEK_CUR); // Hidden area
	//lseek(dd, bootEntry->BPB_BytesPerSec * bootEntry->BPB_RsvdSecCnt, SEEK_CUR); // Reserved area
	//lseek(dd, bootEntry->BPB_BytesPerSec * bootEntry->BPB_FATSz32 	* bootEntry->BPB_NumFATs, SEEK_CUR); // FAT Areas

	printf("BPB_RsvdSecCnt: %d\n", bootEntry->BPB_RsvdSecCnt);
	printf("BPB_HiddSec: %ld\n", bootEntry->BPB_HiddSec);
	printf("BPB_FATSz32: %ld\n", bootEntry->BPB_FATSz32);
	printf("BPB_NumFATs: %d\n", bootEntry->BPB_NumFATs);
	printf("ByteOfRootDir: %ld\n", bootEntry->BPB_BytesPerSec * (bootEntry->BPB_HiddSec + bootEntry->BPB_RsvdSecCnt + bootEntry->BPB_FATSz32 	* bootEntry->BPB_NumFATs));
	
	if( lseek(fd,(bootEntry->BPB_BytesPerSec * (bootEntry->BPB_HiddSec + bootEntry->BPB_RsvdSecCnt + bootEntry->BPB_FATSz32 	* bootEntry->BPB_NumFATs)), 0) <0 ){
			printf("lseek error");
	}
	int i = 0, ret_val=0, f_cluster=0;
	char f_name[12];
	while (1){
		(int)read(fd, (void *)tmpDir, (int)sizeof(struct DirEntry));
		if (check_zero(tmpDir)==0){ break;}
		if ( (ret_val = check_file(tmpDir->DIR_Name[0], tmpDir->DIR_Attr)) > 0) {
			i++;
			int j=0, k=0;
			while(tmpDir->DIR_Name[j]!=32 && j<8){ f_name[j]=tmpDir->DIR_Name[j];j++;}
			k=j, j=8;
			if(ret_val==2){f_name[k] = '.'; k++;}
			while(tmpDir->DIR_Name[j]!=32 && j<11){f_name[k]=tmpDir->DIR_Name[j];j++;k++;}
			f_name[k]='\0';

			f_cluster = file_cluster(tmpDir->DIR_FstClusHI, tmpDir->DIR_FstClusLO);
			
			if (ret_val == 1) {	
				printf("%d, %s/, %ld, %d\n", i, f_name, tmpDir->DIR_FileSize, f_cluster);
			}
			else if (ret_val == 2){
				printf("%d, %s, %ld, %d\n", i, f_name, tmpDir->DIR_FileSize, f_cluster);
			}
		}
//		else printf("Not valid!\n");
	}
	
	printf("BPB_RootClus = %ld\n", bootEntry->BPB_RootClus);

}


