# simplified-FAT
## 1. Description

The program is an attempt to implement a simple file system that allows managing files and directories in a virtual in-memory disk. The file system is based on simplified concepts of a File Allocation Table (<a href="https://en.wikipedia.org/wiki/File_Allocation_Table" target="_blank">**FAT**</a>). It allows the creation of files and directories within this virtual hard disk and the performance of simple read and write operations on files.

The virtual disk is simulated by an array of memory blocks, where each block is a fixed array of bytes. Each block has a block number (from 0 to **MAXBLOCKS**-1). The allocation of a new block to a file is recorded in the **FAT**. The **FAT** is a table (an array of integers) of size **MAXBLOCKS** that acts as a kind of block directory for the complete disk content: it contains an entry for each block and records whether this block is allocated to a file or unused. The **FAT** itself is also stored on this virtual disk at a particular location.

## 2. Interface

The complete public interface of the file system for this virtual disk is the following:

* **void format()** - Creates the initial structure on the virtual disk, writing the FAT and the root directory into the virtual disk

* **MyFILE * myfopen ( const char * filename, const char * mode )** - Opens a file on the virtual disk and manages a buffer for it of size **BLOCKSIZE**, mode may be either “r” for read-only or “w” for read/write/append (default “w”)

* **void myfclose ( MyFILE * stream )** - Closes the file, writes out any blocks not written to disk

* **int myfgetc ( MyFILE * stream )** - Returns the next byte of the open file, or **EOF** (== -1)

* **void myfputc ( int b, MyFILE * stream )** - Writes a byte to the file. Depending on the write policy, either writes the disk block containing the written byte to disk, or waits until block is full

* **void mymkdir ( const char * path )** - The function creates a new directory, using path, e.g. mymkdir(“/first/second/third”) creates directory “third” in parent dir “second”, which is a subdir of directory “first”, and “first is a sub directory of the root directory

* **char ** mylistdir (const char * path)** - Lists the content of a directory and returns a list of strings, where the last element is **NULL**

* **void mychdir ( const char * path )** - The function changes the current directory into an existing directory, using path, e.g. mkdir (“/first/second/third”) creates directory “third” in parent dir “second”, which is a subdir of directory “first”, and “first is a sub directory of the root directory

* **void myremove ( const char * path )** - Removes an existing file, using path, e.g. myremove (“/first/second/third/testfile.txt”)

* **void myrmdir ( const char * path )** - Removes an existing directory, using path, e.g. myrmdir “/first/second/third”) removes directory “third” in parent dir “second”, which is a subdir of directory “first”, and “first is a sub directory of the root directory

* **int nextUnusedBlock ()** - Returns the next unused block number

* **int nextUnusedDir ()** - Returns the next unused directory entry number

* **int fileLocation ( const char\* filename )** - Looks for a file block given a path (file name)

* **void cleanFAT ( int fatEntry )** - Recursively cleans FAT by setting its entries to UNUSED

* **diskblock_t freeDirBlock ()** -  Returns a newly created free directory block

* **int fileEntryIndex (const char \*path)** - Checks if there is an entry that matches the path. If so returns the entry
number and **EOF** otherwise

* **int isDirEmpty (dirblock_t dir)** - Checks whether an entry is empty (unused). Returns **TRUE** if it finds such
entry and **FALSE** otherwise

* **void printContents (char **contentList)** - Prints the contents of a list (in our case files and folder names)

#### Extra functions implemented:

* **void myfvdsys(const char\* sysFile, const char\* vdFile)** - Copies the contents of a virtual disk file to a physical disk file

* **void myfsysvd(const char\* sysFile, const char\* vdFile)** - Copies the contents of a physical disk file to virtual disk file

* **void myfcopy (char\* src, char\* dst)** - Copies content from source to destination file (in the virtual disk)

* **void myfmove (char\* src, char\* dst)** - Moves content from source to destination file (in the virtual disk) and
deletes source file (reuses “myfcopy” and “myremove” functions)

## 3. How to run
#### (!) Before you start make sure u have all the files required in the same directory: filesys.c, filesys.h, shell.c and Makefile.

### On Windows:
* You will need to install C compiler if you don’t have one, such as GCC (https://gcc.gnu.org/). If you are on a Windows computer, you can run the command ‘gcc -v’ to check if it’s already installed.

* Open the command prompt by going to the Start button and typing ‘cmd' in the search or run bar (or click on Command Prompt if provided).

* Change your directory to where you have your C program (in our casefilesys.c, filesys.h, shell.c and Makefile.) using the command ‘cd’. We need to pass the name of the directory in which the program is stored.
  o Example: cd Desktop (if the program is already in the user directory)

* Compile the source code by typing ‘make’ in the Command Prompt (compilation is automatically done by the Makefile). (additional command ‘make clean’ to clean up the compilation files)

* Run the executable file by typing the name of the executable file without the extension (in our case ‘shell’) and hit ‘Enter’.

### On Linux:

* Open the terminal window.

* If you don’t have a compiler already installed you will need to run the following apt-get commands in the terminal to install GNU c/c++ compiler:

```
$ sudo apt-get update
$ sudo apt-get install build-essential manpages-dev
```

* Navigate to the program directory using the command ‘cd’ and type in ‘make' to compile the program (since we have a Makefile). (additional command ‘make clean’ to clean up the compilation files)

4) To run the executable just type in ‘./<name of file>’ (‘./shell’ in our case) and hit ‘Enter’.

### In case Makefile is missing you can compile the program manually using the following commands:

```
$ gcc -c filesys.c
$ gcc -c shell.c
$ gcc -o shell shell.o filesys.o
```
