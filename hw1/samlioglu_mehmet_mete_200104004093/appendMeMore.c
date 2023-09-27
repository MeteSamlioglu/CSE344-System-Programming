#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include <unistd.h>


int main(int argc, char* argv[])
{
	int fd;
	int i;
	char *filename = argv[1];
	mode_t modes = S_IWUSR | S_IRUSR | S_IWGRP | S_IROTH | S_IRGRP;
	
	if(argc == 3 || argc == 4)
	{
		if(argc == 4 && strcmp(argv[3],"x") == 0)
			fd = open(filename, O_WRONLY | O_CREAT, modes);
		else if(argc == 3)
			fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, modes); 
		else{
			perror("This command does not exist!");
			exit(EXIT_SUCCESS);
		}
		if(fd == -1)
		{
			perror("fd\n");
			exit(EXIT_SUCCESS);
		}
		int bytes_num = atoi(argv[2]);
		if(bytes_num == 0)
		{
			perror("The command must include the number of bytes!\n");
			exit(EXIT_SUCCESS);
		}

		if(argc == 4){
			for(i = 0 ; i < bytes_num; i++)
			{
				if(lseek(fd, 0, SEEK_END) == -1)
				{
					perror("lseek\n");
					exit(EXIT_SUCCESS);
				}
				if(write(fd,"",1) == -1)
				{
					perror("write\n");
					exit(EXIT_SUCCESS);
				}
			}
		}
		else if(argc == 3)
		{
			for(i = 0 ; i < bytes_num; i++)
			{
				if(write(fd,"",1) == -1)
				{
					perror("write\n");
					exit(EXIT_SUCCESS);
				}
			}
		}
	}
	else
	{
		perror("At least 3 arguments is needed but can not be more than 4.\n");
		exit(EXIT_SUCCESS);
	}

	close(fd);
	return 0;
}
