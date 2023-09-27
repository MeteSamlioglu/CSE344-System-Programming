#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include<semaphore.h>
#include <termios.h>
#include<signal.h>


#define MAX_FILE_NUMBER 100

struct FileEntry {
    char FileName[256];
    char lastModificationTime[256];
    int size;
};



struct FileEntry *clientFiles;


int sendFilesToServer(int sockfd, char files[][256], int number_of_files);
int sendFileToServerHelper(int sockfd, char files[][256],  int num_of_files); 
int updateClientFiles( ); 

void receiveServerFiles(int sockfd);
void addToClient(int newsockfd, char files[][256], int num_of_files_added);
int isClientEdited( );

int file_size = 0;
sem_t semFull;

void error(const char *msg)
{
	perror(msg);
	exit(1);
}
void signal_handler(int signal) 
{
    switch(signal)
    {
        case SIGINT:
            sem_destroy(&semFull);
			free(clientFiles);
			printf("\nSIGINT signal is catched\n");            
			exit(1);
			break;
        
		default:
            break;
    }
}


int main(int argc, char* argv[])
{
	
	if(argc < 3)
	{
		fprintf(stderr, "%s Error: At least three parameters needed. [IP] or [127.0.0.1] and [PortNo] \n", argv[0]);
		exit(0);
	}	
	
	int sockfd, portno;
	struct sockaddr_in serv_addr; 
	struct hostent *server; /* It is used to store information about host */
							/*Such as a host name or protocol... */
	char buffer[256];
	clientFiles = calloc(MAX_FILE_NUMBER, sizeof(struct FileEntry));
	sem_init(&semFull, 0, 0);
	signal(SIGINT, signal_handler);


	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd < 0)
		error("ERROR openning socket");
	
	server = gethostbyname(argv[1]);
	
	if(server == NULL)
	{
		fprintf(stderr, "ERROR, no such host\n");
		exit(1);
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, 
		(char *)&serv_addr.sin_addr.s_addr,
		server->h_length);
		serv_addr.sin_port = htons(portno);
	if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR connecting");
	
	bzero(buffer, 256);

	/* Up to now it is skeleton for server-client connection */
	printf("Client\n");
	
	updateClientFiles( );
	//usleep(500000);
	receiveServerFiles(sockfd);	

	char files[file_size][256];
	char ServerCommand[256];
	char CopyCommand[256];
	ssize_t receive_command;
	ssize_t send_command;
	
	strcpy(buffer, "done");
	if(send(sockfd, buffer, 256, 0) > 0)
	{
		for(int counter = 0; counter < file_size; counter++)
			strcpy(files[counter], clientFiles[counter].FileName);

		sendFilesToServer(sockfd, files, file_size);
	}
	bzero(buffer, 256);
	
	if(recv(sockfd, buffer, 256, 0) > 0 )
	{
		printf("Client is active...\n");
	}	
	
	while(1)
	{
		printf("Client\n");
		receive_command = recv(sockfd, ServerCommand, 256, 0);
		if(receive_command < 0)
		{
			perror("Failed to receive command");
			break;
		}
		bzero(ServerCommand, 256);
		int isEdited = isClientEdited( );
		if(isEdited != 0)
		{
			
			if(isEdited == 1)
				strcpy(CopyCommand, "copyToServer2");
			else if(isEdited == -1)
				strcpy(CopyCommand, "copyFromServer2");

			send_command = send(sockfd, CopyCommand, 256, 0);
			if(send_command < 0 )
			{
				perror("Failed to send");
			}
			printf("CopyCommand %s",CopyCommand);
			if(strcmp(CopyCommand, "copyToServer2") == 0)
			{
				updateClientFiles( );
				char files_to_copy[file_size][256];
				int counter2 = 0;
				for(counter2 = 0; counter2 < file_size; counter2++)
					strcpy(files_to_copy[counter2], clientFiles[counter2].FileName);
				
				sendFilesToServer(sockfd, files_to_copy, file_size);
			}
			else if(strcmp(CopyCommand, "copyFromServer2") == 0)
			{
				usleep(500000);
				receiveServerFiles(sockfd);
			}
			strcpy(ServerCommand,"Something Happend");
			send_command = send(sockfd, ServerCommand, 256, 0);
			if(send_command  < 0)
			{
				perror("Failed to send");
			}
			
			receive_command = recv(sockfd, ServerCommand, 256, 0);
			if(receive_command < 0)
			{
				perror("Failed to receive");
			}
			else 
				printf("Received Command %s\n",ServerCommand);
			
			bzero(ServerCommand,256);
			bzero(CopyCommand, 256);	

		}
		else 
		{
			strcpy(ServerCommand, "Nothing Happened");
			send_command = send(sockfd, ServerCommand, 256, 0);
			if(send_command < 0)
			{
				perror("Failed to send command");
				break;
			}
		}
		sleep(3);
	}

	close(sockfd);
	sem_destroy(&semFull);
	free(clientFiles);
}

