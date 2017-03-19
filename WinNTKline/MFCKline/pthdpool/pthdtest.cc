#include "pthdpool.h"
#include <stdio.h>

int m=0;

void *my_routine(void *lp)
{
	int * t = (int*)lp;
	while(m < 10)
	{
		printf("%d[%d]\n",m,*t);
		m++;
	}
}

int main()
{
	int s=3;
	my_pool *pool = my_pool_init(10,8);
	m=my_pool_add_task(pool, &my_routine, (void*)&s);
	printf("error=%d\n",m);
	s++;
	my_pool_wait(pool);
	m=my_pool_add_task(pool, &my_routine, (void*)&s);
	printf("error=%d\n",m);
	s++;
	my_pool_wait(pool);
	m=my_pool_add_task(pool, &my_routine, (void*)&s);
	printf("error=%d\n",m);
	my_pool_destroy(pool);
}
