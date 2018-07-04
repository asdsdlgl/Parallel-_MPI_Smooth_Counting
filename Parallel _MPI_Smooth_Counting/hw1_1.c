#include <stdio.h>     // printf()
#include <limits.h>    // UINT_MAX
#include "mpi.h"
int checkCircuit (int, long);

int main (int argc, char *argv[]) {
        long i,max;               /* loop variable (64 bits) */
        int id = 0;           /* process id */
        int count = 0;        /* number of solutions */
        double startTime = 0.0, totalTime = 0.0;
        int numprocs,rec_sum;
        int level,no_level,ilevel;

        MPI_Init(&argc, &argv);

        MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
        MPI_Comm_rank(MPI_COMM_WORLD,&id);

        startTime = MPI_Wtime();	//time start

        max = UINT_MAX;
        MPI_Bcast(&max, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);


        for (i = id; i <= max; i+=numprocs)
		{
                count += checkCircuit (id, i);		//distribute the count with processor
        }



        level = (numprocs+1)/2;
        no_level = (int)(log10((double)(numprocs)) / log10((double) 2));		//count the height of tree
        
        for(ilevel = 0;ilevel<no_level;ilevel++)
		{
                MPI_Status status;
                if(id>=level)
                        MPI_Send(&count,1,MPI_INT,id%level,1, MPI_COMM_WORLD);		//send if the node at the right side of tree
                else
				{
                        MPI_Recv(&rec_sum,1,MPI_INT,id+level,1,MPI_COMM_WORLD,&status); //receive if the node at the left side of tree
                        count += rec_sum;										//count the total count
                }
                level = (level+1)/2;			//go to next layer
        }




        totalTime = MPI_Wtime() - startTime;	//time end
        printf("Process %d finished in time %f secs.\n", id, totalTime);

        MPI_Finalize();


        printf ("Process %d finished.\n", id);
        fflush (stdout);
        printf("\nA total of %d solutions were found.\n\n", count);
        return 0;
}

#define EXTRACT_BIT(n,i) ( (n & (1<<i) ) ? 1 : 0)
#define SIZE 32

int checkCircuit (int id, long bits) {
        int v[SIZE];        /* Each element is a bit of bits */
        int i;

        for (i = 0; i < SIZE; i++)
                v[i] = EXTRACT_BIT(bits,i);
        if ( ( (v[0] || v[1]) && (!v[1] || !v[3]) && (v[2] || v[3])
                                && (!v[3] || !v[4]) && (v[4] || !v[5])
                                && (v[5] || !v[6]) && (v[5] || v[6])
                                && (v[6] || !v[15]) && (v[7] || !v[8])
                                && (!v[7] || !v[13]) && (v[8] || v[9])
                                && (v[8] || !v[9]) && (!v[9] || !v[10])
                                && (v[9] || v[11]) && (v[10] || v[11])
                                && (v[12] || v[13]) && (v[13] || !v[14])
                                && (v[14] || v[15]) )
                        ||
                        ( (v[16] || v[17]) && (!v[17] || !v[19]) && (v[18] || v[19])
                          && (!v[19] || !v[20]) && (v[20] || !v[21])
                          && (v[21] || !v[22]) && (v[21] || v[22])
                          && (v[22] || !v[31]) && (v[23] || !v[24])
                          && (!v[23] || !v[29]) && (v[24] || v[25])
                          && (v[24] || !v[25]) && (!v[25] || !v[26])
                          && (v[25] || v[27]) && (v[26] || v[27])
                          && (v[28] || v[29]) && (v[29] || !v[30])
                          && (v[30] || v[31]) ) )
        {
                printf ("%d) %d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d \n", id,
                                v[31],v[30],v[29],v[28],v[27],v[26],v[25],v[24],v[23],v[22],
                                v[21],v[20],v[19],v[18],v[17],v[16],v[15],v[14],v[13],v[12],
                                v[11],v[10],v[9],v[8],v[7],v[6],v[5],v[4],v[3],v[2],v[1],v[0]);
                fflush (stdout);
                return 1;
        } 
		else
		{
                return 0;
        }
}

