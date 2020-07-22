/*  sslmfpws

  The mani program to calculate the lorenz curve for each subarea
  based on landuse, watershed boundary, elevation, and slope.
  on d8 flow model.

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

#include "lorenzfp.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h" // for stringify JSON
#include <cstdio>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>

using namespace std;
using namespace rapidjson;



int lorenzSub(char *pfile,
	char *distfile,
	char *wsfile,
	char *lufile,
	char *elevfile,
	char *slpfile,
	char *lzpvajs)
{

MPI_Init(NULL,NULL);{  
	//  All code within braces so that objects go out of context and destruct before MPI is closed
	int rank,size;
	MPI_Comm_rank(MCW,&rank);
	MPI_Comm_size(MCW,&size);
	if(rank==0)printf("sslmfpws version %s\n",TDVERSION);
	int i,j,in,jn;
	float tempFloat; 
	double tempdxc,tempdyc;
	short tempShort,k;
	int32_t tempLong;
	bool finished;

 //  Begin timer
    double begint = MPI_Wtime();

	//Read Flow Direction header using tiffIO
	tiffIO pf(pfile, LONG_TYPE);
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



 	//Read distfile file 
	tdpartition *distgrid;
	tiffIO distf(distfile,FLOAT_TYPE);
	if(!pf.compareTiff(distf)) {
		printf("File sizes do not match\n%s\n", distfile);
		fflush(stdout);
		MPI_Abort(MCW,5);
		return 1;  //And maybe an unhappy error message
	}
	distgrid = CreateNewPartition(distf.getDatatype(), totalX, totalY, dxA, dyA, distf.getNodata());
	distf.read(xstart, ystart, ny, nx, distgrid->getGridPointer());

	// Read watershed bourndary ws file.
	tdpartition *ws;
	tiffIO wsf(wsfile, LONG_TYPE);
	if (!pf.compareTiff(wsf)) {
		printf("File sizes do not match\n%s\n", wsfile);
		fflush(stdout);
		MPI_Abort(MCW, 5);
		return 1;  //And maybe an unhappy error message
	}
	ws = CreateNewPartition(wsf.getDatatype(), totalX, totalY, dxA, dyA, wsf.getNodata());
	wsf.read(xstart, ystart, ny, nx, ws->getGridPointer());

	// Read landuse lufile file.
	tdpartition *lugrid;
	tiffIO luf(lufile, LONG_TYPE);
	if (!pf.compareTiff(luf)) {
		printf("File sizes do not match\n%s\n", lufile);
		fflush(stdout);
		MPI_Abort(MCW, 5);
		return 1;  //And maybe an unhappy error message
	}
	lugrid = CreateNewPartition(luf.getDatatype(), totalX, totalY, dxA, dyA, luf.getNodata());
	luf.read(xstart, ystart, ny, nx, lugrid->getGridPointer());

	// Read elevation elevfile.
	tdpartition *elevgrid;
	tiffIO elevf(elevfile, FLOAT_TYPE);
	if (!pf.compareTiff(elevf)) {
		printf("File sizes do not match\n%s\n", elevfile);
		fflush(stdout);
		MPI_Abort(MCW, 5);
		return 1;  //And maybe an unhappy error message
	}
	elevgrid = CreateNewPartition(elevf.getDatatype(), totalX, totalY, dxA, dyA, elevf.getNodata());
	elevf.read(xstart, ystart, ny, nx, elevgrid->getGridPointer());

	// Read slope slp file.
	tdpartition *slpgrid;
	tiffIO slpf(slpfile, FLOAT_TYPE);
	if (!pf.compareTiff(slpf)) {
		printf("File sizes do not match\n%s\n", slpfile);
		fflush(stdout);
		MPI_Abort(MCW, 5);
		return 1;  //And maybe an unhappy error message
	}
	slpgrid = CreateNewPartition(slpf.getDatatype(), totalX, totalY, dxA, dyA, slpf.getNodata());
	slpf.read(xstart, ystart, ny, nx, slpgrid->getGridPointer());

	//Record time reading files
	double readt = MPI_Wtime();
   
	// Get unique subIDs
	vector <long> luids;

	int lunot;

	for (j = 0; j < ny; ++j) {
		for (i = 0; i < nx; ++i) {
			if (!ws->isNodata(i, j))
			{
				lunot = lugrid->getData(i, j, tempLong);
				if (find(luids.begin(), luids.end(), lunot) == luids.end())
				{
					luids.push_back(lunot);
				}

			}
		}
	}


	// Create subLuData
	// subLuData is a vector with size of maxsubid.
	// The value of this vector will be a hash table
	// storing the values for each lu in this subarea.
	totallunos = luids.size();
	HashMapTable *wsLuData = new HashMapTable;

	// Put data into the hastable
	int luno;
	float eleval, distval, slpval;
	int hsSearchRlt;

	for (j = 0; j < ny; ++j) {
		for (i = 0; i < nx; ++i) {
			if (!ws->isNodata(i, j))
			{
				// Value of hashtable is a LuData
				luno = lugrid->getData(i, j, tempLong);
				eleval = elevgrid->getData(i, j, tempFloat);
				distval = distgrid->getData(i, j, tempFloat);
				slpval = slpgrid->getData(i, j, tempFloat);

				// Get the x and y resolution
				flowDir->getdxdyc(j, tempdxc, tempdyc);

				hsSearchRlt = wsLuData->SearchKey(luno);

				if (hsSearchRlt == (int)-1)
				{
					// If the LU key not in the table, insert one
					Ludata *ludt = new Ludata;
					ludt->thisluno = luno;
					wsLuData->Insert(luno, ludt);
					
				}
				wsLuData->getLuData(luno)->elevarr.push_back(eleval);
				wsLuData->getLuData(luno)->distarr.push_back(distval);
				wsLuData->getLuData(luno)->slparr.push_back(slpval);
			}
		}
	}

	// See what we got for the structure
	// Print vector elements 
	//for (auto& subNo : subLuData) {
	//	subNo->displayHash();
	//}

	// Then, sort the vector data, and calculate percentage
	wsLuData->sortHashElevDistSlp();
	wsLuData->calPercElevDistSlp();
	wsLuData->countTotalCellinSub();
	wsLuData->removeVecDuplicates();
	wsLuData->calAccAreaLuElevDistSlp();
	//wsLuData->displayHash();


	//Stop timer
	double computet = MPI_Wtime();


	// Create and write output files file
	// There are two files to write:
	// lzpointfile
	// lzareafile
	// I will try to write them to json format, which 
	// is easier for php processing.
	// The rapidjson library was used
	// Reference https://rapidjson.org/

	// Creating the LzPoint JSON file.
	int jsSubNo, jsLuNo, len;
	int luTotalCell, subTotalCell;
	string tempLuComp;
	char buffer[50];

	// Structure for LZPoint JSON:
	// { tempLuNO: { tempElevKey: { tempValueKey: [], tempPointKey: []}}
	Document luESDJson;
	Document::AllocatorType& allocator = luESDJson.GetAllocator();
	luESDJson.SetObject();//Instantialize a doc object
	assert(luESDJson.IsObject());

	// Subarea as key Each Subarea has a key and a Object value

	for (auto &luid : luids)
	{
		Value tempLuNoKey;
		Value tempLuESD(kObjectType);

		hsSearchRlt = wsLuData->SearchKey(luid);
		//printf("LuTEst: %d\n", hsSearchRlt);
		if (hsSearchRlt != (int)-1)
		{
			jsLuNo = wsLuData->getLuData(luid)->thisluno;

			//////////////////////////////////////////////////////////////
			// Processing data for LZ Points 
			// For the first one, tempSubNo is Null.
			// For the second one, we need to check whether
			// these lu ESD value are for the same subarea as the first one.
			// Layer2: tempLuKey: {}
			if (tempLuNoKey.IsNull())
			{
				len = sprintf(buffer, "%d", jsLuNo);
				tempLuNoKey.SetString(buffer, len, allocator);
				memset(buffer, 0, sizeof(buffer));
			}
			else {
				len = sprintf(buffer, "%d", jsSubNo);
				tempLuComp = tempLuNoKey.GetString();
				// Modify the subarea no if the value are different
				if (tempLuComp != buffer) {
					tempLuNoKey.SetString(buffer, len, allocator);
				}
			}

			// Processing data from Elevation
			// Creat Object, Create key for object
			// Create array, create key for array
			// Values elevation
			Value tempElevObj(kObjectType);
			Value tempElevObjkey;
			tempElevObjkey.SetString("Elevation", allocator);

			Value tempValObjkeyElev;
			tempValObjkeyElev.SetString("Value", allocator);
			Value tempValObjArrElev(kArrayType);

			for (auto & itev : wsLuData->getLuData(luid)->elevarr)
			{
				Value tempElevVal;
				len = sprintf(buffer, "%f", itev);
				tempElevVal.SetString(buffer, len, allocator);
				memset(buffer, 0, sizeof(buffer));
				tempValObjArrElev.PushBack(tempElevVal, allocator);
			}
			tempElevObj.AddMember(tempValObjkeyElev, tempValObjArrElev, allocator);

			// Percentage elevation
			Value tempPerObjkeyElev;
			tempPerObjkeyElev.SetString("Percent", allocator);

			Value tempPerObjArrElev(kArrayType);

			for (auto & itev : wsLuData->getLuData(luid)->elevperarr)
			{
				Value tempElevPer;
				len = sprintf(buffer, "%f", itev);
				tempElevPer.SetString(buffer, len, allocator);
				memset(buffer, 0, sizeof(buffer));
				tempPerObjArrElev.PushBack(tempElevPer, allocator);
			}
			tempElevObj.AddMember(tempPerObjkeyElev, tempPerObjArrElev, allocator);


			// Processing data from Distance
			// Creat Object, Create key for object
			// Create array, create key for array
			Value tempDistObj(kObjectType);
			Value tempDistObjkey;
			tempDistObjkey.SetString("Dist2WSOlt", allocator);

			Value tempValObjkeyDist;
			tempValObjkeyDist.SetString("Value", allocator);
			Value tempValObjArrDist(kArrayType);

			for (auto & itev : wsLuData->getLuData(luid)->distarr)
			{
				Value tempDistVal;
				len = sprintf(buffer, "%f", itev);
				tempDistVal.SetString(buffer, len, allocator);
				memset(buffer, 0, sizeof(buffer));
				tempValObjArrDist.PushBack(tempDistVal, allocator);
			}
			tempDistObj.AddMember(tempValObjkeyDist, tempValObjArrDist, allocator);

			// Percentage Distance
			Value tempPerObjkeyDist;
			tempPerObjkeyDist.SetString("Percent", allocator);
			Value tempPerObjArrDist(kArrayType);

			for (auto & itev : wsLuData->getLuData(luid)->distperarr)
			{
				Value tempDistPer;
				len = sprintf(buffer, "%f", itev);
				tempDistPer.SetString(buffer, len, allocator);
				memset(buffer, 0, sizeof(buffer));
				tempPerObjArrDist.PushBack(tempDistPer, allocator);
			}
			tempDistObj.AddMember(tempPerObjkeyDist, tempPerObjArrDist, allocator);


			// Processing data from Slope
			// Creat Object, Create key for object
			// Create array, create key for array
			Value tempSlpObj(kObjectType);
			Value tempSlpObjkey;
			tempSlpObjkey.SetString("Slope", allocator);

			Value tempValObjkeySlp;
			tempValObjkeySlp.SetString("Value", allocator);
			Value tempValObjArrSlp(kArrayType);

			for (auto & itev : wsLuData->getLuData(luid)->slparr)
			{
				Value tempSlpVal;
				len = sprintf(buffer, "%f", itev);
				tempSlpVal.SetString(buffer, len, allocator);
				memset(buffer, 0, sizeof(buffer));
				tempValObjArrSlp.PushBack(tempSlpVal, allocator);
			}
			tempSlpObj.AddMember(tempValObjkeySlp, tempValObjArrSlp, allocator);

			// Percentage elevation
			Value tempPerObjkeySlp;
			tempPerObjkeySlp.SetString("Percent", allocator);
			Value tempPerObjArrSlp(kArrayType);

			for (auto & itev : wsLuData->getLuData(luid)->slpperarr)
			{
				Value tempSlpPer;
				len = sprintf(buffer, "%f", itev);
				tempSlpPer.SetString(buffer, len, allocator);
				memset(buffer, 0, sizeof(buffer));
				tempPerObjArrSlp.PushBack(tempSlpPer, allocator);
			}
			tempSlpObj.AddMember(tempPerObjkeySlp, tempPerObjArrSlp, allocator);

			// Add the Elev Val and Per member to the Elev obj
			tempLuESD.AddMember(tempElevObjkey, tempElevObj, allocator);
			tempLuESD.AddMember(tempDistObjkey, tempDistObj, allocator);
			tempLuESD.AddMember(tempSlpObjkey, tempSlpObj, allocator);


			//////////////////////////////////////////////////////////////
			// Processing data for LZ area 
			// {tempSubNo: { tempLuO: {elev: value, dist: value, slp: value, 
			//							luCellCount: value, luArea: value,
			//							luAreaPer: value}
			//				totalSubArea:{totalCellCount: value, totalArea: value}
			//				}

			Value elevAK;
			elevAK.SetString("lzAreaElevation", allocator);
			Value elevAV;
			len = sprintf(buffer, "%f", wsLuData->getLuData(luid)->elevarea);
			elevAV.SetString(buffer, len, allocator);
			memset(buffer, 0, sizeof(buffer));

			Value distAK;
			distAK.SetString("lzAreaDistance", allocator);
			Value distAV;
			len = sprintf(buffer, "%f", wsLuData->getLuData(luid)->distarea);
			distAV.SetString(buffer, len, allocator);
			memset(buffer, 0, sizeof(buffer));

			Value slopeAK;
			slopeAK.SetString("lzAreaSlope", allocator);
			Value slopeAV;
			len = sprintf(buffer, "%f", wsLuData->getLuData(luid)->slparea);
			slopeAV.SetString(buffer, len, allocator);
			memset(buffer, 0, sizeof(buffer));

			// Total cell count
			Value tcCK;
			tcCK.SetString("totalCell", allocator);
			Value tcCV;
			len = sprintf(buffer, "%d", wsLuData->getLuData(luid)->totallucells);
			tcCV.SetString(buffer, len, allocator);
			memset(buffer, 0, sizeof(buffer));

			Value tLuAK;
			tLuAK.SetString("totalLuArea", allocator);
			Value tLuAV;
			len = sprintf(buffer, "%f", float(wsLuData->getLuData(luid)->totallucells)*(float)tempdxc*(float)tempdyc / (float)10000.0);
			tLuAV.SetString(buffer, len, allocator);
			memset(buffer, 0, sizeof(buffer));

			Value tLuPK;
			tLuPK.SetString("totalLuAreaPer", allocator);
			Value tLuPV;
			len = sprintf(buffer, "%f", (float)wsLuData->getLuData(luid)->luareaper);
			tLuPV.SetString(buffer, len, allocator);
			memset(buffer, 0, sizeof(buffer));


			// A LuESDObj to store the key and value for elev, dist and slope.
			Value luESDAreaObj(kObjectType);
			Value luESDAreaObjkey;
			luESDAreaObjkey.SetString("LULZAreas", allocator);

			luESDAreaObj.AddMember(elevAK, elevAV, allocator);
			luESDAreaObj.AddMember(distAK, distAV, allocator);
			luESDAreaObj.AddMember(slopeAK, slopeAV, allocator);
			luESDAreaObj.AddMember(tcCK, tcCV, allocator);
			luESDAreaObj.AddMember(tLuAK, tLuAV, allocator);
			luESDAreaObj.AddMember(tLuPK, tLuPV, allocator);

			//totalSubAreaObj.AddMember(totalCellCountK, Value, allocator);
			tempLuESD.AddMember(luESDAreaObjkey, luESDAreaObj, allocator);


			// Blocks to store subareas
			Value subTotalAreaObj(kObjectType);
			Value subTotalAreakey;
			subTotalAreakey.SetString("TotalSubArea", allocator);

			// Check whether the object has the area key.
			Value::ConstMemberIterator itr = tempLuESD.FindMember(subTotalAreakey);
			if (itr == tempLuESD.MemberEnd())
			{
				//printf("Do not have this member\n");
				// There is no total area, Create a value and add it
				Value subTotalCellKey;
				subTotalCellKey.SetString("TotalCellCount", allocator);
				Value subTotalCellVal;
				len = sprintf(buffer, "%d", wsLuData->getLuData(luid)->totalsubcells);
				subTotalCellVal.SetString(buffer, len, allocator);
				memset(buffer, 0, sizeof(buffer));

				Value subTotalAreaKey;
				subTotalAreaKey.SetString("TotalArea", allocator);
				Value subTotalAreaVal;
				len = sprintf(buffer, "%f",
					(float)wsLuData->getLuData(luid)->totalsubcells * (float)tempdxc * (float)tempdyc / (float)10000.0);
				subTotalAreaVal.SetString(buffer, len, allocator);
				memset(buffer, 0, sizeof(buffer));

				subTotalAreaObj.AddMember(subTotalCellKey, subTotalCellVal, allocator);
				subTotalAreaObj.AddMember(subTotalAreaKey, subTotalAreaVal, allocator);
				tempLuESD.AddMember(subTotalAreakey, subTotalAreaObj, allocator);
			}
		}
		luESDJson.AddMember(tempLuNoKey, tempLuESD, allocator);
	}
	

	//printf("\nModified JSON with reformatting:\n");
	StringBuffer sbLuESDJson;
	PrettyWriter<StringBuffer> writer(sbLuESDJson);
	luESDJson.Accept(writer);    // Accept() traverses the DOM and generates Handler events.
	//printf("%s\n", sbsubLuESDJson.GetString());


	// After getting the sting, write them into the output file
	FILE* file = fopen(lzpvajs, "wb");
	if (file)
	{
		fputs(sbLuESDJson.GetString(), file);
		fclose(file);
	}




	//// Create and write output files file
	//// There are two files to write:
	//FILE *flzpOut;
	//flzpOut = fopen(lzpvajs, "w");
	//FILE *flzaout;
	//flzaout = fopen(lzareafile, "w");
	//
	//// Write head line for lzpoints
	//fprintf(flzpOut, "luno|var|vartype|values\n");
	//fprintf(flzaout, "luno|var|areaUnderCurve|areaThisLU|areaPerThisLu|TotalSubcell|TotalsubAreainha\n");


	////printf("xy reso: %f, %f", tempdxc, tempdyc);

	//for (auto &luid: luids)
	//{
	//	hsSearchRlt = wsLuData->SearchKey(luid);
	//	//printf("LuTEst: %d\n", hsSearchRlt);
	//	if (hsSearchRlt != (int)-1)
	//	{
	//		// Write to LZPoint files
	//		fprintf(flzpOut, "%d|%s|%s|", 
	//			wsLuData->getLuData(luid)->thisluno,
	//			"elev", "pointValue"
	//		);
	//		for (auto & itev : wsLuData->getLuData(luid)->elevarr)
	//		{
	//			fprintf(flzpOut, "%f,", itev);
	//		}
	//		fprintf(flzpOut, "\n");

	//		fprintf(flzpOut, "%d|%s|%s|",
	//			wsLuData->getLuData(luid)->thisluno,
	//			"elev", "accumAreaPer"
	//		);
	//		for (auto & itep : wsLuData->getLuData(luid)->elevperarr)
	//		{
	//			fprintf(flzpOut, "%f,", itep);
	//		}
	//		fprintf(flzpOut, "\n");


	//		fprintf(flzpOut, "%d|%s|%s|",
	//			wsLuData->getLuData(luid)->thisluno,
	//			"dist2subolt", "pointValue"
	//		);
	//		for (auto & itdv : wsLuData->getLuData(luid)->distarr)
	//		{
	//			fprintf(flzpOut, "%f,", itdv);
	//		}
	//		fprintf(flzpOut, "\n");

	//		fprintf(flzpOut, "%d|%s|%s|",
	//			wsLuData->getLuData(luid)->thisluno,
	//			"dist2subolt", "accumAreaPer"
	//		);
	//		for (auto & itdp : wsLuData->getLuData(luid)->distperarr)
	//		{
	//			fprintf(flzpOut, "%f,", itdp);
	//		}
	//		fprintf(flzpOut, "\n");

	//		fprintf(flzpOut, "%d|%s|%s|",
	//			wsLuData->getLuData(luid)->thisluno,
	//			"slope", "pointValue"
	//		);
	//		for (auto & itsv : wsLuData->getLuData(luid)->slparr)
	//		{
	//			fprintf(flzpOut, "%f,", itsv);
	//		}
	//		fprintf(flzpOut, "\n");

	//		fprintf(flzpOut, "%d|%s|%s|",
	//			wsLuData->getLuData(luid)->thisluno,
	//			"slope", "accumAreaPer"
	//		);
	//		for (auto & itsp : wsLuData->getLuData(luid)->slpperarr)
	//		{
	//			fprintf(flzpOut, "%f,", itsp);
	//		}
	//		fprintf(flzpOut, "\n");

	//		// Write to LZArea files
	//		//subno|luno|var|areaUnderLZCurve|areaThisLUinha|areaPerThisLu|TotalSubcell|TotalsubAreainha\n
	//		fprintf(flzaout, "%d|%s|%f|%f|%f|%f|%f\n",
	//			wsLuData->getLuData(luid)->thisluno,
	//			"elev",
	//			wsLuData->getLuData(luid)->elevarea,
	//			float(wsLuData->getLuData(luid)->totallucells)*(float)tempdxc*(float)tempdyc / (float)10000.0,
	//			(float)wsLuData->getLuData(luid)->luareaper,
	//			(float)wsLuData->getLuData(luid)->totalsubcells,
	//			float(wsLuData->getLuData(luid)->totalsubcells) * (float)tempdxc * (float)tempdyc/(float)10000.0
	//		);
	//		fprintf(flzaout, "%d|%s|%f|%f|%f|%f|%f\n",
	//			wsLuData->getLuData(luid)->thisluno,
	//			"dist",
	//			wsLuData->getLuData(luid)->distarea,
	//			(float)wsLuData->getLuData(luid)->totallucells* (float)tempdxc * (float)tempdyc / (float)10000.0,
	//			(float)wsLuData->getLuData(luid)->luareaper,
	//			(float)wsLuData->getLuData(luid)->totalsubcells,
	//			(float)wsLuData->getLuData(luid)->totalsubcells * (float)tempdxc * (float)tempdyc / (float)10000.0
	//		);
	//		fprintf(flzaout, "%d|%s|%f|%f|%f|%f|%f\n",
	//			wsLuData->getLuData(luid)->thisluno,
	//			"slope",
	//			wsLuData->getLuData(luid)->slparea,
	//			(float)wsLuData->getLuData(luid)->totallucells* (float)tempdxc * (float)tempdyc / (float)10000.0,
	//			(float)wsLuData->getLuData(luid)->luareaper,
	//			(float)wsLuData->getLuData(luid)->totalsubcells,
	//			(float)wsLuData->getLuData(luid)->totalsubcells * (float)tempdxc * (float)tempdyc / (float)10000.0
	//		);




	//	}
	//}

	//fclose(flzpOut);
	//fclose(flzaout);

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

