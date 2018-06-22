#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#define CB_MSQKEY			1078
#define SZ_MSQID_CONFIG 	"msq_config"
#define CB_MSQ_DATA_SIZE	64

int g_bUseKey = false;

struct stMsqBuf
{
	long  mtype;
	char mdata[1];
};

void Usage()
{
	printf("Usage: \n");
	printf("\tsysV_msq <use_key>\n");
	printf("\tuse_key: boolean value. 0: don't use key.  1: use key (key was specified in code)\n");
}

void description()
{
	printf("=========================================================\n");
	printf("This is message queue of SVR4(System V Release 4)\n");
	printf("=========================================================\n");
}

void child()
{
	// read message queue id
	int idMsq = 0;
	if(!g_bUseKey)
	{
		// read id from config file that created by parent process
		int fdMsqCfg = 0;
		if(-1 == (fdMsqCfg = open(SZ_MSQID_CONFIG, O_RDONLY)))
		{
			printf("failed to open file [%s(%d)]\n", strerror(errno), errno);
			_exit(0);
		}
		char szBuf[UCHAR_MAX] = "";
		if(-1 == read(fdMsqCfg, szBuf, UCHAR_MAX))
			printf("failed to read [%s(%d)]\n", strerror(errno), errno);
		idMsq = atoi(szBuf);
	}
	else
	{
		if (-1 == (idMsq = msgget(CB_MSQKEY, O_RDONLY)))
			exit(1);
	}
	printf("(child) got message qeueue id '%d'\n", idMsq);

	// get message queue data structure
	struct msqid_ds buf;
	if (-1 == msgctl(idMsq, IPC_STAT, &buf))
		printf("failed to get information [%s(%d)]\n", strerror(errno), errno);
	else
	{
		printf("(child) owner = %d.%d, perms = %04o, max bytes = %lu\n",
				buf.msg_perm.uid,
				buf.msg_perm.gid,
				buf.msg_perm.mode,
				buf.msg_qbytes);
		printf("(child) %lu msgs = %lu bytes on queue\n", buf.msg_qnum, buf.msg_cbytes);
	}

	// read data
	struct stMsqBuf *pstMsqBuf = NULL;
	pstMsqBuf = (struct stMsqBuf *) calloc(1, sizeof(long) + CB_MSQ_DATA_SIZE);

	// If msgtyp is 0, then the first message in the queue is read.
	//
	// the calling process is blocked until one of the following conditions occurs: (without IPC_NOWAIT)
	// 1) A message of the desired type is placed in the queue.
	// 2) The message queue is removed from the system. In this case the system call fails with errno set to EIDRM.
	int iRet = 0;
	do
	{
		// read type 2 first
		if(-1 != (iRet = msgrcv(idMsq, pstMsqBuf, CB_MSQ_DATA_SIZE, 2, IPC_NOWAIT)))
			printf("(child) Ret = %d; Data = '%s'\n", iRet, pstMsqBuf->mdata);
		// read type 1 
		else if(-1 != (iRet = msgrcv(idMsq, pstMsqBuf, CB_MSQ_DATA_SIZE, 1, IPC_NOWAIT)))
			printf("(child) Ret = %d; Data = '%s'\n", iRet, pstMsqBuf->mdata);
		// read message in front of queue
		else if(-1 != (iRet = msgrcv(idMsq, pstMsqBuf, CB_MSQ_DATA_SIZE, 0, IPC_NOWAIT)))
			printf("(child) Ret = %d; Data = '%s'\n", iRet, pstMsqBuf->mdata);
		// show error
		else
			printf("(child) failed to rec [%s(%d)]\n", strerror(errno), errno);
	}while(-1 != iRet);
}

void parent()
{
	// get a message queue id
	int idMsq = -1;
	if(!g_bUseKey)
	{
		if (-1 == (idMsq = msgget(IPC_PRIVATE, IPC_CREAT | 0660)))
			exit(1);

		// write message queue id to file
		// let child reads the id from this file
		int fdMsgCfg = -1;
		if(-1 == (fdMsgCfg = open(SZ_MSQID_CONFIG, O_CREAT | O_TRUNC | O_WRONLY, 0660)))
			exit(1);
		else
		{
			int len = 0;
			char id[UCHAR_MAX] = "";
			printf("write msq id %d on config\n", idMsq);
			len = snprintf(id, UCHAR_MAX, "%d", idMsq);
			write(fdMsgCfg, &id, sizeof(char) * len);
			close(fdMsgCfg);
		}
	}
	else
	{
		if (-1 == (idMsq = msgget(CB_MSQKEY, IPC_CREAT | 0660)))
			exit(1);
	}
	printf("Message queue id is %d\n", idMsq);

	// get message queue data structure
	struct msqid_ds buf;
	if (-1 == msgctl(idMsq, IPC_STAT, &buf))
		printf("failed to get information [%s(%d)]\n", strerror(errno), errno);
	else
	{
		printf("owner = %d.%d, perms = %04o, max bytes = %lu\n",
				buf.msg_perm.uid,
				buf.msg_perm.gid,
				buf.msg_perm.mode,
				buf.msg_qbytes);
		printf("%lu msgs = %lu bytes on queue\n", buf.msg_qnum, buf.msg_cbytes);
	}

	// set new buffer size
	buf.msg_qbytes = 1024;
	if (-1 == msgctl(idMsq, IPC_SET, &buf))
		printf("failed to setting size to %lu bytes [%s(%d)]\n", buf.msg_qbytes, strerror(errno), errno);
	else
		printf("setting size to %lu bytes\n", buf.msg_qbytes);

	//  send message to queue
	struct stMsqBuf *pstMsqData = NULL;
	pstMsqData = (struct stMsqBuf *) calloc(1, sizeof(long) + CB_MSQ_DATA_SIZE);
	// mtype can't be zero or nonpositive value. must be > 0
	for(int mtype = 1; mtype < 4; mtype++)
	{
		pstMsqData->mtype = mtype;
		snprintf(pstMsqData->mdata, CB_MSQ_DATA_SIZE, "%lu-%s", pstMsqData->mtype, "hello ! Message qeueue - SYS V");
		if(-1 == msgsnd(idMsq, pstMsqData, CB_MSQ_DATA_SIZE, 0))
			printf("failed to send [%s(%d)]\n", strerror(errno), errno);
		else
			printf("send '%s'\n", pstMsqData->mdata);
	}

	// wait child 
	wait(NULL);                

	// remove message queue 
	if (-1 == msgctl(idMsq, IPC_RMID, NULL))
		printf("failed to remove message queue %d [%s(%d)]\n", idMsq, strerror(errno), errno);
	else
		printf("remove message queue '%d'\n", idMsq);

	if(-1 == unlink(SZ_MSQID_CONFIG))
		printf("failed to unlink %s [%s(%d)]\n", SZ_MSQID_CONFIG, strerror(errno), errno);
}

int main(int argc, char *argv[]) 
{
	pid_t iChildPid = -1;

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
			printf("Can't fork process\n");
			exit(1);

		// child
		case 0:
			sleep(1);
			child();
			_exit(0);

		// parent
		default:
			parent();
			break;
	}

}
