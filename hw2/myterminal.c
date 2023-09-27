#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include<string.h>
#include <signal.h>
#include <errno.h>
#include<signal.h>

#define MAX_SHELL_LENGTH 200

const int findPipeCount(char*); /*Returns the number of pipes*/
void executeCommands(const char* ,const char*);
int splitCommand(char*, char*, char*,char*, char*, char*); /* Returns the splited, command1 and command2 */
int executSingleCommand(char*,char*, char*, char*, char*);
int executePipedCommands(char* ,int );

void handler(int signum) 
{
	switch(signum)
	{
		case SIGTERM:
			printf("SIGTERM: Terminal is going to be terminated...\n");
			break;
		case SIGINT:/* CTRL C */
			printf("SIGINT\n");
			break;
		case SIGUSR1:/* CHILD */
			printf("SIGUSR1\n");
			break;
		case SIGUSR2: /* CHILD */
			printf("SIGUSR2\n");
			break;
		case SIGTSTP: /* CTRL Z */
			printf("SIGTSTP\n");
			break;
		case SIGQUIT:
			printf("SIGQUIT\n");
		default:
			break;
	}
}


int main( ){
	char *str = malloc(sizeof(char)*MAX_SHELL_LENGTH);
	char *command = malloc(sizeof(char)*50);
	char *redirection = malloc(sizeof(char)*1);
	char *file = malloc(sizeof(char)*100);
	char *extension = malloc(sizeof(char)*2);
	char *file2 = malloc(sizeof(char)*100);
	int size = 0, pipe_count, status, fd_stdin, fd_stdout;
	pid_t pid;
	signal(SIGINT, handler);
	signal(SIGTSTP, handler);
	signal(SIGUSR1, handler);
	signal(SIGUSR2, handler);
	signal(SIGQUIT, handler);

	fd_stdin = dup(STDIN_FILENO);	 
	fd_stdout = dup(STDOUT_FILENO);
	while(1)
	{ 
		pid  = fork( );
		
		if(pid == 0) /* This is going to be executed by the CHILD */
		{
			printf("myTerminal:$");
			fflush(stdout);
			fflush(stdin);
			
			if(fgets(str, MAX_SHELL_LENGTH, stdin) != NULL)
			{
				
				size = (int)(strlen(str));
				pipe_count = findPipeCount(str);
				str[size - 1] = '\0'; /* Extract the new line character */
				
				if(strcmp(str, ":q") == 0) 
				{
					kill(getppid(), SIGTERM);
					_exit(EXIT_SUCCESS);
				}
				
				if(pipe_count > 0)
				{
					executePipedCommands(str, pipe_count);
				}
				else
				{
					if(splitCommand(str, command, extension, redirection, file, file2) == 1)
					{
						if(executSingleCommand(command,extension,redirection, file, file2) == 1)
							_exit(127);  
					}	
					else						
						exit(0);
				}
			}
		}
		else
		{	/* This scope is going to be executed by the Parent */

			wait(&status); /* Child is terminated */
			dup2(fd_stdin, STDIN_FILENO);
			dup2(fd_stdout, STDOUT_FILENO);
			str[0] = '\0';
			command[0] = '\0';
			redirection[0] = '\0';
			file2[0] ='\0';
			file[0] = '\0';
			extension[0] = '\0';
		}
	}
	free(str);
	free(command);
	free(extension);
	free(file);
	free(redirection);
	free(file2);
	return 0;
}

