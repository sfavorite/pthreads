#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>



typedef struct
{
	double	*a;
	double	*b;
	double	sum;
	int		veclen;
} DOTDATA;

#define NUMTHRDS	4
#define	VECLEN		100

DOTDATA			dotstr;
pthread_t		callThd[NUMTHRDS];
pthread_mutex_t	mutexsum;

time_t end_time;

void *dotprod(void *arg)
{
	int	i, start, end, len;
	long offset;
	double mysum, *x, *y;
	time_t start_time = time(NULL);
	time_t end_time;
	time_t seconds = 120;
	
	end_time = start_time + seconds;
	// What section of the numbers are we working with.
	offset = (long)arg;
	
	// Which thread am I
	printf("Thread %ld\n", offset);
	
	
	len = dotstr.veclen;
	start = offset * len;
	end = start + len;
	x = dotstr.a;
	y = dotstr.b;
	
	mysum = 0;
	for (i = start; i < end; i++)
	{
		mysum += (x[i] * y[i]);
	}
	int status;
	int misses = 0;
	int done = 0;
	while (time (NULL) < end_time && done == 0) 
	{
		status = pthread_mutex_trylock(&mutexsum);
		if (status != EBUSY) {
			printf("Thread %ld now locking\n", offset);
			//pthread_mutex_lock(&mutexsum);
			printf("Thread %ld has a lock\n", offset);
			dotstr.sum += mysum;
			pthread_mutex_unlock(&mutexsum);
			printf("Thread %ld has unlocked\n", offset);
			done = 1;
			pthread_exit((void *) 0);
		} else {
			printf("Thread %ld is waiting for a lock...%ld seconds.\n", offset, time(NULL) - start_time);
			sleep(1);
		}
	}
	pthread_exit((void *) 0);
}

int main(int argc, char *argv[])
{
	long	i;
	double	*a, *b;
	void	*status;
	pthread_attr_t	attr;
	
	// Assign storage and initialize values
	a = (double *) malloc(NUMTHRDS * VECLEN * sizeof(double));
	b = (double *) malloc(NUMTHRDS * VECLEN * sizeof(double));
	
	end_time = time(NULL) + 60; // Run for 1 minute
	
	for (i = 0; i < VECLEN * NUMTHRDS; i++)
	{
		a[i] = 1.0;
		b[i] = a[i];
	}
	dotstr.veclen = VECLEN;
	dotstr.a = a;
	dotstr.b = b;
	dotstr.sum = 0;
	
	pthread_mutex_init(&mutexsum, NULL);
	// Create threads to perform the dotproduct
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	for (i = 0; i < NUMTHRDS; i++)
	{
		// Each thread works on a different set of data.
		// The offset is specified by i. The size of the data for each thread is 
		// indicated by VECLEN
		pthread_create(&callThd[i], &attr, dotprod, (void *)i);
	}
	
	pthread_attr_destroy(&attr);
	
	// Wait on the other threads
	for (i = 0; i < NUMTHRDS; i++)
	{
		pthread_join(callThd[i], &status);
	}
	
	// Print results and cleanup
	printf("Sum = %f\n", dotstr.sum);
	free(a);
	free(b);
	pthread_mutex_destroy(&mutexsum);
	pthread_exit(NULL);
}