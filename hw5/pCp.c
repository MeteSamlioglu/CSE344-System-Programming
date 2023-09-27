#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/time.h>
#include<signal.h>
#include<semaphore.h>
#include<errno.h>



struct FilePath
{
    
    char destination_path[256];
    char source_path[256];
    int source_fd;
    int destination_fd;
};

struct FilePath* buffer = NULL;

int count = 0;
int done = 0;

pthread_mutex_t mutexBuffer;
pthread_cond_t producerSignal;

sem_t semEmpty;
sem_t semFull;

int total_copied_files = 0;
int total_bytes_read = 0;
int total_copied_directories = 0;
int total_copied_fifos = 0;
int copied_bytes_num = 0; // Total number of copied bytes


void signal_handler(int signal) 
{
    switch(signal)
    {
        case SIGINT:
            printf("\nSIGINT signal is catched\n");            
            break;

        case SIGTERM:
            printf("\nSIGTERM signal is catched\n");
            break;

        case SIGUSR1:
            printf("\nSIGUSR1 signal is catched\n");
            break;

        case SIGUSR2:
            printf("\nSIGUSR2 signal is catched\n");
            break;
            
        default:
            break;
    }
    printf("The program is releasing all resources...\n");
    sleep(2);
    done = 1;
    pthread_cond_broadcast(&producerSignal); /* Send the signal to each thread that still running and terminate all the threads */
}
    

int is_directory(const char* directory_path)  /* Checks if the given path is for directory */
{
    struct stat st;

    if (stat(directory_path, &st) == 0) 
    {
        return S_ISDIR(st.st_mode); 
    }
    return 0;  
}


void copy(const char* path_source, const char* path_destination)  /* Copies the file from path_source to path_destination */

