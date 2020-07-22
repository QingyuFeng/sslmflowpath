/*  subindexmap

  The mani program to create a map of sslmindex for subarea.

  Qingyu Feng
  RCEES
  June 29, 2020

*/

/*  Copyright (C) 2020  Qingyu Feng, RCEES

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 2, 1991 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the full GNU General Public License is included in file
gpl.html. This is also available at:
http://www.gnu.org/copyleft/gpl.html
or from:
The Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA  02111-1307, USA.

If you wish to use or incorporate this program (or parts of it) into
other software that does not meet the GNU General Public License
conditions contact the author to request permission.
Qingyu Feng
email:  qyfeng18@rcees.ac.cn
*/

#include <mpi.h>
#include <math.h>
#include <queue>
#include "commonLib.h"
#include "linearpart.h"
#include "createpart.h"
#include "tiffIO.h"
#include <algorithm>
#include <string.h>
#include <vector>
#include <iostream>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h" 
#include "rapidjson/filereadstream.h"

#include <cstdio>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>

#include "idxhash.h"

using namespace std;
using namespace rapidjson;



bool compare_float(float x, float y, float epsilon = 0.001f) {
	if (fabs(x - y) < epsilon)
		return true; //they are same
	return false; //they are not same
}

// This function reclass the values into 1 to 0 based on 
// the index values.
// mapserver has no datavalue as 0, so, we can not use it.
// Besides, it require 8 bit map, which will be converted later
// using gdal
int ssidxValue2Class(float sslmval) {
	
	int recalval = 0;
	if (sslmval <= 0.1) { recalval = 1; }
	else if ((sslmval > 0.1) && (sslmval <= 0.2)) { recalval = 2; }
	else if ((sslmval > 0.2) && (sslmval <= 0.3)) { recalval = 3; }
	else if ((sslmval > 0.3) && (sslmval <= 0.4)) { recalval = 4; }
	else if ((sslmval > 0.4) && (sslmval <= 0.5)) { recalval = 5; }
	else if ((sslmval > 0.5) && (sslmval <= 0.6)) { recalval = 6; }
	else if ((sslmval > 0.6) && (sslmval <= 0.7)) { recalval = 7; }
	else if ((sslmval > 0.7) && (sslmval <= 0.8)) { recalval = 8; }
	else if ((sslmval > 0.8) && (sslmval <= 0.9)) { recalval = 9; }
	else if ((sslmval > 0.9) && (sslmval <= 1.9)) { recalval = 10; }
	return recalval;
}





