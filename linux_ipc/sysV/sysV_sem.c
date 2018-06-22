#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#define SZ_SEM_CONFIG	"sem_config"	
#define CB_KEY_ID 		2048	
#define CB_MAX_SEM		5

bool g_bUseKey = false;

void usage ()
{
	printf("Usage:\n");
	printf("\tsysV_sem <use_key>\n");
	printf("\tuse_key: boolean value. 0: don't use key.  1: use key (key was specified in code)\n");
}

void description()
{
	printf("=========================================================\n");
	printf("This is semaphore of SVR4(System V Release 4)\n");
	printf("=========================================================\n");
}

/*
 * 	brief		Show semaphore set information
 *	iSemID:		the semaphore id
 *	reutnr:		none
 */
void show_sem_info(int iSemID)
{
	union semun arg;
	struct  semid_ds stSemStat;
	// **** assign pointer to the structure *****
	arg.buf = &stSemStat;
	if(-1 == semctl(iSemID, 0, IPC_STAT, arg))
	{
		printf("failed to semctl[%s(%d)]\n", strerror(errno), errno);
		return;
	}

#if 1
	printf("Sem #\t");
	for(int iItem = 0; iItem < arg.buf->sem_nsems; iItem++)
		printf("%4d", iItem);
	printf("\n");
	// get information one bye one
	printf("val\t");
	for(int iItem = 0; iItem < arg.buf->sem_nsems; iItem++)
		printf("%4u", semctl(iSemID, iItem, GETVAL, 0));
	printf("\n");
#else
	// get information and store these in arrray
	union semun uniSemAllVals;
	u_short arrAllVals[CB_MAX_SEM] = {0};
	// **** assign pointer to the structure *****
	uniSemAllVals.array = arrAllVals;
	if(-1 == semctl(iSemID, arg.buf->sem_nsems, GETALL, uniSemAllVals))
		printf("failed to semctl(GETALL) [%s(%d)]\n", strerror(errno), errno);
	else
	{
		printf("val\t");
		//for(int iItem = 0; iItem < arg.buf->sem_nsems; iItem++)
		for(int iItem = 0; iItem < CB_MAX_SEM; iItem++)
			printf("%4u", uniSemAllVals.array[iItem]);
		printf("\n");
	}
#endif
}

void do_one_semop(int iSemID, int idxSem, int iVal, int mFlg)
{
	// do one operation. if third argument is bigger than 1, the second argument must be an array.
	struct sembuf consumer;
	consumer.sem_num = idxSem;
	consumer.sem_op = iVal;
	consumer.sem_flg = mFlg;
	if(-1 == semop(iSemID, &consumer, 1))
		printf("==> failed to semop - idx = %d, val = %d [%s(%d)]\n", idxSem, iVal, strerror(errno), errno);
	else
		printf("==> semop - idx = %d, val = %d\n", idxSem, iVal);
}

void child_consumer()
{
	printf("=========================================================\n");
	printf("child \n");
	printf("=========================================================\n");

	int iSemID = 0;

	if(!g_bUseKey)
	{
		// read id from config file that created by parent process
		int fsSemCfg = 0;
		if(-1 == (fsSemCfg = open(SZ_SEM_CONFIG, O_RDONLY)))
		{
			printf("failed to open file [%s(%d)]\n", strerror(errno), errno);
			_exit(0);
		}
		char szBuf[UCHAR_MAX] = "";
		if(-1 == read(fsSemCfg, szBuf, UCHAR_MAX))
			printf("failed to read [%s(%d)]\n", strerror(errno), errno);
		iSemID = atoi(szBuf);
	}
	else
	{
		if (-1 == (iSemID = semget(CB_KEY_ID, O_RDONLY, 0660)))
		{
			printf("failed to semget [%s(%d)]\n", strerror(errno), errno);
			exit(1);
		}
	}
	printf("got message qeueue id '%d'\n", iSemID);

	show_sem_info(iSemID);
	// wait idx 0
	do_one_semop(iSemID, 0, -1, IPC_NOWAIT);
	show_sem_info(iSemID);
	// wait idx 1
	do_one_semop(iSemID, 1, -1, IPC_NOWAIT);
	show_sem_info(iSemID);
	// wait idx 2
	do_one_semop(iSemID, 2, -10, IPC_NOWAIT);
	show_sem_info(iSemID);

	// post idx 0
	do_one_semop(iSemID, 0, 1, IPC_NOWAIT);
	show_sem_info(iSemID);
	// wait idx 0
	do_one_semop(iSemID, 0, -1, IPC_NOWAIT);
	show_sem_info(iSemID);
	_exit(0);
}


