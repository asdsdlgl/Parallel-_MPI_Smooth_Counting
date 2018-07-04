#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void sort(int a,int *list){
	int i,j,temp;
	for(i = 0; i < a; i++)
	{
		for(j = i+1;j < a; j++)
		{
			if(list[i]>list[j])				//sort by swap
			{
				temp = list[i];
				list[i] = list[j];
				list[j] = temp;
			}
		}
	}
	return;
}


int main(int argc, char *argv[]){

	int i,j;
	int *RC;
	int *displs;
	int id,comm_sz,phase,partner;
	int total;			
	int n,count;
	int *printlist;
	int *locallist;
	int *numlist;
	int *globallist;

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&comm_sz);	
	MPI_Comm_rank(MPI_COMM_WORLD,&id);	

	srand(time(NULL)+id);
	if(id == 0){
		printf("Press number n:");
		scanf("%d",&n);
		globallist = (int*)calloc(n,sizeof(int));		//allocate mem to process0 of list
	}
	MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);			//share n from process0 to all process

	RC = (int*)malloc(sizeof(int)*comm_sz);
	displs = (int*)malloc(sizeof(int)*comm_sz);


	if(n < comm_sz)
	{

		if(id == 0)
		{
			locallist = (int*)malloc(sizeof(int)*n);
			for(i = 0; i < n; i++)
				locallist[i] = rand()%100;
			sort(n,locallist);					//if n less than comm_sz then do sort in process0
			for(i=0;i<n;i++)
				printf("%d ",locallist[i]);
			printf("\n");
			free(locallist);
			free(RC);
			free(displs);
			free(globallist);
		}	
		MPI_Finalize();
		return 0;

	}else{
		count = n/comm_sz;
		j = n%comm_sz;
		for(i = 0; i < j; i++)
			if(id == i)count++;				//if n> comm_sz then share to all process to do the following sort
		locallist = (int*)malloc(sizeof(int)*count);
		for(i = 0; i < count; i++)
			locallist[i] = rand()%100;
		sort(count,locallist);				//local sort respectively
	}


	for(phase = 0; phase < comm_sz; phase++)
	{
		if(phase%2 == 0) {						//find partner
			if(id%2 != 0)
			
				partner = id-1;
			else
				partner = id+1;
		}
		else
		 {
			if(id%2 !=0)
				partner = id+1;
			else
				partner = id-1;
		}
		if(partner == -1 || partner == comm_sz)
			partner = MPI_PROC_NULL;

		if(partner != MPI_PROC_NULL)
		{
			if(id%2 == 0)
				MPI_Send(&count,1,MPI_INT,partner,0,MPI_COMM_WORLD);		//send to the partner the number set it is
			else
				MPI_Recv(&total,1,MPI_INT,partner,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);


			if(id%2 == 0)
				MPI_Recv(&total,1,MPI_INT,partner,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
			else
				MPI_Send(&count,1,MPI_INT,partner,0,MPI_COMM_WORLD);


			total += count;				//to count the many number sets should do sort 
			numlist = calloc(total,sizeof(int));	//numlist should be the size of 2 process between myself size and partner's

			if(id%2 == 0)
				MPI_Send(locallist,count,MPI_INT,partner,0,MPI_COMM_WORLD);
			else
				MPI_Recv(numlist,total-count,MPI_INT,partner,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);


			if(id%2 == 0)
				MPI_Recv(numlist,total-count,MPI_INT,partner,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);		//fill up  The first Half of list
			else
				MPI_Send(locallist,count,MPI_INT,partner,0,MPI_COMM_WORLD);


			for(i = 0;i<count;i++)
				numlist[total-count+i] = locallist[i];	//fill up the last half of list

			sort(total,numlist);		//sort

			if(id<partner){
				for(i = 0; i<count; i++)
					locallist[i] = numlist[i];	//if id is the less one to get the first half of sorted list
			}
			else{
				for(i = 0; i<count; i++)
					locallist[i] = numlist[total-count+i]; //if id is the less one to get the last half of sorted list
			}
		}

		for(i = 0; i<comm_sz; i++)
			RC[i] = n/comm_sz;
		if(n%comm_sz!=0)
			for(i = 0; i<n%comm_sz; i++)
				RC[i] += 1;						//to find the number set of process respectively
		displs[0] = 0;
		for(i = 1; i<comm_sz; i++)
			displs[i] = displs[i-1] + RC[i-1];		//to find where to start its position

		MPI_Gatherv(locallist,RC[id],MPI_INT,globallist,RC,displs,MPI_INT,0,MPI_COMM_WORLD);

		j = 1;
		if(id == 0)
		{
			printf("after phase%d\n",phase);
			printf("process 0: ");
			for(i = 0 ; i<n; i++)
			{
				printf("%d ",globallist[i]);
				if(i==displs[j]-1)
				{
					if(j==comm_sz)continue;			//list every phase of list
					printf("\nprocess %d: ",j);
					j++;
				}
			}
			printf("\n");		
		}
	}
	if(id==0)
	{
		printf("=============================\n");
		printf("Sorted by odd-even sort:\n");
		for(i = 0;i<n;i++)
		{
			printf("%d ",globallist[i]);		//list the final list
			if(i%20==0&&i!=0)printf("\n");
		}
		printf("\n");
		free(globallist);
	}
	free(RC);
	free(displs);
	free(locallist);			//free all of  mem  of list
	free(numlist);

	MPI_Finalize();
	return 0;
}
