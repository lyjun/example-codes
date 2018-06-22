#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SZ_FIFO_PATH	"/tmp/fifo-example"
#define SZ_HELLO		"hello mkfifo !"

void child()
{
	// open fifo for reading
	int fdFifo = 0;
	if(-1 == (fdFifo = open(SZ_FIFO_PATH, O_RDONLY)))
	{
		perror("open error");
		exit(EXIT_FAILURE);
	}

	printf("child: receive => ");
	fflush(stdout);
	char buf;
	while(read(fdFifo, &buf, 1) > 0)
		write(STDOUT_FILENO, &buf, 1);
	write(STDOUT_FILENO, "\n", 1);
	close(fdFifo);
	_exit(EXIT_SUCCESS);
}

void parent(pid_t icPid)
{
	printf("Parent: send '%s' to child(%u)\n", SZ_HELLO, icPid);
	// create fifo with name
	if(-1 == mkfifo(SZ_FIFO_PATH, S_IRWXU))
	{
		perror("mkfifo error");
		exit(EXIT_FAILURE);
	}

	// open fifo for writing
	int fdFifo = 0;
	if(-1 == (fdFifo = open(SZ_FIFO_PATH, O_WRONLY)))
	{
		perror("open error");
		exit(EXIT_FAILURE);
	}

	write(fdFifo, SZ_HELLO, strlen(SZ_HELLO));
	close(fdFifo);
	wait(NULL);
}

int main()
{
	/*
	 *  it has to be open at both ends simultaneously before you can proceed to do any input or output operations on it
	 */
	pid_t icPid = 0;

	unlink(SZ_FIFO_PATH);
	switch((icPid = fork()))
	{
		case -1:
			perror("fork error");
			exit(EXIT_FAILURE);
			break;

		case 0:
			// child
			// wait a comment for creating fifo
			sleep(1);
			child();
			break;

		default:
			// parent
			parent(icPid);
			break;
	}
}