/** 
 * Client' sends all the files in the directory to the server
 * @param sockfd Sockets that communicates with the server 
 **/
int sendFilesToServer(int sockfd, char files[][256], int number_of_files)
{
	int i = 0;
	int isSuccesfull = 0;
	ssize_t send_bytes = 0;
	send_bytes = send(sockfd, &number_of_files, sizeof(int), 0); /* Send the number of files that are going to delivered to server*/
	
	if(send_bytes < 0)
		perror("Failed to send signal");
	
	else
	{
		while(i < number_of_files)
		{
			
			send(sockfd, files[i], 256, 0);
			printf("File -> %s is sent to Server\n",files[i]);
			usleep(250000);
			i++;
		}
		isSuccesfull = sendFileToServerHelper(sockfd, files,  number_of_files);
		printf("All files have been succesfully sent to Server.\n");
	}
	return isSuccesfull;
}
/**
 * Helper function for sendAllFiles it basically sends the given size of file to server 
 * 
 * @param sockfd Socket for communicating with server
 * @param num_of_files Number of files that are going to be send
 * @return Returns 1 if the operation is succesfull, false otherwise
 */

int sendFileToServerHelper(int sockfd, char files[][256],  int num_of_files)
{

	int isSent = 1, counter = 0;
	int fd_client = 0;
	
	ssize_t read_bytes;
    ssize_t write_bytes;
	ssize_t num_bytes_read = 0;
	
	char buffer[1024];

	while(counter < num_of_files)
	{
		num_bytes_read = 0;
		fd_client = open(files[counter], O_RDONLY | O_CREAT, 0644);
		
		if (fd_client == -1)
		{
			perror("Failed to open file");
			return 0;
		}
		while((read_bytes = read(fd_client, buffer, sizeof(buffer))) > 0)
		{
			write_bytes = send(sockfd, buffer, read_bytes, 0); 
			if(write_bytes == -1)
			{
				perror("Failed to write to file");
				isSent = 0;
				return 0;
			}
			num_bytes_read += read_bytes;
		}

		counter++;
		close(fd_client);    
				
		bzero(&buffer, strlen(buffer));
		usleep(500000);
		if(counter < num_of_files)
		{
			strcpy(buffer,"ENDFILE_SIGNAL_DETECTED_0x24367131");
			send(sockfd, buffer,strlen("ENDFILE_SIGNAL_DETECTED_0x24367131") + 1 , 0);
		}
		else if(counter == num_of_files)
		{
			strcpy(buffer,"TERMINATE_SIGNAL_DETECTED_0x24367131");
			send(sockfd, buffer,strlen("TERMINATE_SIGNAL_DETECTED_0x24367131") + 1 , 0);
		}
	}
	
	printf("Num of bytes written %ld \n",num_bytes_read);
	return isSent;
}


/**
* Adds all the files that are received from server to client directory
* @param sockfd Socket for communicating with Server
*/

