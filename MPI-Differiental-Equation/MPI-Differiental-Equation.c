#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

#define first rank == 0
#define last rank == (num_of_processes - 1)

int main (int argc, char *argv[] )
{
	float **x, **xn, error=0;
	int i, j, n, k, conv, mpi_init, rank, num_of_processes, local_bound;
	double start_time, end_time;
	MPI_Status status;
	/* the array size should be supplied as a command line argument */
	if(argc != 2) 
	{
		printf("Wrong number of arguments\n"); 
		exit(2);
	}
	mpi_init = MPI_Init(&argc,&argv);
	if(mpi_init != MPI_SUCCESS)
	{
		printf("Error in MPI init\n");
		MPI_Abort(MPI_COMM_WORLD, mpi_init);
	}
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&num_of_processes);
	
	n = atoi(argv[1]) ;
	local_bound = n / num_of_processes;
	if(first)
	{
		printf("Array size = %d \n", n );
		printf("Number of processes = %d \n", num_of_processes);
	}
	
	x = calloc(local_bound + 2, sizeof(float *));
	xn = calloc(local_bound + 2, sizeof(float *));
	
  	for(i=0; i<(local_bound+2); i++) 
  	{
  		x[i] = calloc(n+2, sizeof(float));
  		xn[i] = calloc(n+2, sizeof(float));
  		x[i][n+1] = i + (rank * local_bound);
	}
	
	if(last)
	{
		for(j=0; j<(n+2); j++)
		{
			x[local_bound+1][j] = j;
		}
	}

	k = 1;
	start_time = MPI_Wtime();
	while(k < 10000) /*put an upper bound to prevent infinite loops*/
	{
		conv = 0;
		if(first)
		{
			if(num_of_processes != 1)
				MPI_Send(x[local_bound], n+2, MPI_FLOAT, rank+1, rank, MPI_COMM_WORLD);
			for(i=1; i < local_bound; i++)
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
			if(num_of_processes != 1)
				MPI_Recv(x[local_bound+1], n+2, MPI_FLOAT, rank+1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			i = local_bound;
			for(j=1; j <= n; j++)
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
		else if(last)
		{
			MPI_Send(x[1], n+2, MPI_FLOAT, rank-1, rank, MPI_COMM_WORLD);
			for(i=2; i < (local_bound+1); i++)
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
			MPI_Recv(x[0], n+2, MPI_FLOAT, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			
			i = 1;
			for(j=1; j <= n; j++)
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
		else
		{
			MPI_Send(x[1], n+2, MPI_FLOAT, rank-1, rank, MPI_COMM_WORLD);
			MPI_Send(x[local_bound], n+2, MPI_FLOAT, rank+1, rank, MPI_COMM_WORLD);
			for(i=2; i < local_bound; i++)
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
			MPI_Recv(x[0], n+2, MPI_FLOAT, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			MPI_Recv(x[local_bound+1], n+2, MPI_FLOAT, rank+1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			
			for(j=1; j <= n; j++)
			{
				i=1;
				xn[i][j] = 0.25 * (x[i-1][j]+x[i+1][j]+x[i][j-1]+x[i][j+1]);
				if(xn[i][j] <= x[i][j]) 
					error = x[i][j] - xn[i][j] ;
				else   
					error = xn[i][j] - x[i][j] ;
				if(error <= 0.001) 
				{
					conv++;
				}
				i=local_bound;
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
		
		if(num_of_processes != 1)
			MPI_Allreduce(&conv, &conv, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
		
		if(conv == n*n) 
			break;
		
		for (i=1 ; i < (local_bound+1); i++) 
		{
			for (j=1 ; j <= n ; j++)
			{
				x[i][j] = xn[i][j] ;  
			}
		}
		k++;
	}
	end_time = MPI_Wtime();
	
	if(first)
	{
		fprintf(stderr, "Elapsed time: %f \n", end_time - start_time);	
		fprintf(stderr, "Number of iterations is %d \n", k) ;
    	fprintf(stderr, "Diagonal Elements:\n");
    	fprintf(stderr, "Process:%d - ", rank);
		for(i=1, j=1; i < (local_bound+1); i += n/8, j += n/8)
		{
			fprintf(stderr, "%f  ", x[i][j]);
		}
		fprintf(stderr, "\n");
		if(num_of_processes != 1)
			MPI_Send(&j, 1, MPI_INT, rank+1, rank, MPI_COMM_WORLD);
	}
	else if(last)
	{
		MPI_Recv(&j, 1, MPI_INT, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		fprintf(stderr, "Process:%d - ", rank);
		if(j >= (rank * local_bound) && j < ((rank+1) * local_bound))
		{
			for(i = j % local_bound; i < (local_bound+1); j += n/8, i += n/8)
			{
				fprintf(stderr, "%f  ", x[i][j]);
			}
		}
		fprintf(stderr, "\n");
	}
	else
	{
		MPI_Recv(&j, 1, MPI_INT, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		fprintf(stderr, "Process:%d - ", rank);
		if(j >= (rank * local_bound) && j < ((rank+1) * local_bound))
		{
			for(i = j % local_bound; i < (local_bound+1); j += n/8, i += n/8)
			{
				fprintf(stderr, "%f  ", x[i][j]);
			}
		}
		fprintf(stderr, "\n");
		MPI_Send(&j, 1, MPI_INT, rank+1, rank, MPI_COMM_WORLD);
	}
	MPI_Finalize();
	for(i=0; i<(local_bound+2); i++) 
  	{
  		free(x[i]);
  		free(xn[i]);
	}
	free(x);
	free(xn);
	return 0;
}
