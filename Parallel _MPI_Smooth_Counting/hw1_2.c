#include "mpi.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

double f(double);

double f(double a)
{
            return (4.0 / (1.0 + a*a));
}

int main(int argc,char *argv[])
{
            long long int n;
            int    myid, numprocs, i, local_n,toss,local_sum,sum,rec_sum;
            double PI25DT = 3.141592653589793238462643;
            double mypi, pi, h, x, y;
            double startwtime = 0.0, endwtime;
            int    namelen;
            char   processor_name[MPI_MAX_PROCESSOR_NAME];
            int level,no_level,ilevel;
            int odd = 0;

            scanf("%lld",&n);
            
            MPI_Init(&argc,&argv);
            MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
            MPI_Comm_rank(MPI_COMM_WORLD,&myid);
            MPI_Get_processor_name(processor_name,&namelen);
            MPI_Status status;


            fprintf(stdout,"Process %d of %d is on %s\n",myid, numprocs, processor_name);
            fflush(stdout);

            local_n = n;

            if (myid == 0)startwtime = MPI_Wtime();


            MPI_Bcast(&n, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

            h   = 1.0 / (double) n;
            sum = 0.0;
                                                                                /* A slightly better approach starts from large i and works back */
            for (toss = myid; toss <= n; toss+=numprocs)					//share the toss with processor
			{                     //(toss = 0;toss<=local_n;toss++)
                        x = ((2*(double)rand())/RAND_MAX)-1;				//rand x y
                        y = ((2*(double)rand())/RAND_MAX)-1;
                        if(x*x+y*y<=1)local_sum++;							//if inside the circle of radius 1
            }
            
            
            
            level = (numprocs+1)/2;
            no_level = (int)(log10((double)(numprocs)) / log10((double) 2));	//count the height of tree
            
            for(ilevel = 0;ilevel<no_level;ilevel++)
			{
                  MPI_Status status;
                  if(myid>=level)
				  {
                        MPI_Send(&local_sum,1,MPI_INT,myid%level,1, MPI_COMM_WORLD);	//send if at the right side of tree
                  }
                  else
				  {
                        MPI_Recv(&rec_sum,1,MPI_INT,myid+level,1,MPI_COMM_WORLD,&status);	//receive  if at the left side of tree
                        local_sum += rec_sum;
                  }
                  level = (level+1)/2;			//go to next layer
            }
            if (myid == 0) 
			{
                    pi = local_sum;
                    pi = 4*(double)pi/(double)n;
                    endwtime = MPI_Wtime();
                    printf("pi is approximately %.16f, Error is %.16f\n",pi, fabs(pi - PI25DT));
                    printf("wall clock time = %f\n", endwtime-startwtime);
                    fflush(stdout);
            }

            MPI_Finalize();
            return 0;
}

