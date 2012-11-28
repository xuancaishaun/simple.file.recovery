#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define debugFlag 0 

// If a variable could have neative values, then use int.
// otherwise, use signed long.

void initBootInfo(char* filePath); // Initialize Boot Sector Information. IMPORTANT.
void printUsage(void);
void printSysInfo(char* filePath); // print info. about Boot Sector.
unsigned long  printOneInfo(char* filePath, unsigned long id, unsigned long startByte, int subFlag, char* parentFileName); // print info. about a single File or Folder.
void printAllInfo(char* filePath); // print info. about File/Folder under Root and sub-root Directories.
unsigned long  getClusNum(unsigned short hi, unsigned short lo);
unsigned long  getStartByte(unsigned long clusterNum);
void getFileName(char* filePath, unsigned long startByte, char* fileName);
unsigned long  getNextClusterNum(unsigned long currentClusterNum);
unsigned long  searchFileName(char * filePath, unsigned long cluster_id, int dirDepth, char * namePart, char * extPart);
unsigned long  getNameExtParts(char * filePath, unsigned long startByte, char * NamePart, char * ExtPart);
unsigned long  isEntryFile(char * filePath, unsigned long startByte);
unsigned long  isEntryDeleted(char * filePath, unsigned long startByte);
unsigned long  isEntryLFN(char * filePath, unsigned long startByte);
unsigned long  isEntryEOF(char * filePath, unsigned long startByte);
unsigned long  getClusterID(char * filePath, unsigned long startByte);
int changeFAT(char * filePath, unsigned long cluster_id, unsigned long next_cluster_id);
void recoverFile(char * filePath, unsigned long start_cluster_id);
unsigned long  getFileSize(char * filePath, unsigned long startByte);

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

#define BOOT_SIZE (unsigned long)sizeof(struct BootEntry)
#define DIRENT_SIZE (unsigned long)sizeof(struct DirEntry)
#define FLAG_EOF -1
#define FLAG_Not_Printed -2
#define FLAG_FILE -3	//These flags have to be non-negative

#define FLAG_FAT_CHANGE_FAIL 1
#define FLAG_FAT_CHANGE_SUCC 2

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
					if( debugFlag)	printf("%ld\n", getStartByte(3));
					printAllInfo(argv[2]);
					return 0;
				}
			}
			break;

		case 5:
			if ((strcmp(argv[1], "-d") == 0)&&(strcmp(argv[3], "-r") == 0)){
				// Example; ./recover -d fat32.disk -r [filename]
				// do something
				initBootInfo(argv[2]);

				char inputName[9] = {'\0'};
				char inputExt[4] = {'\0'};
				char *token;
				token  = strtok(argv[4], ".");
				if (token != NULL)
					strncpy( inputName, (const char *) token, 8);
				if (debugFlag) printf("Input Name: %s\n", inputName);
				
				token = strtok(NULL, " ");
				if (token != NULL)
					strncpy( inputExt, (const char *) token, 3);
				if (debugFlag) printf("Input Ext : %s\n", inputExt);
			
				// Convert all alphabets to Uppaer Case.
				int i;
				for ( i = 0; i < 9; i++){
					if(isalpha(inputName[i]) && islower(inputName[i])) inputName[i] = inputName[i] - 32; 
				}

				for ( i = 0; i < 4; i++){
					if(isalpha(inputExt[i]) && islower(inputExt[i])) inputExt[i] = inputExt[i] - 32;
				}
		
				// Covert the 1st one to 0xE5				
				inputName[0] = 229;
				
				if (debugFlag) printf("Modified Input Name: %s\n", inputName);
				if (debugFlag) printf("Modified Input Ext : %s\n", inputExt);
			
				unsigned long clusterNum =  searchFileName(argv[2], 2, 2, inputName, inputExt);

				if (clusterNum == 0) printf("%s: error - file not found\n", argv[2]);
				else{
					if ( debugFlag) printf("\t===> File found: %ld\n", clusterNum);
				}

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

	if (debugFlag) printf("\t---> RootStartByte = %ld\n", global_RootStartByte);
	if (debugFlag) printf("\t---> FATStartByte = %ld\n", global_FATStartByte);
	if (debugFlag) printf("\t---> FATTotalByte = %ld\n", global_FATTotalByte);
}

