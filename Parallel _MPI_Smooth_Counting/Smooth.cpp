#include <mpi.h>
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"

using namespace std;


#define NSmooth 1000

/*********************************************************/
/*�ܼƫŧi�G                                             */
/*  bmpHeader    �G BMP�ɪ����Y                          */
/*  bmpInfo      �G BMP�ɪ���T                          */
/*  **BMPSaveData�G �x�s�n�Q�g�J���������               */
/*  **BMPData    �G �Ȯ��x�s�n�Q�g�J���������           */
/*********************************************************/
BMPHEADER bmpHeader;                        
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;                                               
RGBTRIPLE **BMPData = NULL;                                                

/*********************************************************/
/*��ƫŧi�G                                             */
/*  readBMP    �G Ū�����ɡA�ç⹳������x�s�bBMPSaveData*/
/*  saveBMP    �G �g�J���ɡA�ç⹳�����BMPSaveData�g�J  */
/*  swap       �G �洫�G�ӫ���                           */
/*  **alloc_memory�G �ʺA���t�@��Y * X�x�}               */
/*********************************************************/
int readBMP( char *fileName);        
int saveBMP( char *fileName);        
void swap(RGBTRIPLE *a, RGBTRIPLE *b);
RGBTRIPLE **alloc_memory( int Y, int X );        

