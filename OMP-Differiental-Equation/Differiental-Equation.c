#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

float** x ;
float** xn ;
int n, threads;
float error ;

int main (int argc, char *argv[] )
{
	int i, j, k, conv, conv_total;
	double start_time, end_time;
	/* the array size should be supplied as a command line argument */
	if(argc != 3) 
	{
		printf("wrong number of arguments"); 
		exit(2);
	}
	n = atoi(argv[1]) ;
	threads = atoi(argv[2]);
	
	printf("Array size = %d \n", n );
	printf("Number of threads = %d \n", threads);
	
	x = malloc(sizeof(float*)*(n+2));
  	for (i = 0; i < n+2; i++) 
  	{
  		x[i] = malloc(sizeof(float)*(n+2));
	}
  	xn = malloc(sizeof(float*)*(n+2));
  	for (i = 0; i < n+2; i++) 
  	{
  		xn[i] = malloc(sizeof(float)*(n+2));
  	}

  	for(k=1 ; k <= n ; k++)
  	{
  		x[0][k] = 0; 
  		x[k][0] = 0; 
  		x[n+1][k] = k; 
  		x[k][n+1] = k; 
  	}
	for (i=1 ; i <= n ; i++)
		for (j=1 ; j <= n ; j++)
			x[i][j] = 0 ;
	
	k = 1;
	
	#pragma omp parallel num_threads(threads) default(none)  \
		firstprivate(i, j, error) shared(k, xn, conv, x, n, start_time, end_time)
	{
		#pragma omp single
		start_time = omp_get_wtime();
		while(k < 10000) /* put an upper bound to prevent infinite loops */
		{
			conv = 0;
			#pragma omp for reduction(+:conv)
			for (i=1 ; i <= n ; i++)
			{
				for (j=1 ; j <= n ; j++)
				{
					xn[i][j] = 0.25 * (x[i-1][j]+x[i+1][j]+x[i][j-1]+x[i][j+1]);
					if(xn[i][j] <= x[i][j]) 
						error = x[i][j] - xn[i][j] ;
					else   
						error = xn[i][j] - x[i][j] ;
					if(error <= 0.001) 
					{
						conv++;
					}
				}
			}
			
			if(conv == n*n) 
				break;
			
			#pragma omp for
			for (i=1 ; i <= n ; i++) 
			{
				for (j=1 ; j <= n ; j++)
				{
					x[i][j] = xn[i][j] ;  
				}
			}
			#pragma omp single
			k++;
		}
		#pragma omp single
		end_time = omp_get_wtime();
	}
	
	printf("Elapsed time: %f \n", end_time - start_time);
    printf("number of iterations is %d \n", k) ;
    printf("print some diagonal elements for checking results:\n");
    for (i=1 ; i <= n ; i = i + n/8)  
    	printf("%f  ", x[i][i]) ;
	printf("\n") ;
	return 0;
}

