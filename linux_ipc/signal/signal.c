#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
int g_bChildStop = true;

void Handler_SIGUSER1(int iSignal)
{
	printf("(pid %d) I got signal(%d)\n", getpid(), iSignal);
	g_bChildStop = false;
}

void child()
{
	printf("child: register SIGUSR1 handler\n");
	if(SIG_ERR == signal(SIGUSR1, &Handler_SIGUSER1))
	{
		perror("kill errors");
		_exit(0);
	}

	// wait signal
	while(g_bChildStop)
		sleep(1);
}

void parent(int iChildPid)
{
	printf("Parent: send SIGUSR1 to child(%d)\n", iChildPid);
	// send signal to the process
	if(-1 == kill(iChildPid, SIGUSR1))
	{
		perror("kill errors");
		exit(0);
	}

	wait(NULL);
}

int main()
{
	int iChildPid = 0;

	switch((iChildPid = fork()))
	{
		case -1:
			perror("fork failed");
			exit(0);

		case 0:
			child();
			_exit(0);

		default:
			// wait child to register signal
			sleep(1);
			// send signal to child
			parent(iChildPid);
			exit(0);
	}
	return 0;
}
