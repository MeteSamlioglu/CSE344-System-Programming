#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>
#include<semaphore.h>
#include<pthread.h>
#include<signal.h>

#define THREAD_POOL 100
#define MAX_SERVER_FILES 100

struct ServerDataBase {
    char FileName[256];
    char lastModificationTime[256];
    int size;
};



struct ServerDataBase  *ServerFiles;
pthread_cond_t serverSignal;
int server_files_count = 0;
int num_of_threads = 0;
int sockfd, newsockfd;
sem_t semEmpty;

void receiveClientFiles(int newsockfd);

void addToServer(int newsockfd,char files[][256], int num_of_files_added);


int sendFileToClient(int newsockfd, char files[][256], int number_of_files); 

int sendFileToClientHelper(int newsockfd, char files[][256], int num_of_files);

int updateServerFiles( );

void signal_handler(int signal) 
{
    switch(signal)
    {
        case SIGINT:
            printf("\nSIGINT signal is catched\n");            
            
			free(ServerFiles);
			pthread_cond_destroy(&serverSignal);
			close(sockfd);
			close(newsockfd);
			pthread_exit(NULL);
			break;
        
		default:
            break;
    }
}


void* client(void* args)
{
	ssize_t send_command = 0;
	ssize_t receive_command = 0;
	char command[256];
	char copyCommand[256];
	char message[256];
	int index = *(int*) args;
	int counter = 0;
	char send_files[server_files_count][256];
	
	updateServerFiles( ); /*Get Server Files*/
	
	for(counter = 0; counter < server_files_count; counter++)
	{		
		strcpy(send_files[counter], ServerFiles[counter].FileName);
	}
	sendFileToClient(newsockfd, send_files, server_files_count);
	
	char signalDone[256];
 	if(recv(newsockfd, signalDone, 256, 0) > 0)
	{
		if(strcmp(signalDone, "done") == 0)
		{
			receiveClientFiles(newsockfd);
			send(newsockfd, signalDone, 256, 0);
		}
		else 
			return 0;
	}
	else 
		perror("Failed to receive");
	 
	printf("Client %d...\n", index);
	
	while(1)
	{
		printf("Server!\n");
		send_command = send(newsockfd, command, 256, 0);
		
		if(send_command < 0)
		{
			perror("Failed to send command\n");
			break;
		}
		
		bzero(command, 256);
		receive_command = recv(newsockfd, command, 256, 0);
		if(receive_command < 0)
		{
			perror("Failed to receive command\n");
			break;
		}
		
		if(strcmp(command, "Nothing Happened") == 0)
		{
			printf("Nothing happened!\n");
		}
		else 
		{
			printf("Command %s\n",command);
			receive_command = recv(newsockfd, copyCommand , 256, 0);
			if(receive_command < 0)
			{
				perror("Failed to receive command");
			}
			
			printf("CopyCommand %s\n",copyCommand);
			if(strcmp(command,"copyToServer2") == 0)
			{
				receiveClientFiles(newsockfd);
				strcpy(message, "copyToServer2");
			}
			else if(strcmp(command, "copyFromServer2"))
			{
				updateServerFiles( ); /*Get Server Files*/
				int counter2 = 0;
				char send_files2[server_files_count][256];
				for(counter2 = 0; counter2 < server_files_count; counter2++)
				{		
				strcpy(send_files2[counter2], ServerFiles[counter2].FileName);
				}
				sendFileToClient(newsockfd, send_files2, server_files_count);
				strcpy(message, "copyFromServer2");
			}
			else 
			{
				strcpy(message, "done0");
			}
			
			send_command = send(newsockfd, message, 256, 0);
			if(send_command < 0)
			{
				perror("Failed to send command");
			}

			bzero(copyCommand, 256);
			bzero(message, 256);
		}
		
		sleep(3);
	}

	free(args);
	return args;

}


void error(const char *msg)
{
	perror(msg);
	exit(1);
}





