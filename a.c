#define __LIBRARY__
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#define BUFFER_SIZE 20	/* 缓冲区数量 */
#define NUM 25			/* 产品总数 */

#define __NR_sem_open 87
#define __NR_sem_wait 88
#define __NR_sem_post 89
#define __NR_sem_unlink 90
#define __NR_sem_waits 91
#define __NR_sem_posts 92

int waiting = 0;
typedef void sem_t;  /* 下面用到sem_t*的，其实质都是指针，故在此将sem_t*类型定义为void*类型，
						就不用在此重新定义sem_t的结构体，因此也方便了对结构体的管理：如果在
						此重新定义此结构体，内核中也已经定义了此结构体，如需对其修改，则同时
						需要修改此处和内存处两处的结构体 */

_syscall2(int, sem_open, const char*, name, unsigned int, value)
_syscall1(int, sem_wait, sem_t*, sem)
_syscall1(int, sem_post, sem_t*, sem)
_syscall1(int, sem_unlink, const char*, name)
_syscall1(int, sem_waits, sem_t*, sem)
_syscall1(int, sem_posts, sem_t*, sem)
int main()
{
	/* 注意: 在应用程序中不能使用断点等调试功能 */
	int i, j, q;

	int barber_id;
	int customer_id;
	sem_t* barber, * customer, * mutex;
	FILE* fp = NULL;
	pid_t customer_pid, barber_pid;

	/* 创建empty、full、mutex三个信号量 */
	barber = (sem_t*)sem_open("barber", 0);
	customer = (sem_t*)sem_open("customer", 0);
	mutex = (sem_t*)sem_open("mutex", 1);

	/* 用文件建立一个共享缓冲区 */
	fp = fopen("filebuffer.txt", "wb+");

	/* 创理发师1进程 */
	if (!fork())
	{
		barber_pid = getpid();
		printf("barber1 pid=%d create success!\n", barber_pid);
		for (i = 0; i < 10; i++)
		{
			sem_wait(customer);
			sem_wait(mutex);
			sem_post(barber);
			sem_post(mutex);
			sleep(3);
			printf("barber1 pid=%d : is  cutting\n", barber_pid);
			fflush(stdout);
		}
		exit(0);
	}
	/*创理发师2进程*/
	if (!fork())
	{
		barber_pid = getpid();
		printf("barber2 pid=%d create success!\n", barber_pid);
		for (q = 0; q < 10; q++)
		{
			sem_wait(customer);
			sem_wait(mutex);
			sem_post(barber);
			sem_post(mutex);
			sleep(4);
			printf("barber2 pid=%d : is cutting\n", barber_pid);
			fflush(stdout);
		}
		exit(0);
	}
	/* 创建顾客进程 */
	if (!fork())
	{
		customer_pid = getpid();
		printf("\t\t\tCustomer pid=%d create success!\n", customer_pid);
		for (j = 0; j < NUM; j++)
		{
			sem_wait(mutex);

			if (waiting < 20)
			{
				customer_id = j;
				sem_post(customer);
				waiting++;
				sem_post(mutex);
				sem_wait(barber);
				printf("\t\t\t%d Customer is waiting\n", customer_id);
				fflush(stdout);
				/*waiting--;*/
			}
			else
			{
				customer_id = j;
				sem_post(mutex);
				printf("\t\t\t%d Customer is leaving\n", customer_id);
				fflush(stdout);
			}
		}
		exit(0);
	}

	waitpid(customer_pid, NULL, 0);	
	waitpid(barber_pid, NULL, 0);
	waitpid(barber_pid, NULL, 0);
	/* 关闭所有信号量 */
	sem_unlink("customer");
	sem_unlink("barber");
	sem_unlink("mutex");

	/* 关闭文件 */
	fclose(fp);

	return 0;
}
