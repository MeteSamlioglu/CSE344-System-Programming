#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#define SIZE 256


int main(int argc, char *argv[]) {

    /* Error condition */
    if (argc < 3) 
    {
        fprintf(stderr, "Error: The server name and the connect/type must be entered! %s\n", argv[0]);
        return 0;
    }
    if(!(strcmp(argv[1],"connect") == 0 || strcmp(argv[1],"tryConnect")==0))
    {
        fprintf(stderr, "Connection type can not be %s, it must be connect or tryConnect", argv[0]);
        return 0;
    }

    /* Variables */
    char *directory_name = argv[2];
    char path_client_fifo[SIZE];
    char command[SIZE];
    char response_message[SIZE];
    ssize_t response_sw = 0;


    char fifo_path[SIZE];
    snprintf(fifo_path, sizeof(fifo_path), "/tmp/fifo_%s", directory_name); /* Find the path */

  
    int fifo_server = open(fifo_path, O_WRONLY);
    if (fifo_server == -1) {
        perror("Server FIFO open failed");
        return 1;
    }

    snprintf(path_client_fifo, sizeof(path_client_fifo), "/tmp/client_fifo%d", getpid());
    if (mkfifo(path_client_fifo, 0666) == -1) 
    {
        perror("Fifo path is not found!");
        return 0;
    }
    
    int client_fifo_fd = open(path_client_fifo, O_RDWR);
    if (client_fifo_fd == -1) {
        perror("Fifo client is not found!");
        return 0;
    }

    //connection request for the server
    write(fifo_server, path_client_fifo, strlen(path_client_fifo));

   
    response_sw = read(client_fifo_fd, response_message, sizeof(response_message) - 1);
    if (response_sw == -1 && errno != EAGAIN) 
    {
            perror("The respond of the server has not been received!");
            return 1;
    }
    
    
    
    response_message[response_sw] = '\0';
    if (response_sw <= 0)    
        printf("Server response has not been received\n");
    else
    { 
        printf("The response of %s server has been received: %s\n",argv[1], response_message);
    }

        printf("Enter a command: ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';
        write(client_fifo_fd, command, strlen(command));
        usleep(100000);
        ssize_t server_response = read(client_fifo_fd, response_message, sizeof(response_message));
        if (server_response == -1 && errno != EAGAIN) 
        {
            perror("Server response read failed");
        }
        response_message[server_response] = '\0';
        printf("Server response: %s\n", response_message); /* Server Response */
        memset(command, 0, sizeof(command));


    close(client_fifo_fd);
    unlink(path_client_fifo);
    close(fifo_server);

    return 0;
}