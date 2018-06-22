#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#define SZ_SHMID_CONFIG 	"shm_config"
#define CB_SHM_KEY			64

int g_bUseKey = 0;

struct ST_SHM_DATA
{
	char szText[UCHAR_MAX];
};

void Usage()
{
	printf("Usage: \n");
	printf("\tsysV_shm <use_key>\n");
	printf("\tuse_key: boolean value. 0: don't use key.  1: use key (key was specified in code)\n");
}

void description()
{
	printf("=========================================================\n");
	printf("This is share memory of SVR4(System V Release 4)\n");
	printf("=========================================================\n");
}

void child()
{
	int iShmId = 0;
	if(!g_bUseKey)
	{
		// read id from config file that created by parent process
		int fdShmCfg = 0;
		if(-1 == (fdShmCfg = open(SZ_SHMID_CONFIG, O_RDONLY)))
		{
			printf("failed to open file [%s(%d)]\n", strerror(errno), errno);
			_exit(0);
		}
		char szBuf[UCHAR_MAX] = "";
		if(-1 == read(fdShmCfg, szBuf, UCHAR_MAX))
			printf("failed to read [%s(%d)]\n", strerror(errno), errno);
		iShmId = atoi(szBuf);
	}
	else
	{
		iShmId = shmget(CB_SHM_KEY, sizeof(struct ST_SHM_DATA), 0660);
	}
	printf("(child) share memory id is %d\n", iShmId);

	// show informatio
	struct shmid_ds stShmInfo;
	if(-1 == shmctl(iShmId, IPC_STAT, &stShmInfo))
		printf("failed to get information[%s(%d)]\n", strerror(errno), errno);
	else
	{
		printf("(child) owner = %d.%d, perms = %04o, max bytes = %lu\n",
				stShmInfo.shm_perm.uid,
				stShmInfo.shm_perm.gid,
				stShmInfo.shm_perm.mode,
				stShmInfo.shm_segsz);
		printf("(child) number of attahed = %d\n", stShmInfo.shm_nattch);
	}

	// read data from share memory
	struct ST_SHM_DATA *pstBuf = (struct ST_SHM_DATA *) shmat(iShmId, (void *) 0, 0);
	if(!pstBuf)
		printf("address is 0.[%s(%d)]\n", strerror(errno), errno);
	else
	{
		printf("(child) '%s' in share memory\n", pstBuf->szText);
		if(-1 == shmdt(pstBuf))
			printf("failed to detach.[%s(%d)]\n", strerror(errno), errno);
	}
}

void parent()
{
	int iShmId = 0;
	// create share memory
	// 1) use PRIVATE key to generate sharem id and store id into file
	if(!g_bUseKey)
	{
		if(-1 != (iShmId = shmget(IPC_PRIVATE, sizeof(struct ST_SHM_DATA), IPC_CREAT | 0660)))
		{
			int fdShmCfg = 0;
			if(-1 == (fdShmCfg = open(SZ_SHMID_CONFIG, O_CREAT | O_TRUNC | O_WRONLY, 0660)))
				exit(1);
			else
			{
				int len = 0;
				char id[UCHAR_MAX] = "";
				printf("write shm id %d on config\n", iShmId);
				len = snprintf(id, UCHAR_MAX, "%d", iShmId);
				write(fdShmCfg, &id, sizeof(char) * len);
				close(fdShmCfg);
			}
		}
	}
	// 2) use key to generate share memory id
	else
		iShmId = shmget(CB_SHM_KEY, sizeof(struct ST_SHM_DATA), IPC_CREAT | 0660);
	if(-1 == iShmId)
	{
		printf("failed to create share memory[%s(%d)]\n", strerror(errno), errno);
		exit(1);
	}
	printf("share memory id is %d\n", iShmId);

	// show informatio
	struct shmid_ds stShmInfo;
	if(-1 == shmctl(iShmId, IPC_STAT, &stShmInfo))
		printf("failed to get information[%s(%d)]\n", strerror(errno), errno);
	else
	{
		printf("owner = %d.%d, perms = %04o, max bytes = %lu\n",
				stShmInfo.shm_perm.uid,
				stShmInfo.shm_perm.gid,
				stShmInfo.shm_perm.mode,
				stShmInfo.shm_segsz);
		printf("number of attahed = %d\n", stShmInfo.shm_nattch);
	}

	// wirte data into share memory
	struct ST_SHM_DATA *pstBuf = (struct ST_SHM_DATA *) shmat(iShmId, (void *) 0, 0);
	if(!pstBuf)
		printf("address is 0.[%s(%d)]\n", strerror(errno), errno);
	else
	{
		strcpy(pstBuf->szText, "Hello ! share memory !");
		printf("write '%s' in memory\n", pstBuf->szText);
	}

	// wait child to exit
	wait(NULL);

	// detach and remove
	if(-1 == shmdt(pstBuf))
		printf("failed to detach.[%s(%d)]\n", strerror(errno), errno);
	if(-1 == shmctl(iShmId, IPC_RMID, NULL))
		printf("failed to remove sharem memory.[%s(%d)]\n", strerror(errno), errno);

	// unlink
	if(-1 == unlink(SZ_SHMID_CONFIG))
		printf("failed to unlink .[%s(%d)]\n", strerror(errno), errno);
}

int main(int argc, char *argv[])
{
	int iChildPid = 0;

	if(2 > argc)
	{
		Usage();
		exit(0);
	}
	description();
	g_bUseKey = atoi(argv[1]);
	printf("<use key> is %d\n", g_bUseKey);

	switch((iChildPid = fork()))
	{
		case -1:
			exit(0);

		case 0:
			// wait parent to create share mermoies
			sleep(1);
			child();
			_exit(0);

		default:
			parent();
			break;
	}

	return 0;
}