void printUsage(void){
	printf("Usage: ./recover -d [device filename] [other arugments]\n");
	printf("-i			Print boot sector information\n");
	printf("-l			List all the directory entries\n");
	printf("-r filename 		File recovery\n");
}

void printSysInfo(char* filePath){
	if(!global_initFlag) initBootInfo(filePath);	
	printf("Number of FATs = %d\n", global_NumFATs);
	printf("Number of bytes per sector = %d\n", global_BytesPerSec);
	printf("Number of sectors per cluster = %d\n", global_SecPerClus);
	printf("Number of reserved sectors = %d\n", global_RsvdSecCnt);
}

unsigned long searchFileName(char * filePath, unsigned long cluster_id, int dirDepth, char * namePart, char * extPart){
//	char target_namePart[9];// 8 char + '\0';	MAX_SIZE of namePart = 9, including '\0'
//	char target_extPart[4]; // 3 char + '\0';	MAX_SIZE of  extPart = 4, including '\0'
	char tmp_namePart[9];
	char tmp_extPart[4];
//	strncpy(target_namePart, (const char*) namePart, 9);
//	strncpy(target_extPart, (const char*) extPart, 4);
	char * target_namePart = namePart;
	char * target_extPart  = extPart;

	unsigned long current_order_id;
	unsigned long current_byte;
	//unsigned long cluster_id = 2;
	unsigned long num_of_entries;
	unsigned long sub_current_byte;
	unsigned long sub_cluster_id;
	unsigned long sub_num_of_entries;
	unsigned int  sub_EOF_Flag;

	unsigned long cluster_num = 0;
	// This variable indicates the depth of directories
	// dirDepth = 2: (1) Root Directory (2) Sub-root Directory
	dirDepth -- ;

	do{
		num_of_entries = 0;	
		current_byte = getStartByte(cluster_id);
		
		while(1){
			num_of_entries ++;

			if(isEntryEOF(filePath, current_byte))
				return cluster_num;

			if(isEntryFile(filePath, current_byte) && isEntryDeleted(filePath, current_byte) && !isEntryLFN(filePath, current_byte)){	
		//	if(isEntryFile(filePath, current_byte) && !isEntryDeleted(filePath, current_byte) && !isEntryLFN(filePath, current_byte)){
				cluster_num = getNameExtParts( filePath, current_byte, tmp_namePart, tmp_extPart);
				
				if (debugFlag){
					printf("\t-------------------------\n");
					printf("\t---> Target Name Part: %s\n", target_namePart);
					printf("\t---> Target Ext  Part: %s\n", target_extPart);
					printf("\t---> Source Name Part: %s\n", tmp_namePart);
					printf("\t---> Source Ext  Part: %s\n", tmp_extPart);
				}

				if(strcmp(target_namePart, tmp_namePart) == 0 && strcmp(target_extPart, tmp_extPart) == 0){
					return cluster_num;
				}else{
					cluster_num = 0;
				}

			}else if(dirDepth && !isEntryFile(filePath, current_byte) && !isEntryDeleted(filePath, current_byte)){
				cluster_num = searchFileName(filePath, getClusterID(filePath, current_byte), dirDepth, namePart, extPart);
				if (cluster_num != 0) return cluster_num;
			}

			current_byte += DIRENT_SIZE;
			if(num_of_entries == 16) break;
		}

	}while((cluster_id = getNextClusterNum(cluster_id)) != -1);

	//return cluster_num;	// if cluster_num == 0, not found	
}

unsigned long isEntryEOF(char * filePath, unsigned long startByte){
	// 0 ==> NO
	// 1 ==> YES

	struct DirEntry* tmp = (struct DirEntry *) malloc(DIRENT_SIZE);
        if(lseek(global_fd, startByte, 0) == -1) { perror("Error: printOneInfo()"); exit(-1);}
        if(read(global_fd, (void *) tmp, DIRENT_SIZE) == -1) { perror("Error: printOneInfo()"); exit(-1);}

        if(tmp->DIR_Name[0] == 0) { free(tmp); return 1;}
        else { free(tmp); return 0;}
}