int subindexmap(char *wsfile,
	char *subidxjson,
	char *subidxmap
)
{

MPI_Init(NULL,NULL);{  
	//  All code within braces so that objects go out of context and destruct before MPI is closed
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("sslmfpsub version %s\n",TDVERSION);
	int i,j,in,jn;
	float tempFloat; 
	double tempdxc,tempdyc;
	short tempShort,k;
	int32_t tempLong;
	bool finished;

 //  Begin timer
    double begint = MPI_Wtime();

	//Read Flow Direction header using tiffIO
	tiffIO wsf(wsfile, LONG_TYPE);
	long totalX = wsf.getTotalX();
	long totalY = wsf.getTotalY();
	double dxA = wsf.getdxA();
	double dyA = wsf.getdyA();

	if(rank==0)
		{
			float timeestimate=(1.2e-6*totalX*totalY/pow((double) size,0.65))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}

	//Read flow direction data into partition
	tdpartition *ws;
	ws = CreateNewPartition(wsf.getDatatype(), totalX, totalY, dxA, dyA, wsf.getNodata());
	int nx = ws->getnx();
	int ny = ws->getny();
	int xstart, ystart;
	ws->localToGlobal(0, 0, xstart, ystart);
	ws->savedxdyc(wsf);
	wsf.read(xstart, ystart, ny, nx, ws->getGridPointer());

	// Get a vector of subIDs
	vector <long> subids;
	int subno;
	for (j = 0; j < ny; ++j) {
		for (i = 0; i < nx; ++i) {
			if (!ws->isNodata(i, j))
			{
				subno = ws->getData(i, j, tempLong);
				if (find(subids.begin(), subids.end(), subno) == subids.end())
				{
					subids.push_back(subno);
				}

			}
		}
	}


	// Readin the json contents
	Document indexSubJson;
	indexSubJson.SetObject();//Instantialize a doc object
	assert(indexSubJson.IsObject());

	FILE* fp = fopen(subidxjson, "rb");

	//char readBuffer[967979]; // Tested max value for char initition
	char readBuffer[1024*800];
	FileReadStream inpStream(fp, readBuffer, sizeof(readBuffer));

	indexSubJson.ParseStream(inpStream);
	fclose(fp);



	//Record time reading files
	double readt = MPI_Wtime();

	// Put subid into a hash table
	float hsSearchRlt;
	Value subIdxValObj;
	float subIdxVal;
	subno = 0;
	// Need the total number to determine the size of hashtable
	totalsubnos = subids.size();
	HashMapTable subIndexs;


	// Put the index value into the hastable
	for (auto & jsitr : indexSubJson.GetObject())
	{
		subno = stoi(jsitr.name.GetString());
		subIdxValObj = jsitr.value.GetObject();
		subIdxVal = stof(subIdxValObj["comb"].GetString());

		hsSearchRlt = subIndexs.SearchKey(subno);
		// If dont have the key, insert one
		if (compare_float(hsSearchRlt, -1.0))
		{
			//printf("Don't has the key: %d\n", subno);
			subIndexs.Insert(subno, subIdxVal);
		}
		//else { printf("Has the key: %d\n", subno); }

	}

	// Check the updated hash table
	// subIndexs.displayHash();

	// Then put the value into a new partition

	//Create empty partition to store distance information
	tdpartition *subindex;
	subindex = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, MISSINGSHORT);

	int subIdxValint;

	for (j = 0; j < ny; ++j) {
		for (i = 0; i < nx; ++i) {
			if (!ws->isNodata(i, j))
			{
				subno = ws->getData(i, j, tempLong);
				hsSearchRlt = subIndexs.SearchKey(subno);
				// If it has the value insert one.
				if (not compare_float(hsSearchRlt, -1.0))
				{
					// A multiplier of 100 was applied to make sure each 
					// value is int for display purpose.
					// Mapserver does not allow values larger than 127, since
					// it is a 8 bit. The value of index range from 0 to 100.
					// So, it will fit the requirement.
					subIdxVal = subIndexs.getValue(subno);
					//subIdxValint = (int)floor(subIdxVal * 100.0 + 0.5);
					//printf("IndexValue: %d\n", subIdxValint);
					subindex->setData(i, j, (short)ssidxValue2Class(subIdxVal));
				}
			}
		}
	}

	subindex->share();

	//Stop timer
	double computet = MPI_Wtime();

	//Create and write TIFF file
	int16_t aNodata = MISSINGSHORT;
	tiffIO a(subidxmap, SHORT_TYPE, aNodata, wsf);
	a.write(xstart, ystart, ny, nx, subindex->getGridPointer());

	double writet = MPI_Wtime();


        double dataRead, compute, write, total,tempd;
        dataRead = readt-begint;
        compute = computet-readt;
        write = writet-computet;
        total = writet - begint;

        MPI_Allreduce (&dataRead, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        dataRead = tempd/size;
        MPI_Allreduce (&compute, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        compute = tempd/size;
        MPI_Allreduce (&write, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        write = tempd/size;
        MPI_Allreduce (&total, &tempd, 1, MPI_DOUBLE, MPI_SUM, MCW);
        total = tempd/size;

        if( rank == 0)
                printf("Processors: %d\nRead time: %f\nCompute time: %f\nWrite time: %f\nTotal time: %f\n",
                  size, dataRead, compute, write,total);

	//Brackets force MPI-dependent objects to go out of scope before Finalize is called
	}MPI_Finalize();
return(0);
}

