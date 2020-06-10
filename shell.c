/* shell.c
 *
 * provides main function
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "filesys.h"

void d3_d1 () {

    format(); /* format the virtual disk */

    printBlock(0); /* print the 1st FAT block */

    writedisk("virtualdiskD3_D1"); /* write the virtual disk to the physical disk */
}

void c3_c1 () {
    format(); /* format the virtual disk */
    MyFILE * testFile = myfopen("testfile.txt", "w"); /* open a virtual file in write mode */

    char string[] = "ABCDEFGHIJKLMNOPQRSTUVWXWZ"; /* string used as data to fill the file */

    /*
     * fill the file with data of size 4 * BLOCKSIZE (4*1024 bytes or 4 KB)
     */
    for (int i = 0; i < 4* BLOCKSIZE; i++)
        myfputc(string[rand() % (int) (sizeof string - 1)], testFile); /* randomize data */
    myfclose(testFile); /* close the file */

    FILE *systemFile = fopen("testfileC3_C1_copy.txt", "w"); /* open a file on the hard disk
    (system file) */

    MyFILE *readFile = myfopen("testfile.txt", "r"); /* now open the same file in read mode */

    /*
     * print out the content of the file
     */
    for (int i = 0; i < 4*BLOCKSIZE; i++) {

        char ch = myfgetc(readFile);
        if (ch == EOF)
            break;
        printf("%c", ch);
        fprintf(systemFile, "%c", ch);
    }

    myfclose(readFile); /* close the virtual file */
    fclose(systemFile); /* close the system file */
    printf("\n\n");

    printBlock(0);
    writedisk("virtualdiskC3_C1"); /* write the virtual disk to the physical disk */
}

void b3_b1 () {

	const char *text = "THIS IS B3_B1!!!"; /* string to populate the opened files */
	char **dirContent; /* list to store directory contents */
	format(); /* format the virtual disk */

	mymkdir("/myfirstdir/myseconddir/mythirddir");

    /*
     * get the contents of "seconddir" and print them out
     */
	dirContent = mylistdir("/myfirstdir/myseconddir");
	printContents(dirContent);
	writedisk("virtualdiskB3_B1_a"); /* write the virtual disk to the physical disk */

	MyFILE *testFile = myfopen("/myfirstdir/myseconddir/testfile.txt", "w"); /* open a virtual file
	in write mode */

    /*
     * fill the file with text pointer data
     */
	for (int i = 0; i < strlen(text)*10; i++)
        myfputc(text[i%strlen(text)], testFile);
	myfclose(testFile);

    /*
     * get the contents of "seconddir" and print them out
     */
	dirContent = mylistdir("/myfirstdir/myseconddir");
	printContents(dirContent);

	writedisk("virtualdiskB3_B1_b"); /* write the virtual disk to the physical disk */
}

void a5_a1 () {

    const char *text = "THIS IS A5_A1!!!"; /* string to populate the opened files */
    char **dirContent; /* list to store directory contents */

    format(); /* format the virtual disk */

    mymkdir("/firstdir/seconddir"); /* create "firstdir" parent directory and "seconddir" subdirectory */

    MyFILE *testFile = myfopen("/firstdir/seconddir/testfile1.txt", "w"); /* open a virtual file in
    write mode in "seconddir" */

    /*
     * fill the file with text pointer data
     */
    for (int i = 0; i < strlen(text)*10; i++)
        myfputc(text[i%strlen(text)], testFile);
    myfclose(testFile);

    /*
     * get the contents of "seconddir" and print them out
     */
    dirContent = mylistdir("/firstdir/seconddir");
    printContents(dirContent);

    mychdir("/firstdir/seconddir"); /* change the directory to "seconddir" */

     /*
      * get the contents of "seconddir" and print them out
      */
    dirContent = mylistdir("/firstdir/seconddir/");
    printContents(dirContent);

    MyFILE *testFile2 = myfopen("testfile2.txt", "w");  /* open a virtual file in write mode in
    "seconddir" directory */

    /*
     * fill the file with the string assigned to text pointer
     */
	for (int i = 0; i < strlen(text)*2; i++)
		myfputc(text[i%strlen(text)], testFile2);
	myfclose(testFile2);


    mymkdir("thirddir"); /* create new directory called "thirddir" */
	MyFILE *testFile3 = myfopen("thirddir/testfile3.txt", "w"); /* open a virtual file in write mode
	in "thirddir" */

    /*
     * fill the file with text pointer data
     */
	for (int i = 0; i < strlen(text)*2; i++)
        myfputc(text[i%strlen(text)], testFile3);
	myfclose(testFile3);

    writedisk("virtualdiskA5_A1_a"); /* write the virtual disk to the physical disk */

    /*
     * remove the text files
     */
	myremove("testfile1.txt");
	myremove("testfile2.txt");

	writedisk("virtualdiskA5_A1_b"); /* write the virtual disk to the physical disk */

	mychdir("thirddir"); /* change the directory to "thirddir" */
	myremove("testfile3.txt"); /* remove the text file named "testfile3.txt */

	writedisk("virtualdiskA5_A1_c"); /* write the virtual disk to the physical disk */

	mychdir("/firstdir/seconddir"); /* change the directory to "seconddir" */
	myrmdir("thirddir"); /* remove the "thirddir" directory */

	mychdir("/firstdir"); /* change the directory to "firstdir" */
	myrmdir("seconddir"); /* remove the "seconddir" directory */

    mychdir("/"); /* change the directory to root */
	myrmdir("firstdir"); /* remove the "firstdir" directory */

    writedisk("virtualdiskA5_A1_d"); /* write the virtual disk to the physical disk */

}

void extra () {

    const char *text = "EXTRA!!!"; /* string to populate the opened files */
    format();
    MyFILE *textFile = myfopen("textfile.txt", "w");  /* open a virtual file in write mode in
    "seconddir" directory */

    /*
     * fill the file with the string assigned to text pointer
     */
	for (int i = 0; i < strlen(text)*2; i++)
		myfputc(text[i%strlen(text)], textFile);
	myfclose(textFile);

    myfcopy("textfile.txt", "copyfile.txt"); /* copy content of "textfile.txt" to "copyfile.txt" */
    myfmove("copyfile.txt", "movefile.txt"); /* copy content of "copyfile.txt" to "movefile.txt" and
    delete the former */

    myfvdsys("text.txt", "textfile.txt"); /* copy text from virtual disk file to the physical disk
    file */
    myfsysvd("text.txt", "system.txt"); /* copy text from physical disk file to a new virtual disk
    file */

    writedisk("virtualdiskA5_A1_extra"); /* write the virtual disk to the physical disk */
}

int main() {
    d3_d1();
    c3_c1();
    b3_b1();
    a5_a1();
    extra();
    return 0;
}