unsigned long isEntryFile(char* filePath, unsigned long startByte){
	// 0 ==> diretory
	// 1 ==> File
	struct DirEntry* tmp = (struct DirEntry *) malloc(DIRENT_SIZE);
	if(lseek(global_fd, startByte, 0) == -1) { perror("Error: printOneInfo()"); exit(-1);}
        if(read(global_fd, (void *) tmp, DIRENT_SIZE) == -1) { perror("Error: printOneInfo()"); exit(-1);}

	if(tmp->DIR_Attr / 16 % 2 == 0) { free(tmp); return 1;}
        else { free(tmp); return 0;}
}

unsigned long isEntryDeleted(char * filePath, unsigned long startByte){
	// 0 ==> Existing
        // 1 ==> Deleted
        struct DirEntry* tmp = (struct DirEntry *) malloc(DIRENT_SIZE);
        if(lseek(global_fd, startByte, 0) == -1) { perror("Error: printOneInfo()"); exit(-1);}
        if(read(global_fd, (void *) tmp, DIRENT_SIZE) == -1) { perror("Error: printOneInfo()"); exit(-1);}

        if(tmp->DIR_Name[0] == 229) { free(tmp); return 1;}
        else { free(tmp); return 0;}
}

unsigned long isEntryLFN(char * filePath, unsigned long startByte){
	// 0 ==> Not LFN
        // 1 ==> LFN
        struct DirEntry* tmp = (struct DirEntry *) malloc(DIRENT_SIZE);
        if(lseek(global_fd, startByte, 0) == -1) { perror("Error: printOneInfo()"); exit(-1);}
        if(read(global_fd, (void *) tmp, DIRENT_SIZE) == -1) { perror("Error: printOneInfo()"); exit(-1);}

        if(tmp->DIR_Attr == 15) { free(tmp); return 1;}
        else { free(tmp); return 0;}
}

unsigned long getClusterID(char * filePath, unsigned long startByte){
	struct DirEntry* tmp = (struct DirEntry *) malloc(DIRENT_SIZE);
        if(lseek(global_fd, startByte, 0) == -1) { perror("Error: printOneInfo()"); exit(-1);}
        if(read(global_fd, (void *) tmp, DIRENT_SIZE) == -1) { perror("Error: printOneInfo()"); exit(-1);}

        unsigned long cluster_id = getClusNum(tmp->DIR_FstClusHI, tmp->DIR_FstClusLO);
	free(tmp);
	return cluster_id;
}

int changeFAT(char * filePath, unsigned long cluster_id, unsigned long next_cluster_id){
	
	unsigned long buf;
	unsigned long offset = global_FATStartByte + 4 * cluster_id;
	if(lseek(global_fd, offset, 0) == -1) { perror("Error: seek error"); exit(-1);}
	if((int)read(global_fd, &buf, 4) == -1){ perror("Error: read"); exit(-1);}

	// if FAT entry is occupied (not zero), than recover failed
	if(buf != 0) return FLAG_FAT_CHANGE_FAIL;

	if(lseek(global_fd, offset, 0) == -1) { perror("Error: seek error"); exit(-1);}
	if(write(global_fd, &next_cluster_id, 4) == -1) { perror("Error: fail to write"); exit(-1);}

	return FLAG_FAT_CHANGE_SUCC;
}

void recoverFile(char * filePath, unsigned long start_cluster_id){
//	unsigned long x = 
}

unsigned long getFileSize(char * filePath, unsigned long startByte){
	return 0;
}

