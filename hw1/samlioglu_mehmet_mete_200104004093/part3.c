#include<fcntl.h>
#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<unistd.h>

int dup(int oldfd);
int dup2(int oldfd, int newfd);


int main( )
{
	mode_t modes = S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH;
	int fd = open("myfile.txt", O_CREAT | O_WRONLY, modes);
	if(fd == -1)
	{
		perror("open");
		exit(EXIT_SUCCESS);
	}
	int fd2 = dup(fd);
	int fd3 = dup2(fd, fd2);
	printf("File descriptor values fd1 fd2 fd3.\n");
	printf("fd:%d fd2:%d fd3:%d\n",fd, fd2, fd3);
	/* ------------------------------ Test1 ---------------------------------------------*/
	off_t offset1 = lseek(fd, 20, SEEK_CUR);
	off_t offset2 = lseek(fd2, 0, SEEK_CUR);
	off_t offset3 = lseek(fd3, 0, SEEK_CUR);

	printf("The duplicated file descriptor will have the same offset value.\n");
	printf("offset1: %ld  offset2: %ld offset3: %ld\n",offset1, offset2, offset3);
	int fd4 = open("myfile.txt",O_WRONLY,modes);
	if(fd4 == -1)
	{
		perror("open");
	}
	off_t offset4 = lseek(fd4, 0, SEEK_CUR);
	printf("fd4 is not a duplicated file descriptor, so that the offset4 will be different than duplicated ones.\n");
	printf("offset1: %ld  offset2: %ld offset3: %ld offset4: %ld\n",offset1, offset2, offset3, offset4);
	/*--------------------------- Test1 END --------------------------------------------------------*/
	/* -------------------------- Test2 -------------------------------------------*/
	/* Try to write to the file and check whether duplicated file descriptors shares the same offset */
	if(write(fd,"Hello ",6) == -1)
	{
		perror("write");
	}
	
	if(write(fd2,"World",5) == -1)
	{
		perror("write");
	}
	
	if(write(fd4,"Test myFile",11) == -1)
	{
		perror("write");
	}
	
	if(write(fd3, " Test2 is completed",19 )==-1)
	{
		perror("write");
	}
	
	/* ------------------------------ Test 2 END ----------------------------------*/
	/* ------------------------ Test 3 -----------------------------------*/
	
	lseek(fd, 10, SEEK_CUR);
	if(write(fd,"Test 3",6)==-1)
	{
		perror("write");
	}
	if(write(fd2," is",3) == -1)
	{
		perror("write");
	}
	if(write(fd3," ended",6) == -1)
	{
		perror("write");
	}
	
	/* ------------------------ Test 3 END -----------------------------------*/
	/* ------------------------ Test 4 ---------------------------------------*/
	/* Lets see if duplicates have the same file status ----------------------*/
	int filestatus1 = fcntl(fd, F_GETFL);
	int filestatus2 = fcntl(fd2, F_GETFL);
	int filestatus3 = fcntl(fd3, F_GETFL);
	printf("filestatus fd1 = %d filestatus fd2 = %d filestatus fd3 = %d\n",filestatus1, filestatus2, filestatus3);

	/* -------------------- Test 4 END --------------------------------------*/

	/* All test are done, program shows that fd, fd2, fd3 are duplicated file descriptors and they shares the same offset value and openfile */
	close(fd);
	close(fd2);
	close(fd3);
	close(fd4);
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