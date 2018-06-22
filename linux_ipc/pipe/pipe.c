#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SZ_HELLO	"hello pipe"

int main(int argc, char *argv[])
{
	/*
	 *  it has to be open at both ends simultaneously before you can proceed to do any input or output operations on it
	 */
	int pipefd[2] = {0};
	pid_t cpid = 0;
	char buf;

	// create pipe
	if (-1 == pipe(pipefd)) 
	{
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	switch((cpid = fork()))
	{
		case  -1:
			perror("fork");
			exit(EXIT_FAILURE);

		// Child reads from pipe
		case 0:
			printf("Child: recive => ");
			fflush(stdout);
			// Close unused write end
			close(pipefd[1]);          

			while (read(pipefd[0], &buf, 1) > 0)
				write(STDOUT_FILENO, &buf, 1);

			write(STDOUT_FILENO, "\n", 1);
			close(pipefd[0]);
			_exit(EXIT_SUCCESS);

		// Parent writes SZ_HELLO to pipe
		default:	
			printf("Parent: send '%s' to child(%d)\n", SZ_HELLO, cpid);
			// Close unused read end
			close(pipefd[0]);          
			write(pipefd[1], SZ_HELLO, strlen(SZ_HELLO));
			// Reader will see EOF
			close(pipefd[1]);          
			// Wait for child 
			wait(NULL);                
			exit(EXIT_SUCCESS);
	}
}