unsigned long printOneInfo(char* filePath, unsigned long id, unsigned long startByte, int subFlag, char* parentDirName){
	// Example: 1, MAKEFILE, 21, 11
	struct DirEntry * tmp = (struct DirEntry *)malloc(DIRENT_SIZE);
	
	if(lseek(global_fd, startByte, 0) == -1) { perror("Error: printOneInfo()"); exit(-1);}
	if(read(global_fd, (void *) tmp, DIRENT_SIZE) == -1) { perror("Error: printOneInfo()"); exit(-1);}
	
	// Requirement A: check if this is the last entry in Root directory
	if((tmp->DIR_Name[0] == 0) && (tmp->DIR_Name[1] == 0)) {	// 0  <==> 0x00
		if(debugFlag) printf("\t---> FLAG_EOF reached \n");
		return FLAG_EOF;
	}

	// Requirement B: check if this entry is deleted or not.
	if(tmp->DIR_Name[0] == 229){	// 229 <==> 0xE5
		if(debugFlag) printf("\t---> FLAG_Not_Printed reached\n");
		return FLAG_Not_Printed; 
	}

	// Requirement C: check if this is LFN or not.
	if(tmp->DIR_Attr == 15){	// 15 <==> 0x0F ==> Long File Name, just ignore
		if(debugFlag) {
			char fileName_tmp[12];
			getFileName(filePath, startByte, fileName_tmp);
			printf("\t---> LFN reached: %s\n", fileName_tmp);
		}
		return FLAG_Not_Printed;
	}

	// Requirement D: 
	// File => FileName.Extension
	unsigned long clusNum = getClusNum(tmp->DIR_FstClusHI, tmp->DIR_FstClusLO);	
	
	char fileName[12];
	getFileName(filePath, startByte, fileName);	

	if(tmp->DIR_Attr / 16 % 2 == 0){ // xxx0 xxxx ==> this is a File.
		if(subFlag) printf("%ld, %s/%s, %ld, %ld\n", id, parentDirName, fileName, tmp->DIR_FileSize, clusNum);
		else printf("%ld, %s, %ld, %ld\n", id, fileName, tmp->DIR_FileSize, clusNum);
		return FLAG_FILE;
	}else{	
		// Folder
		//    => Folder/, 0, StartClus
		//    => Folder/., 0, StartClus => printed later
		//    => Folder/.., 0, 0	=> printed later
		if(subFlag) printf("%ld, %s/%s, 0, %ld\n", id, parentDirName, fileName, clusNum);
		else printf("%ld, %s/, 0, %ld\n", id, fileName, clusNum);
		return clusNum;
	}
	/* IMPORTANT Note for return value*/
	// return value:
	//	(a) FLAG_EOF: last entry in Root directory
	//	(b) FLAG_Not_Printed: this entry is not printed and do not increase id.
	//	(c) FLAG_FILE: this entry is a file
	//	(d) Values >= 0: this is a sub-directory and its starting cluster is Return Value. 

	if (debugFlag) printf("WTF\n");
}

void getFileName(char* filePath, unsigned long startByte, char* fileName){
	struct DirEntry * tmp = (struct DirEntry *) malloc(DIRENT_SIZE);
	if(lseek(global_fd, startByte, 0) == -1) { perror("Error: getFileName()"); exit(-1);}
	if(read(global_fd, (void *) tmp, DIRENT_SIZE) == -1) { perror("Error: getFileName()"); exit(-1);}
	
	//char fileName[12];
        int j = 0, k = 0;
        while(tmp->DIR_Name[j] != 32 && j < 8) 
		fileName[k++] = tmp->DIR_Name[j++];
        if(tmp->DIR_Attr / 16 % 2 == 0) 
		fileName[k++] = '.';
        j = 8;
        while(tmp->DIR_Name[j] != 32 && j < 11) 
		fileName[k++] = tmp->DIR_Name[j++];
        for ( ; k < 12; k++)
		fileName[k] = '\0';

	free(tmp);	// ======> tmp is only used inside this function.
}

unsigned long getClusNum(unsigned short hi, unsigned short lo){
	// Covert to Cluster Number from its Higher and Lower 2 bytes
	unsigned long f_cluster = 0;
	f_cluster = hi*256 + lo;
	return f_cluster;
}

