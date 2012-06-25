/*****************************************************************************/
/*Matrix vector multiplication                                               */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>

int N ;
int P;
pthread_t *threads;
float** a ;
float* b ;
float* x ;
double time_start, time_end;
void* do_work(void*);

main (int argc, char *argv[] )    {
/* the array size should be supplied as a command line argument */
  if(argc != 3) {printf("wrong number of arguments") ; exit(2) ;}
  N = atoi(argv[1]);
  P = atoi(argv[2]);
  printf("Array size = %d \n ", N );
  printf("Number of Threads = %d \n", P);
  int mid = (N+1)/2;
  int i, j;

/* allocate arrays dynamically */
  a = malloc(sizeof(float*)*N);
  for (i = 0; i < N; i++) {
		a[i] = malloc(sizeof(float)*N);
  }
  b = malloc(sizeof(float)*N);
  x = malloc(sizeof(float)*N);

  /* Inititialize matrix A and vector B. */
  for (i=0; i<N; i++) {
    for (j=0; j<N; j++) {
      if (j == i)                    { a[i][j] = 2.0; }
      else if (j == i-1 || j == i+1) { a[i][j] = 1.0; }
      else                           { a[i][j] = 0.01; }
    }
    b[i] = mid - abs(i-mid+1);
  }

  for (i=0; i<N; i++)
	  x[i] = 0.0;

  threads = malloc(sizeof(pthread_t) * P);
  for(i=0; i<P; i++)
	  pthread_create(&threads[i], NULL, do_work, (void *)(long)i);

  for(i=0; i<P; i++)
	  pthread_join(threads[i], NULL);

/* this is checking the results */
  for (i=0; i<N; i+=N/10) { printf(" %10.6f",x[i]); }
  printf("\n");
  printf ("time = %lf\n", time_end - time_start);
  free(threads);
  free(a);
  free(b);
  free(x);
}

void* do_work(void* args)
{
	struct timeval tv;
	struct timezone tz;
	int thread_id = (int)(long)args;
	int i, j;
	gettimeofday (&tv ,   &tz);
	time_start = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
 

	for (i=thread_id*(N/P); i<((thread_id+1)*(N/P)); i++) 
	{ 
		for (j=0; j<N; j++) 
		{
			x[i] += a[i][j] * b[j];
		}
	}
	gettimeofday (&tv ,  &tz);
	time_end = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