{
    ssize_t read_bytes;
    ssize_t write_bytes;
    char buffer[1024];


    int fd_source = open(path_source, O_RDONLY, 0644);
    if(fd_source == -1)
    {
        perror("Failed to open source file");
        return;
    }

    int fd_destination = open(path_destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if(fd_destination == -1)
    {
        perror("Failed to open destination file");
    }

    while( (read_bytes = read(fd_source, buffer, sizeof(buffer))) > 0)
        {
        write_bytes = write(fd_destination, buffer, read_bytes); 
        if(write_bytes == -1)
        {
            perror("Failed to writing to file");
            return;
        }
         total_bytes_read += write_bytes;
    
    }
    
    close(fd_destination);
    close(fd_source);
    
}

void* producer(void* args)
{
    
    struct dirent* entry;
    char* source_dir = ((char**)args)[0];
    char* destination_dir = ((char**)args)[1];


    DIR* dir = opendir(source_dir);
    if (!dir) 
    {
        printf("Error opening source directory: %s\n", source_dir);
        done = 1;
        return NULL;
    }
        
    while (done == 0)
    {

        if((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
            }

            char source_path[512];
            char destination_path[512];

            sprintf(source_path, "%s/%s", source_dir, entry->d_name);
            sprintf(destination_path, "%s/%s", destination_dir, entry->d_name);            

            struct stat st;

            /* Try to get source_path status */
            if (stat(source_path, &st) == -1) 
            {
                printf("Source path: %s does not exist\n", source_path);
                continue;
            }
        
            if (S_ISREG(st.st_mode)){ /* Regular file handling */
                // Add to the buffer

                struct FilePath file_entry;
                file_entry.source_fd = -1;
                file_entry.destination_fd = -1;
                strncpy(file_entry.source_path, source_path, sizeof(file_entry.source_path));
                strncpy(file_entry.destination_path, destination_path, sizeof(file_entry.destination_path));
                
                sem_wait(&semEmpty);
                pthread_mutex_lock(&mutexBuffer);

                buffer[count] = file_entry;

                count++;
                
                pthread_mutex_unlock(&mutexBuffer);
                sem_post(&semFull);

            }
            else if(S_ISDIR(st.st_mode)) /* Check if it is Directory*/
            {
                sleep(1);
                char *sub_dir_source = malloc(512);
                char *sub_dir_destination = malloc(512);
                
                sprintf(sub_dir_source, "%s/%s",source_dir, entry->d_name);
                sprintf(sub_dir_destination,"%s/%s",destination_dir, entry->d_name);

                if (!is_directory(sub_dir_destination)) /* Checks if the directory already exist in the destination path */ 
                {
                    if(mkdir(sub_dir_destination, 0777) != 0)
                    {
                        perror("Failed to create the directory");
                        break;
                    }
                }
                
                total_copied_directories++;
                pthread_t sub_thread;
                char* sub_args[] = {sub_dir_source, sub_dir_destination};
                if(pthread_create(&sub_thread, NULL, &producer, sub_args) != 0)
                {
                    perror("Failed to create sub-thread");

                }
                if(pthread_join(sub_thread, NULL) != 0)
                {
                    perror("Failed to join sub-thread");
                }
                
                free(sub_dir_source);
                free(sub_dir_destination);
            }
            else if(S_ISFIFO(st.st_mode)) /* Check if it is FIFO */
            {
                char destination_fifo[520];
                char *fifo_position = strrchr(source_path, '/') + 1; /* We get the name of the fifo */

                sprintf(destination_fifo, "%s/%s", destination_path, fifo_position);
                if(mkfifo(destination_fifo, 0666) != 0)
                {
                    perror("Failed to create destination_fifo");
                    printf(" The errno is %d\n", errno);
                }
                total_copied_fifos++;
            }
        }
        else /* It means that there are not entries in the array */
            break;
    }
    closedir(dir);
    return NULL;
}

void* consumer(void* args)
{
    //int index = *(int*) args;
    
    while (1)
    {
        sem_wait(&semFull);
        if (done == 1) 
        {
            break;
        }
        
        pthread_mutex_lock(&mutexBuffer);
        struct FilePath file_entry;

        file_entry = buffer[count - 1];
        count--;

        pthread_mutex_unlock(&mutexBuffer);

        printf("Succesfully copied:%s -> %s\n", file_entry.source_path, file_entry.destination_path);
        /* Copy files from source to destination path */
        copy(file_entry.source_path, file_entry.destination_path);
        total_copied_files++;
        sem_post(&semEmpty);

        //sleep(1);
    }
    //free(args);
    return NULL;
}



int main(int argc, char* argv[])
{
    
    if(argc < 5)
    {
        printf("At least 4 parameters is required buffer_size <consumer_count source_path destination path");
        return 1;
    }

    pthread_mutex_init(&mutexBuffer, NULL);
    pthread_cond_init(&producerSignal, NULL);

    int i;
    char* source_dir = argv[3];
    char* destination_dir = argv[4];
    
    struct timeval start, end;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);    
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);

    int buffer_size = atoi(argv[1]);
    int consumer_count = atoi(argv[2]);
    int thread_pool = consumer_count + 1; /* One thread for producer and the rest of them is to compute consumer */
    
    buffer = calloc(buffer_size, sizeof(struct FilePath));
    pthread_t th[thread_pool];
    sem_init(&semEmpty,0, consumer_count);
    sem_init(&semFull, 0, 0);

    DIR *destion_directory = opendir(destination_dir);
    if (!destion_directory) /* If destination path does not exist, we need to create one */
    {
        if(mkdir(destination_dir, 0777) == 0)
        {
            printf("Destination path %s is created\n",destination_dir);
        }
    }

    gettimeofday(&start, NULL);

    
    for (i = 0; i < thread_pool; i++)
    {
        if (i == 0)
        {
            if (pthread_create(&th[i], NULL, &producer, (void*[]){source_dir, destination_dir}) != 0)
            {
                perror("Failed to create producer_thread\n");
            }
        }
        else
        {
            //int *a = malloc(sizeof(int));
            //*a = i;
            if (pthread_create(&th[i], NULL, &consumer, NULL) != 0)
            {
                perror("Failed to create consumer_thread\n");
            }          
        }
    }

    for (i = 0; i < thread_pool; i++)
    {
        if(pthread_join(th[i], NULL) == 0)
        {
            if(i == 0)
            {
                printf("Producer Thread has joined\n");
                done = 1;
                for(int i = 0; i < consumer_count; i++)
                    sem_post(&semFull);

                pthread_cond_broadcast(&producerSignal); /* Send the signal to each thread that still running and terminate all the threads */
                printf("Waiting for threads to join...\n");
            }
            else
            {
                printf("Thread %d has joined\n",i);
            }
        }
        else
        {
            perror("Failed to join thread\n");
        }
    }
    gettimeofday(&end, NULL);
    double time_taken = (end.tv_sec - start.tv_sec) +
                          (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("The copying process is succesfully done\n");

    printf("The total time to copy files in the directory: %.2f seconds\n", time_taken);
    printf("The number of bytes read = %d\n",total_bytes_read);
    printf("The number files copied = %d\n", total_copied_files);
    printf("The number directories copied = %d\n", total_copied_directories);
    printf("The number fifos copied = %d\n", total_copied_fifos);

    pthread_mutex_destroy(&mutexBuffer);
    pthread_cond_destroy(&producerSignal);
    sem_destroy(&semEmpty);
    sem_destroy(&semFull);
    free(buffer);
    return 0;
}
