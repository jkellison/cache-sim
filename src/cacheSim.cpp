#include <stdio.h>
#include <cacheSim.h>

int main (int argc, char ** argv)
{

	if (argc > 1) printf("%s\r\n", argv[1]);
	
	int test0, test1;

	scanf("%d %d\n", &test0, &test1);

	printf("%d %d\r\n", test0, test1);

return 0;
}