/* This function is going to print multiple pipped commands */
int executePipedCommands(char* str, int pipes)
{
	/* Initalazation of strings */
	char *file , *file2;
	int index = 0, i = 0, status;
	int fd[2];
	pid_t pid;
	
	char *command1, *command2, *cmd, *_ext, *_redir;
	
	 cmd = malloc(sizeof(char)*20); /* ls cd cat , etc. */
	 _ext = malloc(sizeof(char)*2); /* -l, -n, -s, etc */
	 _redir = malloc(sizeof(char)*1); /* < > */
	 file = malloc(sizeof(char)*50); /* file name */
	 file2 = malloc(sizeof(char)*50); /*file name 2 */
	 command1 = malloc(sizeof(char)*100);
	 command2 = malloc(sizeof(char)*100);

	if(pipes == 1)
	{
		for(index = 0 ; str[index]!='|'; index++);
	}
	
	if(pipe(fd) < 0)
	{
		perror("pipe");
		return -1;
	}
	pid = fork( ); /* CALL FORK */
	if(pid < 0)
	{
		perror("fork");
		return -1;
	}
	

	if(pid == 0) /* Child starts executing ... */
	{
		close(fd[0]); /* Close read end of pipe */
				
		for(i = 0 ; i < index; i++)
			command1[i] = str[i];
		
		command1[index] = '\0'; /* Get the first command */		
				
		if(splitCommand(command1,cmd,_ext,_redir,file, file2) != 1)
		{
			perror("command1");
			exit(0);
		}
		int file_fd = 0;
		char *execute[] = {cmd, file2, NULL};
		char *execute2[] = {cmd, _ext, file2, NULL};
		char *execute3[] = {cmd, NULL};
		char *execute4[] = {cmd, _ext, NULL};
		/*Check output type*/
		if(_redir[0]=='>') /* FILE OUTPUT */
		{	
			file_fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			dup2(file_fd, STDOUT_FILENO); /* redirect to file */
		}	
		else if(_redir[0]=='<')
		{
			file_fd = open(file, O_RDONLY, 0666);
			dup2(file_fd, STDIN_FILENO);
		}
		else /* Output will be written to the pipe */
			dup2(fd[1], STDOUT_FILENO); /* Redirects stdout to pipe's write end */
	
		if(file2[0]!='\0') /* FILE2 EXIST */
		{
			if(_ext[0]=='\0')
				execvp(execute[0], execute);
			else
				execvp(execute2[0], execute2);				
		}	
		if(file2[0] == '\0') /* FILE2 NOT EXIST */
		{
			if(_ext[0]=='\0')
				execvp(execute3[0], execute3);
			else
				execvp(execute4[0], execute4);
		}
			close(fd[1]);
			exit(0);
	}
	else /* Parent starts executing ... */             
	{                                                           /* Split command 2 */ 
		wait(&status); /* Wait for child to terminate */
		close(fd[1]); // Close write end of pipe */
		int counter = 0;
		

		for(i = index + 1; str[i]!='\0'; i++)
		{	
			command2[counter] = str[i];
			counter++;
		}	
		command2[i] = '\0'; /* command2 is taken */
		if(splitCommand(command2,cmd,_ext,_redir,file, file2) != 1) /* Command2 is parsed */
		{
			perror("command2");
			exit(0);
		}
		dup2(fd[0], STDIN_FILENO);

		char *execute[] = {cmd, file2, NULL};
		char *execute2[] = {cmd, _ext, file2, NULL};
		char *execute3[] = {cmd, NULL};
		char *execute4[] = {cmd, _ext, NULL};
		int fd_file;
		
		if(_redir[0] == '>')
		{
			fd_file = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			if(fd_file < 0)
			{
				perror("open");
				exit(0);
			}
			dup2(fd_file, STDOUT_FILENO); /* Write to file */
		}
	
		if(file2[0]!='\0') /* FILE2 EXIST */
		{
			if(_ext[0]=='\0')
			{
				execvp(execute[0], execute);
			}	
				
			else
			{	
				execvp(execute2[0], execute2);				
			}
		}	
		if(file2[0] == '\0') /* FILE2 NOT EXIST */
		{
			if(_ext[0]=='\0')
			{
				execvp(execute3[0], execute3);
			}
			else
			{
				execvp(execute4[0], execute4);
			}
		}

		close(fd[0]);
		exit(0);
	}
	/* it will not be free */
	 free(cmd);
	 free(_ext);
	 free(_redir);
	 free(file);
	 free(file2);
	 free(command1);
	 free(command2);
	return 1;
}	



