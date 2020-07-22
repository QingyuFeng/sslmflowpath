/*  D8HDistToStrm

  This function computes the distance from each grid cell moving downstream until a stream 
  grid cell as defined by the Stream Raster grid is encountered.  The optional threshold 
  input is to specify a threshold to be applied to the Stream Raster grid (src).  
  Stream grid cells are defined as having src value >= the threshold, or >=1 if a 
  threshold is not specified.

  David Tarboton
  Utah State University  
  May 23, 2010 
  
*/

/*  Copyright (C) 2010  David Tarboton, Utah State University

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

using namespace std;



//returns true iff cell at [nrow][ncol] points to cell at [row][col]
bool pointsToMe(long col, long row, long ncol, long nrow, tdpartition *dirData) {
	short d;
	if (!dirData->hasAccess(ncol, nrow) || dirData->isNodata(ncol, nrow)) { return false; }
	d = dirData->getData(ncol, nrow, d);
	if (nrow + d2[d] == row && ncol + d1[d] == col) {
		return true;
	}
	return false;
}


int distgrid(char *pfile, char *srcfile, char *wsfile, char *distfile, int thresh)
{
MPI_Init(NULL,NULL);
{  //  All code within braces so that objects go out of context and destruct before MPI is closed
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("D8HDistToSubOlt version %s\n",TDVERSION);
	int i,j,in,jn;
	float tempFloat; double tempdxc,tempdyc;
	short tempShort,k;
	int32_t tempLong;
	bool finished;

 //  Begin timer
    double begint = MPI_Wtime();

	//Read Flow Direction header using tiffIO
	tiffIO pf(pfile,SHORT_TYPE);
	long totalX = pf.getTotalX();
	long totalY = pf.getTotalY();
	double dxA = pf.getdxA();
	double dyA = pf.getdyA();

	if(rank==0)
		{
			float timeestimate=(1.2e-6*totalX*totalY/pow((double) size,0.65))/60+1;  // Time estimate in minutes
			fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
			fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
			fflush(stderr);
		}

	//Read flow direction data into partition
	tdpartition *flowDir;
	flowDir = CreateNewPartition(pf.getDatatype(), totalX, totalY, dxA, dyA, pf.getNodata());
	int nx = flowDir->getnx();
	int ny = flowDir->getny();
	int xstart, ystart;
	flowDir->localToGlobal(0, 0, xstart, ystart);
	flowDir->savedxdyc(pf);
	pf.read(xstart, ystart, ny, nx, flowDir->getGridPointer());

 	//Read src file
	tdpartition *src;
	tiffIO srcf(srcfile,LONG_TYPE);
	if(!pf.compareTiff(srcf)) {
		printf("File sizes do not match\n%s\n",srcfile);
		fflush(stdout);
		MPI_Abort(MCW,5);
		return 1;  //And maybe an unhappy error message
	}
	src = CreateNewPartition(srcf.getDatatype(), totalX, totalY, dxA, dyA, srcf.getNodata());
	srcf.read(xstart, ystart, ny, nx, src->getGridPointer());

	// Added by Qingyu Feng to get the watersehed and subarea boundary: start
	// Read watershed bourndary ws file.
	tdpartition *ws;
	tiffIO wsf(wsfile, SHORT_TYPE);
	if (!pf.compareTiff(wsf)) {
		printf("File sizes do not match\n%s\n", wsfile);
		fflush(stdout);
		MPI_Abort(MCW, 5);
		return 1;  //And maybe an unhappy error message
	}
	ws = CreateNewPartition(wsf.getDatatype(), totalX, totalY, dxA, dyA, wsf.getNodata());
	wsf.read(xstart, ystart, ny, nx, ws->getGridPointer());
	// Added by Qingyu Feng to get the watersehed and subarea boundary: end


	//Record time reading files
	double readt = MPI_Wtime();
   
 	//Create empty partition to store distance information
	tdpartition *fdarr;
	fdarr = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dxA, dyA, MISSINGFLOAT);

	/*  Calculate Distances  */
	//float dist[9];
	float** dist = new float*[ny];
	for (int j = 0; j < ny; j++)
	{
		dist[j] = new float[9];
	}
	// Commented by Qingyu Feng to understand the code
	// The structure of dist is a rowno*9 array.
	// Then for each row, get the horizental and vertical resolution,
	// For each direction kk, calculate the distance for each flow direction.
	// End of comment
    for (int m=0;m<ny;m++){
		flowDir->getdxdyc(m,tempdxc,tempdyc);
		for(int kk=1; kk<=8; kk++)
		{
			 dist[m][kk]=sqrt(d1[kk]*d1[kk]*tempdxc*tempdxc+d2[kk]*d2[kk]*tempdyc*tempdyc);
		}
	}

	// Added by Qingyu Feng to calculate the distance from stream cell to subarea outlet.
	//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
	//  Block to evaluate distance to outlet
	//Create empty partition to store number of contributing neighbors
	tdpartition *contribs;
	contribs = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, MISSINGSHORT);

	flowDir->share();
	src->share();

	//  Initialize queue and contribs partition	
	queue <node> que;
	node t;
	int p;
	long inext, jnext;
	for (j = 0; j < ny; ++j) {
		for (i = 0; i < nx; ++i) {
			// If I am on stream and my downslope neighbor is on stream contribs is 1
			// If I am on stream and my downslope neighbor is off stream contribs is 0 because 
			//  I am at the end of a stream
			if (!src->isNodata(i, j) && 
				src->getData(i, j, tempLong) > 0 &&
				!flowDir->isNodata(i, j))
			{
				p = flowDir->getData(i, j, tempShort);
				inext = i + d1[p];
				jnext = j + d2[p];
				if (!src->isNodata(inext, jnext) && 
					src->getData(inext, jnext, tempLong) > 0 &&
					!flowDir->isNodata(inext, jnext))
					contribs->setData(i, j, (short)1);//addData? What if it is a junction and has more floing into it?
				else
				{
					contribs->setData(i, j, (short)0);
					t.x = i;
					t.y = j;
					que.push(t);
				}
			}
		}
	}

	// while loop where each process empties its que, then shares border info, and repeats till everyone is done
	finished = false;
	int m;
	int wsid, wsidnext;
	while (!finished) {
		contribs->clearBorders();
		while (!que.empty()) {
			t = que.front();
			i = t.x;
			j = t.y;
			que.pop();
			p = flowDir->getData(i, j, tempShort);
			flowDir->getdxdyc(j, tempdxc, tempdyc);
			float llength = 0.;
			inext = i + d1[p];
			jnext = j + d2[p];

			// Start from the watershed outlet, and trace upwards.
			// If the next is no data, do not change
			// If not no data, calculate the distance.
			// In order to determine which subarea it is now, make a mark
			// and get the ws for the current and next.
			wsid = ws->getData(i, j, tempShort);
			wsidnext = ws->getData(inext, jnext, tempShort);
			//printf("wsid row: %d, col: %d, wsid: %d, wsidnext: %d\n", i, j, wsid, wsidnext);

			if (!fdarr->isNodata(inext, jnext))
			{
				// If the next stream is in the same watershed, accumulate the llength
				if (wsid == wsidnext)
				{
					llength += fdarr->getData(inext, jnext, tempFloat);

					if (p == 1 || p == 5)llength = llength + tempdxc; // make sure that it is correct
					if (p == 3 || p == 7)llength = llength + tempdyc;
					if (p % 2 == 0)llength = llength + sqrt(tempdxc*tempdxc + tempdyc * tempdyc);
				}
				// If the next strem cell is in a different watershed, do not accumulate the length 
				else 
				{
					llength = 0.0;
				}
				
			}
			//printf("streamcell row: %d, col: %d, len: %f\n", i, j, llength);

			// Set the value of length to the length (fdarr) grid).
			fdarr->setData(i, j, llength);
			//printf("fdarr row: %d, col: %d, len: %f\n", i, j, fdarr->getData(i, j, tempFloat));
			//totalP++;
			//  Find if neighbor points to me and reduce its dependency by 1
			for (m = 1; m <= 8; ++m) {
				inext = i + d1[m];
				jnext = j + d2[m];
				if (pointsToMe(i, j, inext, jnext, flowDir) &&
					!src->isNodata(inext, jnext) &&
					src->getData(inext, jnext, tempLong) > 0)
				{
					contribs->addToData(inext, jnext, (short)(-1));
					if (contribs->isInPartition(inext, jnext) &&
						contribs->getData(inext, jnext, tempShort) == 0)
					{
						t.x = inext;
						t.y = jnext;
						que.push(t);
					}
				}
			}
		}
		//Pass information across partitions

		contribs->addBorders();
		fdarr->share();

		//If this created a cell with no contributing neighbors, put it on the queue
		for (i = 0; i < nx; i++) {
			if (contribs->getData(i, -1, tempShort) != 0 && contribs->getData(i, 0, tempShort) == 0)
			{
				t.x = i;
				t.y = 0;
				que.push(t);
			}
			if (contribs->getData(i, ny, tempShort) != 0 && contribs->getData(i, ny - 1, tempShort) == 0)
			{
				t.x = i;
				t.y = ny - 1;
				que.push(t);
			}
		}

		//Check if done
		finished = que.empty();
		finished = contribs->ringTerm(finished);
	}

	// Timer lengtht
	double lengthct = MPI_Wtime();

	//  Now length partition is evaluated
	// // Added by Qingyu Feng to calculate the distance from stream cell to subarea outlet: End



	//  Set neighbor partition to 1 because all grid cells drain to one other grid cell in D8
	tdpartition *neighbor;
	neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dxA, dyA, MISSINGSHORT);
    
	node temp;
	//queue <node> que;
	for(j=0; j<ny; j++) // loop over rows
		for(i=0; i<nx; i++) { // loop over columns
			if(!flowDir->isNodata(i,j)) {
				//Set contributing neighbors to 1 
				neighbor->setData(i,j,(short)1);
			}
			//If src is nodata and the value equal or larger than threshold (default is 1),
			// Set the neighbour to 0.
			// This means if it is a stream, the neighbour is 0, else it is 1.
			// Then, push the temp node to queue. 
			if(!src->isNodata(i,j) && src->getData(i,j,tempLong) >=thresh){
				neighbor->setData(i,j,(short)0);
				temp.x = i;
				temp.y = j;
				//printf("node: x: %d, y: %d\n", i, j);
				que.push(temp); // add temp node to the end of the que
				// This means that the que starts from the first encountered 
				// stream cell, and then till the last. All stream cells.
			}
	}

	//Share information and set borders to zero
	flowDir->share();
	src->share();
	fdarr->share();
	neighbor->clearBorders();

	finished = false;
	//Ring terminating while loop
	while(!finished) {
		while(!que.empty()){
			//Takes next node with no contributing neighbors
			temp = que.front();
			que.pop();
			i = temp.x;
			j = temp.y;
			//printf("node: x: %d, y: %d\n", i, j);
			//  FLOW ALGEBRA EXPRESSION EVALUATION
			//  If on stream, the value is set to 0.0
			//if(!src->isNodata(i,j) && src->getData(i,j,tempLong) >=thresh){
				//printf("stremcell: %d, col: %d, len: %f\n", i, j, fdarr->getData(i, j, tempFloat));
				//continue;
			//}
			//else
			if (!src->isNodata(i, j) && src->getData(i, j, tempLong) < thresh) 
			{
				flowDir->getData(i,j,k);  //  Get neighbor downstream
				// Here, key is the flow direction of this cell.
				// Then, find the downstream cell pointed by this 
				// flow direction.
				in = i+d1[k];
				jn = j+d2[k];
				// If the next is no data, set distance to no data.
				if (fdarr->isNodata(in, jn))
				{
					fdarr->setToNodata(i, j);
				}
				// Else, calculate the distance. This starts from stream, so
				// if the next is on stream, it is 0.0. Then, it move upstreamwards
				// and add the current distance to stream by the distance dictionary.
				else
				{
					//printf("next: %d, col: %d, len: %f\n", i, j, fdarr->getData(in, jn, tempFloat));
					fdarr->setData(i, j, (float)(dist[j][k] + fdarr->getData(in, jn, tempFloat)));
					//printf("after: %d, col: %d, len: %f\n", i, j, fdarr->getData(i, j, tempFloat));
					
				}
				
				
			}
			
			//printf("next: x: %d, y: %d, dist: %f\n", in, jn, (float)(dist[j][k] + fdarr->getData(in, jn, tempFloat)));
			// For each stream cell, find its upslope cells.
			//  Now find upslope cells and reduce dependencies
			for(k=1; k<=8; k++) 
			{
				in = i+d1[k];
				jn = j+d2[k];
			//test if neighbor drains towards cell excluding boundaries 
				
				if(!flowDir->isNodata(in,jn))
				{
					//printf("next: x: %d, y: %d\n", in, jn, k);
					flowDir->getData(in,jn,tempShort);
					//printf("next: x: %d, y: %d, k: %d, temps: %d\n", in, jn, k, tempShort);
					// For 8 neighbours of this target cell, if the flowdirection
					// of the neighbour flows to this cell based on k and p of neighbour.
					// If the difference between k and p is 4, the neighbour flows to this
					// target cell.
					// If neighbbour is a non-stream cell, add it to queue.
					if(tempShort-k == 4 || tempShort-k == -4)
					{
			//Decrement the number of contributing neighbors in neighbor
						neighbor->addToData(in,jn,(short)-1);
			//Check if neighbor needs to be added to que
						if(flowDir->isInPartition(in,jn) &&
							neighbor->getData(in, jn, tempShort) == 0 )
						{
							temp.x=in;
							temp.y=jn;
							//printf("temp: x: %d, y: %d\n", in, jn);
							que.push(temp);
						}
					}

				}
			}
		}
		//  Here the queue is empty
		//Pass information
		fdarr->share();
		neighbor->addBorders();

		//If this created a cell with no contributing neighbors, put it on the queue
		for(i=0; i<nx; i++){
			if(neighbor->getData(i, -1, tempShort)!=0 && neighbor->getData(i, 0, tempShort)==0){
				temp.x = i;
				temp.y = 0;
				que.push(temp);
			}
			if(neighbor->getData(i, ny, tempShort)!=0 && neighbor->getData(i, ny-1, tempShort)==0){
				temp.x = i;
				temp.y = ny-1;
				que.push(temp); 
			}
		}
		//Clear out borders
		neighbor->clearBorders();
	
		//Check if done
		finished = que.empty();
		finished = fdarr->ringTerm(finished);
	}
	//Stop timer
	double computet = MPI_Wtime();

	//Create and write TIFF file
	float aNodata = MISSINGFLOAT;
	tiffIO a(distfile, FLOAT_TYPE, aNodata, pf);
	a.write(xstart, ystart, ny, nx, fdarr->getGridPointer());
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