unsigned long getNameExtParts(char* filePath, unsigned long startByte, char * NamePart, char * ExtPart){
	struct DirEntry * tmp = (struct DirEntry *) malloc(DIRENT_SIZE);

	if(lseek(global_fd, startByte, 0) == -1) { perror("Error: getNameExtParts()"); exit(-1);}
	if(read(global_fd, (void *) tmp, DIRENT_SIZE) == -1) { perror("Error: getNameExtParts()"); exit(-1);}	

	int j, k;
	for ( j = 0, k = 0; j < 8; j++){
		if( tmp->DIR_Name[j] != 32) NamePart[k++] = tmp->DIR_Name[j];
		else break;
	}
	for ( ; k < 9; k ++ )
		NamePart[k] = '\0';

	for ( j = 8, k = 0; j < 11; j++){
		if( tmp->DIR_Name[j] != 32) ExtPart[k++] = tmp->DIR_Name[j];
		else break;
	}
	for ( ; k < 4; k ++ ) 
		ExtPart[k] = '\0';
	
	unsigned long clusterNum = getClusNum( tmp->DIR_FstClusHI, tmp->DIR_FstClusLO);
	free(tmp);
	return clusterNum;
}

unsigned long getStartByte(unsigned long clusternum){
	// Data clusters start at data area and the 1st cluster is Cluster 2.
	return (global_RootStartByte + (clusternum - 2) * global_SecPerClus * global_BytesPerSec);
}

unsigned long getNextClusterNum(unsigned long currentClusterNum){
	unsigned long tmp_byte = global_FATStartByte + 4 * currentClusterNum;
	unsigned long a;	// unsigned long => 4 bytes
//	printf("size of unsigned short = %d\n", sizeof(a));
	if(lseek(global_fd, tmp_byte, 0) == -1) { perror("Error: getNextClusterNum()"); exit(-1);}
	if(read(global_fd, (void *) &a, 4) == -1) { perror("Error: getNextClusterNum()"); exit(-1);}	
	if (a >= 268435448) return -1;
	else return a;
}

void printAllInfo(char* filePath){
	unsigned long current_byte;// = global_RootStartByte;
	unsigned long current_id = 1;
	unsigned long cluster_id = 2; // Root dir starts at cluster 2
	unsigned long ret_value;
	char parentFileName[12];
	unsigned long tmp_num_of_entries;
	unsigned long tmp_current_byte;
	unsigned long tmp_cluster_id;
	int tmp_EOF;
	unsigned long num_of_entries = 0; 

	do{
	num_of_entries = 0;
	current_byte = getStartByte(cluster_id);

	while(1){
		num_of_entries ++;
		switch(ret_value = printOneInfo(filePath, current_id, current_byte, 0, NULL)){
			case FLAG_EOF:
				if (debugFlag) printf("\t---> Last entry reached\n");
				return;
			case FLAG_Not_Printed:
				break;
			case FLAG_FILE:
				current_id ++;
				break;
			default:	// This is a folder; ret_val => starting address of cluster
				current_id ++;
				tmp_cluster_id = ret_value;
				tmp_EOF = 0;
				
				if(debugFlag) printf("\t---> current: %ld\n", tmp_cluster_id);
				if(debugFlag) printf("\t---> next   : %ld\n", getNextClusterNum(tmp_cluster_id));		
				do{
					tmp_num_of_entries = 0;
					tmp_current_byte = getStartByte(tmp_cluster_id);
				
					while(1){
						tmp_num_of_entries ++;
						getFileName(filePath, current_byte, parentFileName);
						switch(printOneInfo(filePath, current_id, tmp_current_byte, 1, parentFileName)){
							case FLAG_EOF:
								tmp_EOF = 1;
								break;
							case FLAG_Not_Printed:
								break;
							default:
								current_id ++;
								break;
						}
						if (debugFlag) printf("\t---> Sub Current Byte: %ld\n", tmp_current_byte);
						tmp_current_byte += DIRENT_SIZE;
						if(tmp_EOF == 1) break;
						if(tmp_num_of_entries == 16) break;
					}
				} while(( tmp_cluster_id = getNextClusterNum(tmp_cluster_id)) != -1);

				break;
		}
		current_byte += DIRENT_SIZE;
		if(num_of_entries == 16) break;
	}

	} while( (cluster_id = getNextClusterNum(cluster_id)) != -1);

	return;	
}
