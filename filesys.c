/* filesys.c
 *
 * provides interface to virtual disk
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "filesys.h"


diskblock_t  virtualDisk [MAXBLOCKS] ;           // define our in-memory virtual, with MAXBLOCKS blocks
fatentry_t   FAT         [MAXBLOCKS] ;           // define a file allocation table with MAXBLOCKS 16-bit entries
fatentry_t   rootDirIndex            = 0 ;       // rootDir will be set by format
direntry_t * currentDir              = NULL ;
fatentry_t   currentDirIndex         = 0 ;


/* writedisk : writes virtual disk out to physical disk
 *
 * in: file name of stored virtual disk
 */

void writedisk ( const char * filename )
{
   printf ( "writedisk> virtualdisk[0] = %s\n", virtualDisk[0].data ) ;
   FILE * dest = fopen( filename, "w" ) ;
   if ( fwrite ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
   fclose(dest) ;

}

void readdisk ( const char * filename )
{
   FILE * dest = fopen( filename, "r" ) ;
   if ( fread ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
      fclose(dest) ;
}


/* the basic interface to the virtual disk
 * this moves memory around
 */

void writeblock ( diskblock_t * block, int block_address )
{
	//printf ( "writeblock> block %d = %s\n", block_address, block->data ) ;
	memmove ( virtualDisk[block_address].data, block->data, BLOCKSIZE ) ;
	//printf ( "writeblock> virtualdisk[%d] = %s / %d\n", block_address, virtualDisk[block_address].data, (int)virtualDisk[block_address].data ) ;
}



/* read and write FAT
 *
 * please note: a FAT entry is a short, this is a 16-bit word, or 2 bytes
 *              our blocksize for the virtual disk is 1024, therefore
 *              we can store 512 FAT entries in one block
 *
 *              how many disk blocks do we need to store the complete FAT:
 *              - our virtual disk has MAXBLOCKS blocks, which is currently 1024
 *                each block is 1024 bytes long
 *              - our FAT has MAXBLOCKS entries, which is currently 1024
 *                each FAT entry is a fatentry_t, which is currently 2 bytes
 *              - we need (MAXBLOCKS /(BLOCKSIZE / sizeof(fatentry_t))) blocks to store the
 *                FAT
 *              - each block can hold (BLOCKSIZE / sizeof(fatentry_t)) fat entries
 */

/* ======================================================================================= D3_D1 FUNCTIONS */

/* implement format()
 */
void format ( )
{
    diskblock_t block ;
    direntry_t  rootDir ;
    int         pos             = 0 ;
    int         fatentry        = 0 ;
    int         fatblocksneeded =  (MAXBLOCKS / FATENTRYCOUNT ) ;

    /* prepare block 0 : fill it with '\0',
    * use strcpy() to copy some text to it for test purposes
    * write block 0 to virtual disk
    */
    memset(block.data, '\0', BLOCKSIZE);
    strcpy (block.data, "CS3026 Operating Systems Assessment 2019\0" );
    writeblock( &block, 0);

    /* prepare FAT table
    * it occupies blocks 1 and 2
    * write FAT blocks to virtual disk
    */
    FAT[0] = ENDOFCHAIN;
    FAT[1] = 2;
    FAT[2] = ENDOFCHAIN;
    FAT[3] = ENDOFCHAIN;
    for (int i = 4; i < MAXBLOCKS; i++) FAT[i] = UNUSED; /* set the other FAT entries to UNUSED */
    copyFAT(); /* copy the FAT table */

    /* prepare root directory
    * write root directory block to virtual disk
    */
    diskblock_t rootBlock;
    rootBlock = freeDirBlock(); /* assign a free directory block to the root block */

    rootDirIndex = fatblocksneeded + 1; /* set the root directory index after the FAT blocks (3) */

    writeblock( &rootBlock, rootDirIndex);

    /*
    * change the current directory to root
    */
    currentDirIndex = rootDirIndex;
    currentDir = virtualDisk[currentDirIndex].dir.entrylist;
}



/* ======================================================================================= C3_C1 FUNCTIONS */

/*
 * Function:  myfopen
 * --------------------
 * opens a file on the virtual disk and manages a buffer for it of size BLOCKSIZE, mode may be
 * either "r" for read-only or "w" for read/write/append (default "w")
 *
 * returns: MyFILE (file descriptor) structure
 */

MyFILE * myfopen ( const char * filename, const char * mode ) {

	if (!(strcmp(mode, "r") == 0 || strcmp(mode, "w") == 0)) return FALSE; /* return if not in correct mode */

    fatentry_t initDir = currentDirIndex; /* entry variable to hold initial directory */

	if (filename[0] == '/') currentDirIndex = rootDirIndex; /* if the path is absolute then set the current
	directory to root */

	char str[strlen(filename)]; /* tokenization string */
	char *rest = str; /* pointer to remaining part of the string */
	strcpy(rest, filename); /* copy the path to the string (to be used for string tokenization) */

    char *token; /* token pointer */

	while((token=strtok_r(rest, "/", &rest))){ /* tokenize the string and go over the tokens */

		if(*rest){ /* if there are still tokens remaining */

			int entryIndex = fileEntryIndex(token); /* get the file entry index */
			if (entryIndex == EOF) exit(0); /* if the entry index is EOF then exit */
			currentDirIndex = virtualDisk[currentDirIndex].dir.entrylist[entryIndex].firstblock; /* set the current
			directory index to the file entry index */
		}
		else{

			MyFILE *file = malloc(sizeof(MyFILE)); /* allocate memory for the file */
			file->pos = 0; /* reset file position to 0 */
			memset(file->buffer.data, '\0', BLOCKSIZE); /* set all elements to "\0" */
			strcpy(file->mode, mode); /* set file mode */

			if(strcmp(mode, "r") == 0){ /* if the opened file is in read mode */

				if(fileLocation(token) == FALSE) exit(0); /* exit if file not found */

				file->blockno = fileLocation(token); /* set file block number */
				return file; /* return the file */
			}
			if(fileLocation(token) != FALSE) { /* if we find the file */

				int path = fileLocation(token); /* set to the block number of the file */

				if(path <= BLOCKSIZE/FATENTRYCOUNT) exit(0); /* if the path block number is less than the root
				directory number, then exit */

				cleanFAT(path); /* adjust FAT chain */

				copyFAT(); /* copy changes to FAT */

				/*
				 * empty all directory entries to "overwrite" the file
				 */
				diskblock_t block;
				fatentry_t nextDir = rootDirIndex; /* start from the root directory */

				while(nextDir != ENDOFCHAIN) { /* while we haven't reached EOC */

					currentDirIndex = nextDir; /* set the current directory index */

					moveToBlock(&block, nextDir); /* put the virtual disk data to the buffer block */

					currentDir = block.dir.entrylist; /* assign buffer block entry list to the current entry list */

					for(int i = 0; i <= block.dir.nextEntry; i++) { /* go through the entries in the buffer
                            block */

						if(strcmp(currentDir[i].name, filename) == 0) { /* if the current directory name is the
                                same as the file name */

							/*
							* clean the entry and break the loop
							*/
							currentDir[i].firstblock = nextUnusedBlock();
							currentDir[i].entrylength = sizeof(currentDir);
							break;
						}
					}

					nextDir = FAT[nextDir]; /* advance to the next FAT byte */
				}

				int location = nextUnusedDir();
				currentDir[location].unused = FALSE; /* set the directory as used */
			}
			else{ /* if file not found */

				int nextUnused = nextUnusedBlock(); /* get the next unused block */

				diskblock_t block; /* buffer block */

				moveToBlock(&block, currentDirIndex); /* transfer current directory block to buffer block */

				direntry_t *unusedEntry = calloc(1, sizeof(direntry_t)); /* allocate memory for the new entry */

				/*
                 * initialize the unused entry as a file
                 */
				unusedEntry->isdir = FALSE;
				unusedEntry->unused = FALSE;
				unusedEntry->firstblock = nextUnused;

				int entryNumber = nextUnusedDir(); /* get unused directory entry number */

				currentDir = block.dir.entrylist; /* assign buffer block entry list to the current entry list */

				currentDir[entryNumber] = *unusedEntry; /* place the unused entry to the block entry list */
				strcpy(currentDir[entryNumber].name, token); /* set its name to the token */

				writeblock(&block, currentDirIndex); /* write the block to the virtual disk */

				file->blockno = nextUnused; /* set the file block number */

				currentDirIndex = initDir; /* go back to initial directory */
				return file; /* return the file */
			}
		}
	}
}


/*
 * Function:  myfputc
 * --------------------
 * writes a byte to the file
 */

void myfputc ( int b, MyFILE * stream ) {

	if (strcmp(stream->mode, "r") == FALSE) return; /* return if the file is in read mode */

	if (stream->pos >= BLOCKSIZE){ /* if the write pointer is BLOCKSIZE */

		int indexOfUnusedBlock = nextUnusedBlock(); /* get the next unused block */
		FAT[stream->blockno] = indexOfUnusedBlock; /* assign the next FAT entry to point to the next unused block */

		stream->pos = 0; /* reset the write pointer to the beginning of the stream */
		writeblock(&stream->buffer, stream->blockno); /* write the stream buffer to the virtual disk */

		memset(stream->buffer.data, '\0', BLOCKSIZE); /* clean the buffer data */
		stream->blockno = indexOfUnusedBlock; /* assign the next unused block to buffer */
	}

	stream->buffer.data[stream->pos++] = b; /* write the character to the file and increment the write pointer
	 position */
}


/*
 * Function:  myfgetc
 * --------------------
 * reads from file
 *
 * returns: the next byte of the open file, or EOF (EOF == -1)
 */

int myfgetc ( MyFILE * stream ) {

	int currentBlockNo = stream->blockno;   /* variable which holds the current block number */

	if(currentBlockNo == ENDOFCHAIN || strcmp(stream->mode, "r") == TRUE) /* check whether the block number is EOC
        or the file is in read mode */
		return EOF;

	if (stream->pos % BLOCKSIZE == 0){ /* if the writer pointer has reached BLOCKSIZE */

		stream->blockno = FAT[currentBlockNo]; /* get next buffer block number from the FAT block chain */

		memcpy(&stream->buffer, &virtualDisk[currentBlockNo], BLOCKSIZE); /* copy the virtual disk data to the
		buffer block used for reading */

		stream->pos = 0; /* reset character writer pointer */
	}

	return stream->buffer.data[stream->pos++]; /* return the read character and increment write pointer */
}


/*
 * Function:  myfclose
 * --------------------
 * closes a file, writes out any blocks not written to disk
 */

void myfclose ( MyFILE * stream ) {

    if(strcmp(stream->mode, "w") == 0){ /* if the file is in write mode */

        int nextUnused = nextUnusedBlock(); /* get the next unused block */

        FAT[stream->blockno] = nextUnused; /* extend the FAT block chain with the next unused block */

        writeblock(&stream->buffer, stream->blockno); /* write the buffer block */
    }
}


/* ======================================================================================= B3_B1 FUNCTIONS */


/*
 * Function:  mymkdir
 * --------------------
 * creates a new directory, using path, e.g. mymkdir ("/first/second/third") creates
 * directory "third" in parent directory "second", which is a sub directory of directory "first",
 * and "first" is a sub directory of the root directory
 */

void mymkdir(char *path){

	diskblock_t block; /* declare a buffer block */

    fatentry_t initDir = currentDirIndex; /* entry variable to hold initial directory */

	if (path[0] == '/') currentDirIndex = rootDirIndex; /* if the path is absolute then set the current directory to root */

	char str[strlen(path)]; /* tokenization string */
	char *rest = str; /* pointer to remaining part of the string */
	strcpy(rest, path); /* copy the path to the string (to be used for string tokenization) */

    char *token; /* token pointer */

	while((token = strtok_r(rest, "/", &rest))){ /* go through every string token */

            int entryIndex = fileEntryIndex(token); /* get file entry index */
            if(entryIndex > EOF){ /* if the entry index is not EOF */
                currentDirIndex = virtualDisk[currentDirIndex].dir.entrylist[entryIndex].firstblock; /* set the
                current directory index to the file entry index */
            }
            else {
                block = freeDirBlock(); /* set buffer block to free directory block */

                int entryIndex = nextUnusedDir(); /* get the next unused directory location where the new directory
                 will be placed */

                strcpy(currentDir[entryIndex].name, token); /* place the token as the directory entry name */

                /*
                 * initialize the directory entry and write it to the virtual disk
                 */
                currentDir[entryIndex].isdir = TRUE;
                currentDir[entryIndex].unused = FALSE;
                currentDir[entryIndex].firstblock = nextUnusedBlock();
                currentDirIndex = currentDir[entryIndex].firstblock;

                writeblock(&block, currentDir[entryIndex].firstblock); /* assign a clean block to the directory */
            }

	}
	currentDirIndex = initDir; /* revert the directory to initial directory index */
}


/*
 * Function:  mylistdir
 * --------------------
 * copies the content of the FAT into one or more blocks
 */

char **mylistdir(char *path) {

    fatentry_t initDir = currentDirIndex; /* entry variable to hold initial directory */

	if (path[0] == '/') currentDirIndex = rootDirIndex; /* if the path is absolute then set the current directory to root */

	char str[strlen(path)]; /* tokenization string */
	char *rest = str; /* pointer to remaining part of the string */
	strcpy(rest, path); /* copy the path to the string (to be used for string tokenization) */

    char *token; /* token pointer */

    while((token=strtok_r(rest, "/", &rest))){

        /*
         * get the entry index of the file which matches the token
         * if there is no such match then exit
         */
        int entryIndex = fileEntryIndex(token);
        if (entryIndex == EOF) exit(0);
        currentDirIndex = virtualDisk[currentDirIndex].dir.entrylist[entryIndex].firstblock; /* set the current
        directory index to the file entry index */
    }

    diskblock_t block; /* create a buffer block */

    fatentry_t nextDir = currentDirIndex; /* FAT buffer entry to current directory */

    char **contentList = malloc(sizeof(char*)); /* array of file and folder names */
    int currIndex = 0; /* shows the index we are currently using in the contentList (starting at 0) */

    while(nextDir != ENDOFCHAIN){ /* while the FAT buffer entry is not EOC */

        moveToBlock(&block, nextDir); /* move the FAT block to the buffer block */

        currentDir = block.dir.entrylist; /* assign buffer block entry list to the current entry list */

        for(int i = 0; i <= block.dir.nextEntry; i++){ /* iterate over the entries in the directory block */

            if(currentDir[i].unused == FALSE){ /* if an used entry is found */

                /*
                 * allocate memory of size file name length
                 * copy the file name to the list where we just allocated memory
                 * extend the list length so that we have an entry for EOF
                 */
                contentList[currIndex] = calloc(sizeof(currentDir[i].name), sizeof(char));
                strcpy(contentList[currIndex], currentDir[i].name);
                contentList = realloc(contentList, (((++currIndex)+1)*sizeof(char*)));
            }
        }

        nextDir = FAT[nextDir]; /* advance to the next FAT entry */
    }

    contentList[currIndex] = NULL; /* set last element to NULL (EOF entry) */
    currentDirIndex = initDir; /* revert to the initial directory */

    return contentList; /* return the list with the contents of the directory */
}

/* ======================================================================================= A5_A1 FUNCTIONS */

/*
 * Function:  mychdir
 * --------------------
 * change into an existing directory, using path, e.g. mychdir ("/first/second/third")
 * changes the directory to "third" in parent directory "second", which is a subdirectory of
 * directory "first", and "first" is a sub directory of the root directory
 */

void mychdir(char *path) {

    /*
     * if the path is "root"
     * then return to the root directory
     */
    if (strcmp(path, "root") == 0){
        printf("Returning to root...\n");
        currentDirIndex = rootDirIndex;
        currentDir->firstblock = rootDirIndex;
        return;
    }

    /*
     * if the path is "/"
     * then return to the root directory
     */
    if (path[0] == '/') {
        currentDirIndex = rootDirIndex;
        currentDir->firstblock = rootDirIndex;
        printf("\nCurrent directory -> root\n");
    }

    else if(strcmp(path, "..") == 0 && currentDirIndex != rootDirIndex) { /* does't work from every directory */
        currentDirIndex--;
        printf("Returning to dir %d...\n", currentDirIndex);
        currentDir->firstblock = currentDirIndex;
        return;
    }

    if (strcmp(path, ".") == 0) { /* don't change directory */
        return;
    }

    /*
     * else go over the path tokens
     */
	char str[strlen(path)]; /* tokenization string */
	char *rest = str; /* pointer to remaining part of the string */
	strcpy(rest, path); /* copy the path to the string (to be used for string tokenization) */

    char *token; /* token pointer */
    int entryIndex; /* to hold the file entry index */

	while((token=strtok_r(rest, "/", &rest))) { /* go over the tokens of the path */

        entryIndex = fileEntryIndex(token); /* get the file entry index */
        if (entryIndex == EOF) return; /* return if it's EOF */
        currentDirIndex = virtualDisk[currentDirIndex].dir.entrylist[entryIndex].firstblock; /* set the current
        directory index to the file entry index */
        currentDir->firstblock = currentDirIndex; /* switch the global currentDir to the current directory index */
        printf("\nCurrent directory -> %s\n", token);
    }

}


/*
 * Function:  myremove
 * --------------------
 * removes an existing file, using path, e.g. myremove ("/first/second/third/testfile.txt")
 * removes "testfile.txt" from "third" in parent directory "second", which is a subdirectory of
 * directory "first", and "first" is a sub directory of the root directory
 */

void myremove (char *path) {

	if(strcmp(path, "/") == 0) return; /* since root directory cannot be removed */

	fatentry_t initDir = currentDirIndex; /* entry variable to hold initial directory */

	if (path[0] == '/') currentDirIndex = rootDirIndex; /* if the path is absolute then set the current directory to root */


	char str[strlen(path)]; /* tokenization string */
	char *rest = str; /* pointer to remaining part of the string */
	strcpy(rest, path); /* copy the path to the string (to be used for string tokenization) */

    char *token; /* token pointer */

	while((token=strtok_r(rest, "/", &rest))){ /* go over the tokens of the path */

        if (*rest) { /* if there are still tokens remaining */

            currentDirIndex = virtualDisk[currentDirIndex].dir.entrylist[fileEntryIndex(token)].firstblock; /* set
            the current directory index to the file entry index */
        }
        else {
            int toRemove = fileEntryIndex(token); /* get the file (to be removed) index at token */

            /*
            * create a buffer block and move the current directory to it
            */
            diskblock_t block;
            moveToBlock(&block, currentDirIndex);

            /*
             * initialize a clean entry
             */
            direntry_t *unusedEntry = calloc(1, sizeof(direntry_t));
            unusedEntry->isdir = FALSE;
            unusedEntry->unused = TRUE;
            unusedEntry->filelength = 0;
            unusedEntry->firstblock = ENDOFCHAIN;

            /*
            * take the first block number of the entry to be removed
            * adjust the FAT chain and set the block number to NULL
            */
            int rmBlockNo = block.dir.entrylist[toRemove].firstblock;
            cleanFAT(rmBlockNo);

            block.dir.entrylist[toRemove] = *unusedEntry; /* put the clean entries to the buffer block */

            copyFAT(); /* copy changes to FAT */
            writeblock(&block, currentDirIndex); /* write the buffer block to the virtual disk */
        }

	}
	currentDirIndex = initDir; /* go back to initial directory */
}


/*
 * Function:  myrmdir
 * --------------------
 * removes an existing directory, using path, e.g. myrmdir ("/first/second/third")
 * removes directory "third" in parent directory "second", which is a subdirectory of
 * directory "first", and "first" is a sub directory of the root directory
 */

void myrmdir (char *path) {

	if(strcmp(path, "/") == 0) return; /* since we cannot remove the root directory */

    fatentry_t initDir = currentDirIndex; /* entry variable to hold initial directory */

	if (path[0] == '/') currentDirIndex = rootDirIndex; /* if the path is absolute then set the current directory
	to root */

	char str[strlen(path)]; /* tokenization string */
	char *rest = str; /* pointer to remaining part of the string */
	strcpy(rest, path); /* copy the path to the string (to be used for string tokenization) */

    char *token; /* token pointer */

	while ((token=strtok_r(rest, "/", &rest))){ /* go over the path tokens */

        int currLoc = fileEntryIndex(token); /* get the file entry number */

        if (*rest) /* if there are still remaining path directories then enter the one we are considering */
            currentDirIndex = virtualDisk[currentDirIndex].dir.entrylist[currLoc].firstblock; /* set the current
            directory index to the file entry index */
		else{

			int toRemove = virtualDisk[currentDirIndex].dir.entrylist[currLoc].firstblock; /* directory entry index
			which we will be removing */

			diskblock_t block; /* buffer block */

			moveToBlock(&block, toRemove); /* move directory data to buffer block so we can delete it */

			if (isDirEmpty(block.dir))  /* if the directory is empty */
				myremove(token); /* reuse "myremove" to delete the directory only if it is empty */
			else
				printf("\nCannot delete! Directory not empty!!!"); /* error message
				to print if directory is not empty */
		}
	}
	currentDirIndex = initDir; /* go back to initial directory */
}


/* ======================================================================================= HELPER FUNCTIONS */

/*
 * Function:  printBlock
 * --------------------
 * prints a block number and data (for testing)
 */

void printBlock ( int blockIndex ){

   printf ( "virtualdisk[%d] = %s\n", blockIndex, virtualDisk[blockIndex].data ) ;
}


/*
 * Function:  copyFAT
 * --------------------
 * copies the content of the FAT into one or more blocks
 */

void copyFAT () {

    diskblock_t block;

	for ( int i = 0; i < MAXBLOCKS/FATENTRYCOUNT; i++) {

        for ( int j = 0; j < FATENTRYCOUNT; j++) block.fat[j] = FAT[j+(FATENTRYCOUNT*i)] ;

        writeblock( &block, i+1);

	}

}

/*
 * Function:  moveToBlock
 * --------------------
 * move virtual disk data to the block data
 */

void moveToBlock ( diskblock_t * block, int block_address ){

	memmove(block->data, virtualDisk[block_address].data, BLOCKSIZE);
}


/*
 * Function:  nextUnusedBlock
 * --------------------
 * get the next unused block number
 *
 * returns: the block number to be used
 */

int nextUnusedBlock () {

	for (int i = 4; i < MAXBLOCKS; i++) /* start at 4 since previous blocks are
        occupied by the FAT and root directory */
		if (FAT[i] == UNUSED){	/* set the next unused block to EOC */
            FAT[i] = ENDOFCHAIN;
            copyFAT();
            return i; /* return the unused block number */
        }
	return -1;
}


/*
 * Function:  nextUnusedDir
 * --------------------
 * get the next unused directory entry number
 *
 * returns: the unused directory entry number
 */

int nextUnusedDir () {

    /*
     * create a buffer block and set a buffer FAT entry to the current directory index
     */
	diskblock_t block;
	fatentry_t nextDir = currentDirIndex;

	while(nextDir != ENDOFCHAIN){ /* while the FAT entry buffer hasn't reached an EOC */

		moveToBlock(&block, nextDir); /* move the FAT entry data to the buffer block */

        /*
		 * go through the buffer block directory entries
		 * check if the block in the entry list is unused
		 * if so set the current directory to that FAT entry
		 * return the block number of that entry
		 */
		for (int i = 0; i <= block.dir.nextEntry; i++) {

			if(block.dir.entrylist[i].unused == TRUE) {
				currentDirIndex = nextDir;
				currentDir = virtualDisk[currentDirIndex].dir.entrylist;
				return i;
			}
		}

		nextDir = FAT[nextDir]; /* advance to the next FAT entry */
	}

	int nextUnused = nextUnusedBlock(); /* get the next unused block number */
	FAT[currentDirIndex] = nextUnused; /* include it to the FAT chain */

    currentDirIndex = nextUnused; /* move the current directory to the newly placed FAT entry */
	memset(block.data, '\0', BLOCKSIZE); /* initialize buffer block to 0 */

	writeblock(&block, currentDirIndex); /* write the block to the virtual disk */
    currentDir = virtualDisk[currentDirIndex].dir.entrylist; /* currentDir entry now points to the newly placed
    FAT entry entry list */

	return 0; /* the entry number returned is 0 since we extended the FAT chain */
}


/*
 * Function:  fileLocation
 * --------------------
 * looks for a file block given a path (file name)
 *
 * returns: the block of the file if the specified path exists, else return 0
 */

int fileLocation (const char *filename) {

	/*
     * create a buffer block and set a buffer FAT entry to the current directory index
     */
	diskblock_t block;
	fatentry_t nextDir = currentDirIndex;

	while(nextDir != ENDOFCHAIN) { /* while the FAT entry buffer hasn't reached an EOC */

		currentDirIndex = nextDir; /* shift the current directory to the next one */

        moveToBlock(&block, nextDir); /* move virtual disk block data to buffer block */

		currentDir = block.dir.entrylist; /* set the global currentDir to the buffer block entry list */

		/*
		 * go through the buffer block directory entries
		 * check if the name of an entry matches the file name
		 * if so return the block of that entry
		 */
		for(int i = 0; i <= block.dir.nextEntry; i++){

			if(strcmp(currentDir[i].name, filename) == 0) return currentDir[i].firstblock;
		}

		nextDir = FAT[nextDir]; /* advance to the next FAT entry */
	}

	return FALSE; /* if there is no such file */
}


/*
 * Function:  cleanFAT
 * --------------------
 * recursively cleans FAT by setting FAT entries to UNUSED
 */

void cleanFAT (int fatEntry) {

    /*
     * create a buffer block and initialize it to 0
     */
    diskblock_t block;
    memset(block.data, '\0', BLOCKSIZE);

	int nextFatEntry = FAT[fatEntry]; /* get the next FAT entry */

	if (nextFatEntry != ENDOFCHAIN) cleanFAT(nextFatEntry); /* if the next fat entry is not EOC, recursively call
	cleanFAT() on that entry */

	writeblock(&block, fatEntry); /* provide a new empty block by overwriting the FAT entry with EOC in the virtual
	disk */

	FAT[fatEntry] = UNUSED; /* set next FAT entry to unused */
}


/*
 * Function:  freeDirBlock
 * --------------------
 * creates a free directory block
 *
 * returns: the newly created free directory block
 */

diskblock_t freeDirBlock () {

    /*
     * buffer block initialized to 0
     */
    diskblock_t block;
    memset(block.data, '\0', BLOCKSIZE);

	/*
	 * make it a directory block
	 */
	block.dir.nextEntry = FALSE;
	block.dir.isdir = TRUE;

	/*
	 * create and initialize a free entry
	 */
	direntry_t *freeEntry = calloc(1, sizeof(direntry_t));
	freeEntry->isdir = FALSE;
	freeEntry->unused = TRUE;

	/*
	 * go through the entry list and set all the entries to the free entry
	 */
	for(int i = 0;i < DIRENTRYCOUNT; i++)
	{
		block.dir.entrylist[i] = *freeEntry;
		block.dir.nextEntry = i;
	}

	return block; /* return the free directory block */
}


/*
 * Function:  fileEntryIndex
 * --------------------
 * checks if there is an entry that matches the path
 *
 * returns: the entry number or EOF
 */

int fileEntryIndex (const char *path) {

	/*
	 * buffer block and FAT entry
	 */
	diskblock_t block;
	fatentry_t nextDir = currentDirIndex;

	while(nextDir != ENDOFCHAIN){ /* while current FAT entry is not EOC */

		moveToBlock(&block, currentDirIndex); /* move the virtual disk directory data block to the buffer data block */

		/*
		 * check directory entries until we find one with a name that matches the path
		 * if found set the current directory to the path and return the block number
		 */
		for(int i = 0; i <= block.dir.nextEntry; i++){

			if (strcmp(block.dir.entrylist[i].name, path) == 0){

				currentDirIndex = nextDir;
				currentDir = block.dir.entrylist;

				return i;
			}
		}
		nextDir = FAT[nextDir]; /* move to next FAT entry */
	}
	return EOF; /* if no match is found return EOF */
}


/*
 * Function:  isDirEmpty
 * --------------------
 * checks whether an entry is empty (unused)
 *
 * returns: boolean
 */

int isDirEmpty (dirblock_t dir) {

	if(dir.nextEntry == 0) return TRUE;

	for(int i = 0; i <= dir.nextEntry; i++)
		if(dir.entrylist[i].unused == FALSE)
			return FALSE;

	return TRUE;
}


/*
 * Function:  printContents
 * --------------------
 * prints the contents of a list (in our case files and folder names)
 */

void printContents (char **contentList) {

    printf("\nDirectory contents: \n\n");
	for(int i = 0; contentList[i] != '\0'; i++) printf("\t%s\t||", contentList[i]);

	printf("\n\n");
}

/* ======================================================================================= EXTRA FUNCTIONS */


/*
 * Function:  myfsysvd
 * --------------------
 * copies the contents of a virtual disk file to a physical disk file
 */

void myfvdsys(const char* sysFile, const char* vdFile) {

    /*
     * open a system system file in write mode
     * open a virtual disk file in read mode
     */
    FILE *systemFile = fopen(sysFile, "w");
    MyFILE *virtualFile = myfopen(vdFile, "r");

    while(TRUE){
        char ch = myfgetc(virtualFile);
        if (ch == EOF) break; /* if no more content to be copied then break the loop */
        fputc(ch, systemFile);
    }

    myfclose(virtualFile);
    fclose(systemFile);
}

/*
 * Function:  myfsysvd
 * --------------------
 * copies the contents of a physical disk file to virtual disk file
 */

void myfsysvd(const char* sysFile, const char* vdFile) {
		FILE *systemFile = fopen(sysFile, "r");
		MyFILE *virtualFile = myfopen(vdFile, "w");

        /*
         * transfer data from system file to virtual disk file
         */
        while(TRUE){
            char ch = fgetc(systemFile);
            if (ch == EOF) break; /* if no more content to be copied then break the loop */
            myfputc(ch, virtualFile);
        }

		myfclose(virtualFile);
		fclose(systemFile);
}


/*
 * Function:  myfcopy
 * --------------------
 * copies content from source to destination (in the virtual disk)
 */

void myfcopy (char* src, char* dst) {

	MyFILE *source = myfopen(src, "r");
	MyFILE *destination = myfopen(dst, "w");

	while (TRUE) {

		char ch = myfgetc(source);
		if( ch == '\0') break; /* if no more content to be copied then break the loop */
		myfputc(ch, destination);
	}

	myfclose(source);
	myfclose(destination);
}


/*
 * Function:  myfmove
 * --------------------
 * moves content from source to destination file (in the virtual disk) and deletes source file
 * (reusing myremove function)
 */

void myfmove (char* src, char* dst) {

	myfcopy(src, dst);
	myremove(src); /* removes the source */
}