int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		fprintf(stderr, "Port No not provided. Program terminated");
		exit(1);
	}
	
	ServerFiles = calloc(MAX_SERVER_FILES, sizeof(struct ServerDataBase));
	pthread_cond_init(&serverSignal, NULL);

	sem_init(&semEmpty, 0, 0);
	
	signal(SIGINT, signal_handler);

	int  portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd < 0)
	{
		error("ERROR opennig socket");
	}
	
	bzero((char*) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0 )
	{
		error("Binding Failed.");
	}
	listen(sockfd, 50); //50 is the max number of clients that can connect to a server at a time
	clilen = sizeof(cli_addr);
	
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

	if(newsockfd < 0)
		error("Error on Accept");
	else if(newsockfd>=0)
		printf("The client is connected...\n");


	/* The client is connected after this line */	
	
	pthread_t client_thread;
	int *client_index = malloc(sizeof(int));  /* Assign an number to client */
	*client_index = num_of_threads + 1;
	if (pthread_create(&client_thread, NULL, &client, client_index) != 0)	
		perror("Failed to create consumer thread");
	else 
		num_of_threads++;


	if(pthread_join(client_thread, NULL) == 0)
		num_of_threads--;
	else 
	{
		perror("Failed to create thread");
		return 0;
	}
	

	close(newsockfd);
	close(sockfd);
	free(ServerFiles);
	sem_destroy(&semEmpty);
	pthread_cond_destroy(&serverSignal);

	return 0;
}
	/**
	 * Adds all the files that are received from client to server directory
	 * @param newsockfd Socket for communicating with client
	 */
	void receiveClientFiles(int newsockfd)
	{
		int number_of_client_files;
		int i = 0; 
		char file_name [256];
		recv(newsockfd, &number_of_client_files, sizeof(int), 0); /*Get the number of files from client*/
		
		char files[number_of_client_files][256];
		
		while (i < number_of_client_files) 
		{	
			recv(newsockfd, file_name, 256, 0);
			strcpy(files[i], file_name); 
			i++;
		}
		
		addToServer(newsockfd, files, number_of_client_files);
		updateServerFiles( );
	}
	
	/** 
	 * Helper function for AddClientFiles, it basically adds the files that are passed from addClientFiles to Server
	 * @param newsockfd 
	 * @param num_of_files_added 
	 */

	void addToServer(int newsockfd, char files[][256], int num_of_files_added) /* Num_of_files not used*/
	{
		int i = 0, counter = 0;
		ssize_t read_bytes;    	
		ssize_t write_bytes;
    	ssize_t num_bytes_read = 0;
		int fd_destination = 0;
		char buffer[1024];

    	
		fd_destination = open(files[counter], O_WRONLY | O_CREAT | O_TRUNC, 0644);

		while ((read_bytes = recv(newsockfd, buffer, sizeof(buffer), 0)) > 0)
		{
			if(strcmp(buffer, "TERMINATE_SIGNAL_DETECTED_0x24367131") == 0)
			{
				break;
			}
			else if(strcmp(buffer, "ENDFILE_SIGNAL_DETECTED_0x24367131") == 0)
			{
				counter++;
				/* printf("ENDFILE_SIGNAL_DETECTED_0x24367131\n"); */
				close(fd_destination);
				printf("File -> %s is received from Client\n",files[counter]);
				fd_destination = open(files[counter], O_WRONLY | O_CREAT | O_TRUNC, 0644);
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
		
		printf("All files have been succesfully received from Client.\n");
		close(fd_destination);
	}
	
	
	int sendFileToClient(int newsockfd, char files[][256], int number_of_files) 
	{
		int i = 0;
		int isSuccesfull = 0;
		ssize_t byte_control = 0;
		byte_control = send(newsockfd, &number_of_files, sizeof(int), 0); /* Send the number of files that are going to delivered to server*/
		
		if(byte_control < 0)
		{

			perror("Failed to send signal");
			return 0;
		}
		else 
		{
			while(i < number_of_files)
			{
				send(newsockfd, files[i], 256, 0);
				printf("File -> %s is sent to the Client\n",files[i]);
				i++;
				usleep(250000);
			}
			isSuccesfull = sendFileToClientHelper(newsockfd, files, number_of_files);
			printf("All  files have been succesfully sent to Client.\n");

			return isSuccesfull;
		}
	}
	
	/**
	 * Sends the the some files from server to client 
	 * @param sockfd Server socket
	 * @param DeliverFiles The files that are going to be delivered to client
	 * @param num_of_files The size of the files that are going to be delivered
	 */
	int sendFileToClientHelper(int newsockfd, char files[][256], int num_of_files)
	{
		int counter = 0;
		int fd_server = 0;
		
		ssize_t read_bytes;
		ssize_t write_bytes;
		ssize_t num_bytes_read = 0;
		char buffer[1024];
		while(counter < num_of_files)
		{
			num_bytes_read = 0;
			fd_server = open(files[counter], O_RDONLY | O_CREAT, 0644);
			
			if (fd_server == -1)
			{
				perror("Failed to open Server file");
				return 0;
			}
			while((read_bytes = read(fd_server, buffer, sizeof(buffer))) > 0)
			{
				write_bytes = send(newsockfd, buffer, read_bytes, 0); 
				if(write_bytes == -1)
				{
					perror("Failed to write to file");
					return 0;
				}
				num_bytes_read += read_bytes;
			}
			counter++;
			close(fd_server);    
			bzero(&buffer, strlen(buffer));
			usleep(500000);
			if(counter < num_of_files)
			{
				strcpy(buffer,"ENDFILE_SIGNAL_DETECTED_0x24367139");
				send(newsockfd, buffer,strlen("ENDFILE_SIGNAL_DETECTED_0x24367139") + 1 , 0);
			}
			else if(counter == num_of_files)
			{
				strcpy(buffer,"TERMINATE_SIGNAL_DETECTED_0x24367139");
				send(newsockfd, buffer,strlen("TERMINATE_SIGNAL_DETECTED_0x24367139") + 1 , 0);
			}
		}
		
		return 1;
	}

	int updateServerFiles( )
	{
		DIR *dir;
		struct dirent *entry;
		server_files_count = 0;
		dir = opendir(".");
		if (dir == NULL) {
			perror("opendir() error");
			return 0;
		}

		while ((entry = readdir(dir)) != NULL) 
		{
			if( strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "server") == 0 || strcmp(entry->d_name, "server.c") == 0 || strcmp(entry->d_name, "makefile") == 0)
				continue;
			else 
			{
				strncpy(ServerFiles[server_files_count].FileName, entry->d_name, sizeof(ServerFiles->FileName));
				server_files_count++;
			}
			
		}

		closedir(dir);
		return 1;
	}