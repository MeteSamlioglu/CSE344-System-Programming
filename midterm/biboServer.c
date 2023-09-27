#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>

#define BUFFER_SIZE 256
#define MAX_CLIENTS 10


sem_t sem_client_count;
int client_count = 0;
int client_fifo_fds[MAX_CLIENTS];
volatile sig_atomic_t condition_break = 1;


void signal_handler(int signal);
void client_commands(int client_index, char* directory_name);


int main(int argc, char *argv[]) 
{
    
    if (argc < 3) 
    {
        fprintf(stderr, "You must enter a directory name and also max_clients %s\n", argv[0]);
        return 0;
    }

    char *directory_name = argv[1];
    int max_clients = atoi(argv[2]);

    // Create the directory if it does not exist
    mkdir(directory_name, 0777);
    char fifo_directory[BUFFER_SIZE]; 

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    sem_init(&sem_client_count, 0, 1);
    snprintf(fifo_directory, sizeof(fifo_directory), "/tmp/fifo_%d", getpid());

  
    if (mkfifo(fifo_directory, 0666) == -1) { /* FIFO is created */
        perror("FIFO is not created!");
        return 1;
    }

    printf("Server Started PID %d\n", getpid());
    printf("Waiting for clients...\n");
    int fd_fifo = open(fifo_directory, O_RDWR);
    if (fd_fifo == -1) {
        perror("FIFO open failed");
        return 1;
    }
   
    while (condition_break) {
        
        char client_fifo[BUFFER_SIZE];
        ssize_t bytes_received = read(fd_fifo, client_fifo, sizeof(client_fifo) - 1);
        if (bytes_received <= 0) {
            perror("FIFO read failed");
            break;
        }
        client_fifo[bytes_received] = '\0';
        
        // Open the client FIFO for writing
        int client_fifo_fd = open(client_fifo, O_RDWR);
        if (client_fifo_fd == -1) {
            perror("Client FIFO open failed");
            return 1;
        }

        sem_wait(&sem_client_count);
        if (client_count >= max_clients) {
            printf("Connection request PID %d... Que FULL\n", getpid());
            close(client_fifo_fd);
            sem_post(&sem_client_count);
            continue;
        }
        client_count++;
        sem_post(&sem_client_count);

        int client_index = client_count - 1;
        client_fifo_fds[client_index] = client_fifo_fd;

        printf("Client PID %d connected as \"client%02d\"\n", getpid(), client_index + 1);
        pid_t pid = fork();
        if (pid == -1) {
            perror("Fork");
            break;
        } 
        else if (pid == 0) 
        {
            // Child process
            close(fd_fifo);
            client_commands(client_index, directory_name);
            exit(0);
        } else {
            // Parent process
            close(client_fifo_fd); // Close the client FIFO in the parent process
        }
    }

    printf("kill signal received.. terminating...\n");
    printf("bye\n");

    close(fd_fifo);
    unlink(fifo_directory);
    while (wait(NULL) > 0);

    sem_destroy(&sem_client_count); /* Destroy the semaphore */
    return 0;
}


void client_commands(int client_index, char *directory_name) {
    int client_fifo_fd = client_fifo_fds[client_index];

    // Read from client FIFO
    char command[BUFFER_SIZE];
    char *server_response = "Connected to the server";
    write(client_fifo_fd, server_response, strlen(server_response));
    usleep(100000);
    while (1) 
    {
        ssize_t bytes_received = read(client_fifo_fd, command, sizeof(command));
        command[strcspn(command, "\n")] = '\0';
        if (bytes_received <= 0) {
            if (bytes_received == -1) {
                perror("Failed FIFO read operation");
            }
            break;
        }
        else
        {
            
            if(strcmp(command, "help") == 0 || strcmp(command, "help help") == 0)
            {
                char *str = "Available comments are :\nhelp, list, readF, writeT, upload, download, quit, killServer";
                write(client_fifo_fd, str, strlen(str));
            }
            if(strcmp(command,"quit") == 0)
            {
                printf("client%02d disconnected\n",client_index + 1);
                char filename[128];
                char path[256];
                snprintf(filename, sizeof(filename), "logfile_%d", (client_index + 1));
                snprintf(path, sizeof(path), "./%s/%s", directory_name, filename);
                FILE* file = fopen(path, "w");
                if (file == NULL) 
                {
                    perror("File is not created");
                }
                fclose(file);
                break;
            }
            if(strcmp(command,"list") == 0)
            {
                char list_command[256];
                snprintf(list_command, sizeof(list_command), "ls %s", directory_name);
                int command_check = system(command);
                if(command_check == -1)
                {
                    perror("Command");
                }
            }
            if( strcmp(command, "killServer") == 0)
            {
                kill(getppid(), SIGTERM);
                break;
            }            

        }
        
        
        memset(command, 0, sizeof(command)); /* Clear command*/
        usleep(100000);
    }

    // Close client FIFO
    close(client_fifo_fd);

    // Update client count
    sem_wait(&sem_client_count);
    client_count--;
    sem_post(&sem_client_count);
}

void signal_handler(int signal) 
{
    switch(signal)
    {
        case SIGINT:
            printf("SIGINT Signal");
            condition_break = 0;
            
            break;
        case SIGTERM:
            printf("SIGTERM Signal");
            condition_break = 0;
            break;
    
        default:
            condition_break = 1;
            break;
    }
}