int executSingleCommand(char* command,char* extension, char* redirection_symbol, char*file, char* file2)
{
	if(redirection_symbol[0] != '\0') /* To set STDIN STDOUT */
	{
		
		if(redirection_symbol[0] == '>')
		{
			int fd = open(file ,O_WRONLY | O_CREAT, 0666);
			if(fd < 0)
			{
				perror("open");
				return -1;
			}
			dup2(fd, STDOUT_FILENO);
		}
		else if(redirection_symbol[0] == '<')
		{
			int fd = open(file, O_RDONLY, 0666);
			if(fd < 0)
			{
				perror("open");
				printf("%s: No such file or directory\n",file);
				return -1;
			}
			dup2(fd, STDIN_FILENO);
		}
	}
	if(file2[0] != '\0') /* Executing stage */
	{	
		if(extension[0] != '\0')
		{
			char *cmd [] = {command, extension, file2, NULL};
			execvp(cmd[0],cmd);
			perror("execvp");
		}
		else
		{
			char *cmd [] = {command, file2, NULL};
			execvp(cmd[0],cmd);
			perror("execvp");
		}
	}
	else
	{	
		if(extension[0] != '\0')
		{
			char *cmd [] = {command, extension, NULL};
			execvp(cmd[0],cmd);
		}
		else
		{
			char *cmd [] = {command, NULL};
			execvp(cmd[0], cmd);
			perror("execvp");
		}
	}
	return 1;
}


/** 
 * Checks if command valid, and splits it accordingly
 *
 */
int splitCommand(char*str ,char* command,char* extension, char* redirection_symbol, char*file, char* file2) 
{
	int counter = 0, flag = 0;
	char *path = malloc(100*sizeof(char));
	char *path2 = malloc(100*sizeof(char));
	char str2 [200]; 
	char *bin = "/usr/bin/";
	strcpy(str2, str);
	char *token = strtok(str2, " ");
	
	while(token!=NULL)
	{
		
		switch(counter)
		{
			case 0:  /* Command is taken */
				strcpy(command, token);
				break;
			case 1: 
				if((strlen(token) == 1) && (token[0] == '<' || token[0] == '>'))
				{
					strcpy(redirection_symbol, token);
					counter++;
				} 
				/* if it is an extension symbol -l or -n */
				else if(strlen(token) == 2 && token[0] == '-')
					strcpy(extension, token);
				/* The last option is to check whether its a file */
				else /* File or a special string > touch mete, grep xyz, etc.*/ 
					strcpy(file2, token);
				break;
			case 2: 
				if((strlen(token) == 1) && (token[0] == '<' || token[0] == '>'))
					strcpy(redirection_symbol, token);
				else if(strlen(token) == 2 && token[0] == '-')
					strcpy(extension, token);
				else
					strcpy(file2, token);
				break;	
			/* ls mete > */
			case 3: 
				if((strlen(token) == 1) && (token[0] == '<' || token[0] == '>'))
				{
					strcpy(redirection_symbol, token);
					flag = 1;
					token = strtok(NULL," ");
					if(token!=NULL)
						strcpy(file, token);
				}
				else
					strcpy(file, token);
				
				break;
			default:
				break;
		}
		if(flag == 1) break;

		token = strtok(NULL," ");
		counter++;
	}
	
	sprintf(path,"%s%s",bin, command);
	sprintf(path2,"%s%s","/bin/",command);
	if(access(path, F_OK) != 0 && access(path2, F_OK)!=0) /* Check if command exist, return -1 if not*/
	{

		printf("Command '%s' is not found\n",command);
		free(path);
		return -1;
	}
	
	if(redirection_symbol[0] != '\0'){
		if(redirection_symbol[0] != '<' && redirection_symbol[0] != '>')
		{
			printf("%s: invalid option -- '%c'\n",command, redirection_symbol[0]);
			free(path);			
			return -1;
		}	
	}
	if(extension[0] != '\0'){
		if(strlen(extension) > 2)
		{
			printf("%s: '%s' not found\n",command, extension);
			free(path);			
			return -1;
		}
	}
	free(path);
	free(path2);
	return 1; /* Command is valid */
}

const int findPipeCount(char *str) /* Returns the number of pipes */
{	
	int i = 0;
	int number_of_pipes = 0;
	for(i = 0; str[i]!= '\0'; i++)
		if(str[i] == '|')
			number_of_pipes++;
	
	return number_of_pipes;
}