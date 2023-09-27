#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include <errno.h>

int dup(int oldfd);
int dup2(int oldfd, int newfd);

int main( )
{
	mode_t modes = S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH;
	char *filename1 ="test1.txt";
	char *filename2 ="test2.txt";
	/* TEST1 DUP() Function */
	/* --------------------- */
	int fd1 = open(filename1, O_CREAT | O_WRONLY | O_APPEND, modes);
	if(fd1 == -1)
	{
		perror("open");
		exit(EXIT_SUCCESS);
	}

	if( write(fd1,"fd1 file descriptor\n",20) == -1)
	{	perror("write");
		exit(EXIT_SUCCESS);
		
	}
	int fd2 = dup(fd1); /* Create the duplicate of fd1 and assign it to fd2 */
	
	if(write(fd2,"fd2 file descriptor\n",20) == -1)
	{
		perror("write");
		exit(EXIT_SUCCESS);
	}
	printf("test1 dup() fd1=%d , fd2=%d\n",fd1, fd2); /* The file descriptor number of fd1 and fd2 will be different but 
												they will write to the same file which is filename1*/

	/* --------------- End of test1 ---------------*/
	/*--------------- TEST2 DUP2(oldfd, newfd)-----------------*/
	int fd3 = open(filename2, O_CREAT| O_WRONLY| O_APPEND, modes);
	int fd5 = 6; // Create a new file descriptor, with value 6.
	if(fd3 == -1)
	{
		perror("fd3 could not open.");
		exit(EXIT_SUCCESS);
	}
	
	int fd4 = dup2(fd3, fd5); //fd4 will be assigned to 6 which is file descriptor number of fd5, so both f4 and fd5 will be duplicates of fd3
	
	if(fd4 == -1)
	{
		perror("fd4");
		exit(EXIT_SUCCESS);
	}
	
	write(fd3, "This is test2 ",14);
	write(fd4,"Hello ",6); /* fd4 writes Hello to filename2.txt */
	write(fd5,"World!",6); /* fd5 writest World! to filename2.txt */
	
	printf("test2 dup2() fd3=%d fd4=%d fd5=%d\n",fd3,fd4,fd5);
	
	/*-------------------- Not valid file descriptor testcase ----------------*/
	/*
	int fd6 = open("not_exist.txt",O_RDONLY);
	int fd7 = dup2(fd6,fd6);
	
	if(fd7 == -1)
	{
		perror("fd7");
	}
	close(fd6)
	close(fd7)

	*/
	/* ------------------------------------------------------------------- */

	close(fd1);
	close(fd2);
	close(fd3);
	
	/*------------------- Test case close -------------------------------- */ 
	/* I closed fd3 but did not close fd4 which is duplicate of fd3. Lets try to write to a file */
	/*
	if(write(fd4," fd4",4) == -1)
	{
		perror("write");
	}
	*/
	close(fd4);
	close(fd5);

	/* All tests are done */
	return 0;
}

int dup(int oldfd)
{
	int newfd = fcntl(oldfd, F_DUPFD,0); //Create the duplicate of oldfd by using F_DUPFD
	if(newfd == -1) // An error occured
	{
		perror("newfd");
		return -1; 
	}
	else
		return newfd;
}

int dup2(int oldfd, int newfd)
{

	if(oldfd == newfd) //Special case
	{
		if(fcntl(oldfd,F_GETFL) == -1)
		{ 								//Check whether oldfd is valid, if it is not valid set errno to EBADF and return -1
			errno = EBADF;
			return -1;
		}
		else
			return oldfd;
	}
	else
	{
		/* Close newfd if it is still open. */
		close(newfd);
		int duplicate = fcntl(oldfd, F_DUPFD, newfd);
		if(duplicate == -1)
		{
			perror("duplicate");
			return -1;
		}
		return duplicate; //Return the new file
	}

}