void receiveServerFiles(int sockfd)
{
	int number_of_server_files;
	int i = 0; 
	char file_name [256];
	recv(sockfd, &number_of_server_files, sizeof(int), 0); /*Get the number of files from client*/
		
	char files[number_of_server_files][256];
	while (i < number_of_server_files) 
	{
		bzero(file_name, 256);
		recv(sockfd, file_name, 256, 0);
		strcpy(files[i], file_name); 
		i++;
	}		
	addToClient(sockfd, files, number_of_server_files);
	updateClientFiles( );
	printf("Server's data is copied successfully.\n");

}

/**
 * When a new file is added to the server, this method adds the new files to the client's directory
 * 
 * @param newsockfd Socket that communicates with server
 * @param num_of_files_added The number of added files
 *
 * */
void addToClient(int sockfd, char files[][256], int num_of_files_added) 
{
		int i = 0, counter = 0;
		
		ssize_t read_bytes;    	
		ssize_t write_bytes;
    	ssize_t num_bytes_read = 0;
		
		int fd_destination = 0;
		char buffer[1024];

    	
		fd_destination = open(files[counter], O_WRONLY | O_CREAT | O_TRUNC, 0644);

		while ((read_bytes = recv(sockfd, buffer, sizeof(buffer), 0)) > 0)
		{
			
			if(strcmp(buffer, "TERMINATE_SIGNAL_DETECTED_0x24367139") == 0)
			{
				break;
			}
			if(strcmp(buffer, "ENDFILE_SIGNAL_DETECTED_0x24367139") == 0)
			{
				counter++;
				//printf("ENDFILE_SIGNAL_DETECTED_0x24367139\n"); 
				close(fd_destination);
				printf("File -> %s is received from Server\n",files[counter]);
				fd_destination = open(files[counter], O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if(fd_destination == -1)
				{
					perror("Failed to open!\n");
				}
			}
			else 
			{
				write_bytes = write(fd_destination, buffer, read_bytes);	
				if(write_bytes == -1)
				{
					close(fd_destination);
					perror("Failed to write\n");
					return;
				}	
				num_bytes_read+=read_bytes;
				i++;
			}
		}
		printf("All files have been succesfully received from Server.\n");
		close(fd_destination);
} 


/**
 * Whenever a client is created, it first checks the directory and add the files to the system
 * @return 1 if it is succesfull, false otherwise
 */

int updateClientFiles( )
{
	DIR *dir;
    file_size = 0;
    struct dirent *entry;

    dir = opendir(".");
    if (dir == NULL) {
        perror("opendir() error");
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) 
    {
        if( strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "client") == 0 || strcmp(entry->d_name, "client.c") == 0 || strcmp(entry->d_name, "makefile") == 0)
        	continue;
        else 
       	{
			strncpy(clientFiles[file_size].FileName, entry->d_name, sizeof(clientFiles->FileName));
	    	file_size++;
    	}
    	
    }

    closedir(dir);
    return 1;
}
/**
 * @return Returns 1 if a new file is added to the client, 0 if no change has been made, -1 the number of files decreased 
 */

int isClientEdited( )
{
	DIR *dir;
    struct dirent *entry;
	int number_of_files_directory = 0;
    int value_return;
	dir = opendir(".");
    if (dir == NULL) 
	{
        perror("opendir() error");
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) 
    {
        if( strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "client") == 0 || strcmp(entry->d_name, "client.c") == 0 || strcmp(entry->d_name, "makefile") == 0)
        	continue;
        else 
       	{
			strncpy(clientFiles[file_size].FileName, entry->d_name, sizeof(clientFiles->FileName));
	    	number_of_files_directory++;
    	}
    }
	
	if(number_of_files_directory == file_size) /* Do nothing */
		value_return = 0;
	
	if(number_of_files_directory > file_size) /* Add a new file*/
		value_return = 1;
	
	if(number_of_files_directory < file_size) /*Copy client to server*/
		value_return = -1;
	
	file_size = number_of_files_directory;

	return value_return;
}