int main(int argc,char *argv[])
{
/*********************************************************/
/*�ܼƫŧi�G                                             */
/*  *infileName  �G Ū���ɦW                             */
/*  *outfileName �G �g�J�ɦW                             */
/*  startwtime   �G �O���}�l�ɶ�                         */
/*  endwtime     �G �O�������ɶ�                         */
/*********************************************************/
	char *infileName = "input.bmp";
     	char *outfileName = "output2.bmp";
	double startwtime = 0.0, endwtime=0;

	int comm_sz=0,id=0;
	int *scnt; 
	int *displs;
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&comm_sz);		
	MPI_Comm_rank(MPI_COMM_WORLD,&id);			


	startwtime = MPI_Wtime();

	
	if(id == 0)
	{	
        	if ( readBMP( infileName) )
                	cout << "Read file successfully!!" << endl;
        	else 
                	cout << "Read file fails!!" << endl;
		
	}

	MPI_Bcast(&bmpInfo.biHeight,1,MPI_INT,0,MPI_COMM_WORLD);
	MPI_Bcast(&bmpInfo.biWidth,1,MPI_INT,0,MPI_COMM_WORLD);	
	if(id != 0)
	{
		BMPSaveData = alloc_memory(bmpInfo.biHeight,bmpInfo.biWidth);
	}
	BMPData = alloc_memory( bmpInfo.biHeight/comm_sz+2, bmpInfo.biWidth);//allocate pic size and 2buffer

	scnt = (int*)malloc(sizeof(int)*comm_sz);
	displs = (int*)malloc(sizeof(int)*comm_sz);		
	
	for(int i = 0; i < comm_sz ; i++){
		scnt[i] = bmpInfo.biHeight/comm_sz;						//for scatterv size
		displs[i] = (bmpInfo.biHeight/comm_sz)*i;				//for scatterv size and to locate his position
	}
	
	MPI_Datatype RGB;
	MPI_Type_contiguous(bmpInfo.biWidth*sizeof(RGBTRIPLE),MPI_CHAR,&RGB);
	MPI_Type_commit(&RGB);



	MPI_Scatterv((char*)BMPSaveData[0],scnt,displs,RGB,(char*)BMPData[1],bmpInfo.biHeight/comm_sz,RGB,0,MPI_COMM_WORLD);//pass pic to all process and divide to differnt part 
	
	
	if(id % 2 == 0)								//to fill up bottom by exchanging buffer[1]
	{
		if(id != 0)
			MPI_Send((char*)BMPData[1],1,RGB,id-1,0,MPI_COMM_WORLD);	
		else
			MPI_Send((char*)BMPData[1],1,RGB,comm_sz-1,0,MPI_COMM_WORLD);	
	
		MPI_Send((char*)BMPData[bmpInfo.biHeight/comm_sz],1,RGB,id+1,0,MPI_COMM_WORLD);	//to fill up top buffer by exchanging buffer[botton-1]
	}
	else
	{
		if(id != comm_sz-1)					//separate two part and do the opposite thing against even
			MPI_Recv((char*)BMPData[bmpInfo.biHeight/comm_sz+1],1,RGB,id+1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		else	
			MPI_Recv((char*)BMPData[bmpInfo.biHeight/comm_sz+1],1,RGB,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
	
		MPI_Recv((char*)BMPData[0],1,RGB,id-1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);	
	}

	if(id % 2 == 0)
	{
		if(id != 0)						//recv and fill up top buff
			MPI_Recv((char*)BMPData[0],1,RGB,id-1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		else	
			MPI_Recv((char*)BMPData[0],1,RGB,comm_sz-1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

		MPI_Recv((char*)BMPData[bmpInfo.biHeight/comm_sz+1],1,RGB,id+1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);//recv and fill up bottom buff
	}
	else
	{
		if(id != comm_sz-1)
			MPI_Send((char*)BMPData[bmpInfo.biHeight/comm_sz],1,RGB,id+1,0,MPI_COMM_WORLD);	
		else
			MPI_Send((char*)BMPData[bmpInfo.biHeight/comm_sz],1,RGB,0,0,MPI_COMM_WORLD);	
		
		MPI_Send((char*)BMPData[1],1,RGB,id-1,0,MPI_COMM_WORLD);	
	}

	
	for(int count = 0;count < NSmooth; count ++)
	{

		for(int i = 1 ; i <= bmpInfo.biHeight/comm_sz;i++)
		{
			for(int j = 0; j < bmpInfo.biWidth;j++)
			{
				
				int Left = j>0?j-1:bmpInfo.biWidth-1;
				int Right = j<bmpInfo.biWidth-1?j+1:0;		//smooth
				
				BMPSaveData[i][j].rgbBlue =  (double) (BMPData[i][j].rgbBlue+BMPData[i-1][j].rgbBlue+BMPData[i+1][j].rgbBlue+BMPData[i][Left].rgbBlue+BMPData[i][Right].rgbBlue)/5+0.5;
				BMPSaveData[i][j].rgbGreen =  (double) (BMPData[i][j].rgbGreen+BMPData[i-1][j].rgbGreen+BMPData[i+1][j].rgbGreen+BMPData[i][Left].rgbGreen+BMPData[i][Right].rgbGreen)/5+0.5;
				BMPSaveData[i][j].rgbRed =  (double) (BMPData[i][j].rgbRed+BMPData[i-1][j].rgbRed+BMPData[i+1][j].rgbRed+BMPData[i][Left].rgbRed+BMPData[i][Right].rgbRed)/5+0.5;
				
			}
		}
		
		swap(BMPSaveData,BMPData);				//swap temp buffer and exactly buffer

		if(id % 2 == 0)
		{
			if(id != 0)							//do the same as before to exchange buffer
				MPI_Send((char*)BMPData[1],1,RGB,id-1,0,MPI_COMM_WORLD);	
			else
				MPI_Send((char*)BMPData[1],1,RGB,comm_sz-1,0,MPI_COMM_WORLD);	
	
			MPI_Send((char*)BMPData[bmpInfo.biHeight/comm_sz],1,RGB,id+1,0,MPI_COMM_WORLD);
		}
		else
		
			if(id != comm_sz-1)
				MPI_Recv((char*)BMPData[bmpInfo.biHeight/comm_sz+1],1,RGB,id+1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
			else	
				MPI_Recv((char*)BMPData[bmpInfo.biHeight/comm_sz+1],1,RGB,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
	
			MPI_Recv((char*)BMPData[0],1,RGB,id-1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);	
		}

		if(id % 2 == 0)
		{
			if(id != 0)
				MPI_Recv((char*)BMPData[0],1,RGB,id-1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
			else	
				MPI_Recv((char*)BMPData[0],1,RGB,comm_sz-1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

			MPI_Recv((char*)BMPData[bmpInfo.biHeight/comm_sz+1],1,RGB,id+1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		}
		else
		{
			if(id != comm_sz-1)
				MPI_Send((char*)BMPData[bmpInfo.biHeight/comm_sz],1,RGB,id+1,0,MPI_COMM_WORLD);	
			else
				MPI_Send((char*)BMPData[bmpInfo.biHeight/comm_sz],1,RGB,0,0,MPI_COMM_WORLD);	
		
			MPI_Send((char*)BMPData[1],1,RGB,id-1,0,MPI_COMM_WORLD);	
		}
	
	}
	
	MPI_Gatherv((char*)BMPSaveData[0],bmpInfo.biHeight/comm_sz,RGB,(char*)BMPSaveData[0],scnt,displs,RGB,0,MPI_COMM_WORLD);		//gather back to exactly buffer		

	
	
	if(id == 0)
	{
        	if ( saveBMP( outfileName ) )
                	cout << "Save file successfully!!" << endl;
        	else
               		cout << "Save file fails!!" << endl;
	}
        endwtime = MPI_Wtime();			//time is up
    	cout << "The execution time = "<< endwtime-startwtime <<endl ;
    	
 	free(BMPData);
 	free(BMPSaveData);
 	free(scnt);
 	free(displs);				//free memory
 	MPI_Finalize();

        return 0;
}


int readBMP(char *fileName)
{
	//�إ߿�J�ɮת���	
        ifstream bmpFile( fileName, ios::in | ios::binary );
 
        //�ɮ׵L�k�}��
        if ( !bmpFile ){
                cout << "It can't open file!!" << endl;
                return 0;
        }
 
        //Ū��BMP���ɪ����Y���
    	bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );
 
        //�P�M�O�_��BMP����
        if( bmpHeader.bfType != 0x4d42 ){
                cout << "This file is not .BMP!!" << endl ;
                return 0;
        }
 
        //Ū��BMP����T
        bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );
        
        //�P�_�줸�`�׬O�_��24 bits
        if ( bmpInfo.biBitCount != 24 ){
                cout << "The file is not 24 bits!!" << endl;
                return 0;
        }

        //�ץ��Ϥ����e�׬�4������
        while( bmpInfo.biWidth % 4 != 0 )
        	bmpInfo.biWidth++;

        //�ʺA���t�O����
        BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
        
        //Ū���������
    	//for(int i = 0; i < bmpInfo.biHeight; i++)
        //	bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));
	bmpFile.read( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);
	
        //�����ɮ�
        bmpFile.close();
 
        return 1;
 
}

int saveBMP( char *fileName)
{
 	//�P�M�O�_��BMP����
        if( bmpHeader.bfType != 0x4d42 ){
                cout << "This file is not .BMP!!" << endl ;
                return 0;
        }
        
 	//�إ߿�X�ɮת���
        ofstream newFile( fileName,  ios:: out | ios::binary );
 
        //�ɮ׵L�k�إ�
        if ( !newFile ){
                cout << "The File can't create!!" << endl;
                return 0;
        }
 	
        //�g�JBMP���ɪ����Y���
        newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

	//�g�JBMP����T
        newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

        //�g�J�������
        //for( int i = 0; i < bmpInfo.biHeight; i++ )
        //        newFile.write( ( char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE) );
        newFile.write( ( char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

        //�g�J�ɮ�
        newFile.close();
 
        return 1;
 
}



RGBTRIPLE **alloc_memory(int Y, int X )
{        
	//�إߪ��׬�Y�����а}�C
        RGBTRIPLE **temp = new RGBTRIPLE *[ Y ];
	RGBTRIPLE *temp2 = new RGBTRIPLE [ Y * X ];
        memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
        memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );

	//��C�ӫ��а}�C�̪����Ыŧi�@�Ӫ��׬�X���}�C 
        for( int i = 0; i < Y; i++){
                temp[ i ] = &temp2[i*X];
        }
 
        return temp;
 
}

void swap(RGBTRIPLE *a, RGBTRIPLE *b)
{
	RGBTRIPLE *temp;
	temp = a;
	a = b;
	b = temp;
}