void parent_init()
{
	printf("=========================================================\n");
	printf("Parent \n");
	printf("=========================================================\n");

	int iSemId = 0;
	
	// use IPC_PRIVATE to create semaphore
	if(!g_bUseKey)
	{
		if(-1 == (iSemId = semget(IPC_PRIVATE, CB_MAX_SEM, IPC_CREAT | 0660)))
		{
			printf("failed to semget [%s(%d)]\n", strerror(errno), errno);
			exit(0);
		}

		// write message queue id to file
		// let child reads the id from this file
		int fdSemCfg = -1;
		if(-1 == (fdSemCfg = open(SZ_SEM_CONFIG, O_CREAT | O_TRUNC | O_WRONLY, 0660)))
			exit(1);
		else
		{
			int len = 0;
			char id[UCHAR_MAX] = "";
			printf("write sem id %d on config\n", iSemId);
			len = snprintf(id, UCHAR_MAX, "%d", iSemId);
			write(fdSemCfg, &id, sizeof(char) * len);
			close(fdSemCfg);
		}
	}
	// use custom key to create semaphore
	else
	{
		if(-1 == (iSemId = semget(CB_KEY_ID, CB_MAX_SEM, IPC_CREAT | 0660)))
		{
			printf("failed to semget [%s(%d)]\n", strerror(errno), errno);
			exit(0);
		}
	}

	// show information
	printf("no initialization...\n");
	show_sem_info(iSemId);

	// set all
	union semun uniSetAllVals;
	u_short arrAllVals[CB_MAX_SEM] = {0, 1, 3, 4, 5};
	uniSetAllVals.array = arrAllVals;
	if(-1 == semctl(iSemId, CB_MAX_SEM, SETALL, uniSetAllVals))
		printf("failed to semctl(SETALL) [%s(%d)]\n", strerror(errno), errno);

	// set one
	union semun uniSetOneVals;
	uniSetOneVals.val = 0;
	// set first item
	if(-1 == semctl(iSemId, 0, SETVAL, uniSetOneVals))
		printf("failed to semctl(SETALL) [%s(%d)]\n", strerror(errno), errno);
	// set last item
	uniSetOneVals.val = 20;
	if(-1 == semctl(iSemId, CB_MAX_SEM - 1, SETVAL, uniSetOneVals))
		printf("failed to semctl(SETALL) [%s(%d)]\n", strerror(errno), errno);

	// show information
	printf("initialization...\n");
	show_sem_info(iSemId);

	// remove semaphore
	wait(NULL);
	if(-1 == semctl(iSemId, 0, IPC_RMID, 0))
		printf("failed to semctl(IPC_RMID) [%s(%d)]\n", strerror(errno), errno);
	unlink(SZ_SEM_CONFIG);
}


int main(int argc, char *argv[])
{
	if(2 > argc)
	{
		usage();
		exit(0);
	}
	description();
	g_bUseKey = atoi(argv[1]);
	printf("use_key is %d\n\n", g_bUseKey);

	pid_t iChildPid = 0;
	switch((iChildPid = fork()))
	{
		case -1:
			printf("failed to fork process\n");
			break;

		case 0:
			sleep(1);
			child_consumer();
			_exit(0);

		default:
			parent_init();
			break;
	}
	
	return 0;
}
