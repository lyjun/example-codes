#include<stdio.h>
#include"foo.h"

int main()
{
	int iRet = 0;
	printf("Main: Call foo()...\n");
	foo();
	return 0;
}
