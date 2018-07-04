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
/*變數宣告：                                             */
/*  bmpHeader    ： BMP檔的標頭                          */
/*  bmpInfo      ： BMP檔的資訊                          */
/*  **BMPSaveData： 儲存要被寫入的像素資料               */
/*  **BMPData    ： 暫時儲存要被寫入的像素資料           */
/*********************************************************/
BMPHEADER bmpHeader;                        
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;                                               
RGBTRIPLE **BMPData = NULL;                                                

/*********************************************************/
/*函數宣告：                                             */
/*  readBMP    ： 讀取圖檔，並把像素資料儲存在BMPSaveData*/
/*  saveBMP    ： 寫入圖檔，並把像素資料BMPSaveData寫入  */
/*  swap       ： 交換二個指標                           */
/*  **alloc_memory： 動態分配一個Y * X矩陣               */
/*********************************************************/
int readBMP( char *fileName);        
int saveBMP( char *fileName);        
void swap(RGBTRIPLE *a, RGBTRIPLE *b);
RGBTRIPLE **alloc_memory( int Y, int X );        

int main(int argc,char *argv[])
{
/*********************************************************/
/*變數宣告：                                             */
/*  *infileName  ： 讀取檔名                             */
/*  *outfileName ： 寫入檔名                             */
/*  startwtime   ： 記錄開始時間                         */
/*  endwtime     ： 記錄結束時間                         */
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
	//建立輸入檔案物件	
        ifstream bmpFile( fileName, ios::in | ios::binary );
 
        //檔案無法開啟
        if ( !bmpFile ){
                cout << "It can't open file!!" << endl;
                return 0;
        }
 
        //讀取BMP圖檔的標頭資料
    	bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );
 
        //判決是否為BMP圖檔
        if( bmpHeader.bfType != 0x4d42 ){
                cout << "This file is not .BMP!!" << endl ;
                return 0;
        }
 
        //讀取BMP的資訊
        bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );
        
        //判斷位元深度是否為24 bits
        if ( bmpInfo.biBitCount != 24 ){
                cout << "The file is not 24 bits!!" << endl;
                return 0;
        }

        //修正圖片的寬度為4的倍數
        while( bmpInfo.biWidth % 4 != 0 )
        	bmpInfo.biWidth++;

        //動態分配記憶體
        BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
        
        //讀取像素資料
    	//for(int i = 0; i < bmpInfo.biHeight; i++)
        //	bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));
	bmpFile.read( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);
	
        //關閉檔案
        bmpFile.close();
 
        return 1;
 
}

int saveBMP( char *fileName)
{
 	//判決是否為BMP圖檔
        if( bmpHeader.bfType != 0x4d42 ){
                cout << "This file is not .BMP!!" << endl ;
                return 0;
        }
        
 	//建立輸出檔案物件
        ofstream newFile( fileName,  ios:: out | ios::binary );
 
        //檔案無法建立
        if ( !newFile ){
                cout << "The File can't create!!" << endl;
                return 0;
        }
 	
        //寫入BMP圖檔的標頭資料
        newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

	//寫入BMP的資訊
        newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

        //寫入像素資料
        //for( int i = 0; i < bmpInfo.biHeight; i++ )
        //        newFile.write( ( char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE) );
        newFile.write( ( char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

        //寫入檔案
        newFile.close();
 
        return 1;
 
}



RGBTRIPLE **alloc_memory(int Y, int X )
{        
	//建立長度為Y的指標陣列
        RGBTRIPLE **temp = new RGBTRIPLE *[ Y ];
	RGBTRIPLE *temp2 = new RGBTRIPLE [ Y * X ];
        memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
        memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );

	//對每個指標陣列裡的指標宣告一個長度為X的陣列 
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
