#include<stdio.h>
#include"foo.h"

int foo()
{
	printf("foo: This is foo!\n");
	return 0;
}

#ifdef _TEST_
int main()
{
	int iRet = 0;
	printf("TEST: Call foo()...\n");
	foo();
	return 0;
}
#endif
