/*
-------------------------------------------------------------------------------------------------------------
**
** Location weighted landscape index calculation program.
**
** This class was developed to do the calculation of landscape index.
** The inputs include :
** 1. land use map(should be reclassifed to include intended sink
** and source classes).
** 2. dem : defines the elevation
** 3. slope : calculated from the DEM
** 4. distance : calculated with the path distance tool with
** dem as surface factor in ArcGIS.
**
** Qingyu Feng
** 
** Jan 2018
**
** Updated June 2020
** This time, I removed the need to read in sink and source lus since
** all of them will be included in the lu lists.
** Steps reorganized:
** 1. Readin watershed boundary (wsbdy.asc)
** This will serve as the master boundary and controls the reading
** for all other variables.
** 2. Added reading pflowdir and dist2stream input 
** 3. Flow path length from each cell to the watershed outlet was 
** calculated by adding dist2strm to the stream to terminal (outlet).
** The distance of stream to term was read from the coord.txt
** 4. Arrays for all variables were initated with -9999 or -9999.0
** These were originally done using memset. But memset works better with
** int values. In addition, the arrays were initated with 0s. For distance
** watershed, slope etc, zero is not a proper initiate, especially when
** we need to sort it later.
** 5. Algorithm for calculating accumulated area was updated
** Originally, the area was calculated with index/total cell number.
** But the index started from 0 to total no-1. This created the maximum number less than 100.
** This was corrected by starting index from 1.
-------------------------------------------------------------------------------------------------------------
*/


// ------------------------------------------------------------------------------------------------------------
// Including standard and customized header files:
#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <cfloat>
#include <algorithm>


using namespace std;

#include "app.h"
#include "message.h"


void fatalError(const char *msg)
{
	char longMsg[1024];

	snprintf(longMsg, 1024, "The sslmfpws program has encountered an error\nand can not continue. The error is:\n\n%s", msg);

	fprintf(stdout, "%s %s\n", longMsg, "Error calculating LWLI");
	exit(-1);

}


/*
** App()
** Constructor for the main App object that drives everything.
*/
App::App()
{
	rows = cols = 0;
	asclu = NULL;
	ascwsbdy = NULL;
	ascelev = NULL;
	ascslope = NULL;
	ascdistoolt = NULL;
	ascdistostrm = NULL;
	coordlns = NULL;
	ascpflowdir = NULL;
	allsrcsinklus = NULL;
	rawludata = NULL;
	perludata = NULL;
	lwlis = NULL;

}


/*
** ~App()
**
** Destrcutor for the main application object. Main task is to free the memory.
*/

App::~App()
{
	if (asclu) delete (asclu);
	if (ascwsbdy) delete (ascwsbdy);
	if (ascelev) delete ascelev;
	if (ascslope) delete ascslope;
	if (ascdistoolt) delete ascdistoolt;
	if (ascdistostrm) delete ascdistostrm;
	if (coordlns) delete coordlns;
	if (ascpflowdir) delete ascpflowdir;
	if (allsrcsinklus) delete (allsrcsinklus);
	if (rawludata) delete (rawludata);
	if (perludata) delete (perludata);
	if (lwlis) delete (lwlis);

}


/*
** cleanMemory()
**
** Free all the dynamically allocated memory that was used.
**
*/
void App::cleanMemory()
{
	if (asclu) delete (asclu);
	if (ascwsbdy) delete (ascwsbdy);
	if (ascelev) delete ascelev;
	if (ascslope) delete ascslope;
	if (ascdistoolt) delete ascdistoolt;
	if (ascdistostrm) delete ascdistostrm;
	if (coordlns) delete coordlns;
	if (ascpflowdir) delete ascpflowdir;
	if (allsrcsinklus) delete (allsrcsinklus);
	if (rawludata) delete (rawludata);
	if (perludata) delete (perludata);
	if (lwlis) delete (lwlis);

	asclu = NULL;
	ascwsbdy = NULL;
	ascelev = NULL;
	ascslope = NULL;
	ascdistoolt = NULL;
	ascdistostrm = NULL;
	coordlns = NULL;
	ascpflowdir = NULL;
	allsrcsinklus = NULL;
	rawludata = NULL;
	perludata = NULL;
	lwlis = NULL;

}





/*
** getUniqueLu()
**
** Get Unique land use numbers within watershed
**
*/
int *App::getUniqueLu()
{
	// Declaring variables
	int *data;

	// Start reading datalines
	// initiate the container data	
	data = new int[MAX_LUIDS];
	if (data == NULL)
	{
		fatalError("Out of memory in readArcviewInt()");
	}

	memset(data, 0, sizeof(int)*MAX_LUIDS);
	for (int ii = 0; ii < MAX_LUIDS; ii++) {
		data[ii] = -9999;
	}

	// Start geting the data and put them into the data
	int index = 0;
	int dtidx = 0;

	for (int i = 0; i < rows; i++)
	{
		// i is the row number, each row has cols number of columns.
		// index is the starting point of each column.

		//std::cout << "Row: " << i << std::endl;

		index = i * cols;
		for (int j = 0; j < cols; j++)
		{
			// For each lu: 
			// 1. Judge whether it is already in the data list.
			// If in break;
			// If not in, append it to the data
			if (asclu[index] != noDataLu) {
				
				// Judge whether the data is already in the data
				bool inData = false;
				for (int k = 0; k < MAX_LUIDS; k++) {
					if (asclu[index] == data[k])  {
						inData = true;
						break;
					}
				}

				if (inData == false && asclu[index] != noDataLu)
				{
					//std::cout << "before data: " << data[dtidx] << ", col: " << j << std::endl;
					data[dtidx] = asclu[index];
					//std::cout << "after data" << data[dtidx] << ", col: " << j << std::endl;
					//Update the index for data, ranging from 0 to MAX_LUIDS
					dtidx++;
				}
			}
			index++;
		}
	}

	//for (int k = 0; k < MAX_LUIDS; k++) {
	//	if (data[k] == noDataLu) {
	//		break;}
	//	else {std::cout << "luno: " << data[k] << std::endl;}
	//}

	return data;
}








/*
** readArcviewIntWs()
**
** Reads an arcview grid file and stores it into an integer array.
**
*/
int *App::readArcviewIntWs(const char *file)
{
	// Declaring variables
	FILE *fp = fopen(file, "r");
	char buf2[512];
	char *buf;
	char ebuf[256];
	int i;
	int *data;

	// Initiate variables
	buf = NULL;
	rows = cols = 0;

	snprintf(buf2, sizeof(buf2), "Reading grid: %s ...\n", file);
	DisplayMessage(buf2);

	if (fp)
	{
		// reading the first 6 lines
		for (i = 0; i < 6; i++)
		{
			fgets(buf2, 256, fp);
			if (!strncmp(buf2, "nrows", 5))
			{
				// sscanf: read data from s and stores
				// them according to parameter formats
				// into the locations given by the additional
				// arguments: here &rows.
				sscanf(&buf2[6], "%d", &rows);
			}
			else if (!strncmp(buf2, "ncols", 5))
			{
				sscanf(&buf2[6], "%d", &cols);
			}
			else if (!strncmp(buf2, "cellsize", 8))
			{
				sscanf(&buf2[9], "%f", &cellsize);
			}
			else if (!strncmp(buf2, "NODATA_value", 6))
			{
				sscanf(&buf2[13], "%d", &noDataWs);
			}
		}

		// Start reading datalines
		// initiate the container data	
		data = new int[rows*cols];
		if (data == NULL)
		{
			fatalError("Out of memory in readArcviewInt()");
		}
		buf = new char[MAX_COL_BYTES + 1];
		if (buf == NULL)
		{
			fatalError("Out of memory in readArcviewInt()");
		}
		// memset(void *ptr, int value, std::size_t num);
		// sets the first num bytes of the block of memory pointed
		// by prt to the specified value
		// sizeof (int): return size in bytes of the object representation of type;
		// sizeof expression: return size in bytes of the object
		// representation of the type that would be returned by
		// expression. 
		// Memset actually allocated the memory. And we do not
		// like 0 values, thus give it -9999 in a later loop
		memset(data, 0, sizeof(int)*rows*cols);
		// data is a one dimension array. The total number of elements
		// is rows*cols
		for (int ii = 0; ii < rows*cols; ii++) {
			data[ii] = -9999;
		}
		// Start geting the data and put them into the data
		int k;
		int index = 0;
		int val;
		bool rowHasData;

		for (i = 0; i < rows; i++)
		{
			// i is the row number, each row has cols number of columns.
			index = i * cols;
			if (fgets(buf, MAX_COL_BYTES, fp) != NULL)
			{
				if ((i == 0) && (strlen(buf) >= MAX_COL_BYTES))
				{
					delete(buf);
					fatalError("Line too long from grid file, max is 50000 bytes");
				}

				// At this time, all validRows value is still all 1s.
				if (validRows[i])
				{
					rowHasData = false;
					k = 0;
					while (buf[k] == ' ') { k++; }
					// Start working with columns.
					// Here later, the columns might be masked by the 
					// subnoinfield.
					for (int j = 0; j < cols; j++)
					{
						sscanf(&buf[k], "%d", &val);
						// Originally, the program uses the value of bounds
						// from the outputs of Topaz. Here, we do not have a bounds
						// output from TauDEM, but we have an array of subarea numbers
						// to help determine whether the row is valid row.

						// We should remove the no data values
						if (val != noDataWs) {
							data[index] = val;
							rowHasData = true;
						}

						//printf("Reading int..%d..%d..%d..%d..\n", i, j, index, data[index]);
						while ((buf[k] != ' ') && (buf[k] != '\n')) { k++; }
						while (buf[k] == ' ') { k++; }
						index++;
					}
					if (rowHasData == false)
					{
						validRows[i] = 0;
					}
				}
			}
		}
		delete(buf);
		fclose(fp);
	}
	else
	{
		snprintf(ebuf, sizeof(ebuf), "Can't find %s\n", file);
		fatalError(ebuf);
	}

	snprintf(buf2, sizeof(buf2), "Done Reading Grid: %s...\n", file);
	DisplayMessage(buf2);

	return data;

}








/*
** readArcviewIntLu()
**
** Reads an arcview grid file and stores it into an integer array.
**
*/
int *App::readArcviewIntLu(const char *file)
{
	// Declaring variables
	FILE *fp = fopen(file, "r");
	char buf2[512];
	char *buf;
	char ebuf[256];
	int i;
	int *data;

	// Initiate variables
	buf = NULL;
	rows = cols = 0;

	snprintf(buf2, sizeof(buf2), "Reading grid: %s ...\n", file);
	DisplayMessage(buf2);

	if (fp)
	{
		// reading the first 6 lines
		for (i = 0; i < 6; i++)
		{
			fgets(buf2, 256, fp);
			if (!strncmp(buf2, "nrows", 5))
			{
				// sscanf: read data from s and stores
				// them according to parameter formats
				// into the locations given by the additional
				// arguments: here &rows.
				sscanf(&buf2[6], "%d", &rows);
			}
			else if (!strncmp(buf2, "ncols", 5))
			{
				sscanf(&buf2[6], "%d", &cols);
			}
			else if (!strncmp(buf2, "cellsize", 8))
			{
				sscanf(&buf2[9], "%f", &cellsize);
			}
			else if (!strncmp(buf2, "NODATA_value", 6))
			{
				sscanf(&buf2[13], "%d", &noDataLu);
			}
		}

		// Start reading datalines
		// initiate the container data	
		data = new int[rows*cols];
		if (data == NULL)
		{
			fatalError("Out of memory in readArcviewInt()");
		}
		buf = new char[MAX_COL_BYTES + 1];
		if (buf == NULL)
		{
			fatalError("Out of memory in readArcviewInt()");
		}
		// memset(void *ptr, int value, std::size_t num);
		// sets the first num bytes of the block of memory pointed
		// by prt to the specified value
		// sizeof (int): return size in bytes of the object representation of type;
		// sizeof expression: return size in bytes of the object
		// representation of the type that would be returned by
		// expression. 
		memset(data, 0, sizeof(int)*rows*cols);
		// data is a one dimension array. The total number of elements
		// is rows*cols
		// In a fomer version, the value of dynamic memories were initialized
		// with memset. However, the memset function was sad to be safe for
		// intergers. That's the reason why the init values with memset has 
		// some wired values. 
		// A proper way is to use for loop and i will use that;
		for (int ii = 0; ii < rows*cols; ii++) {
			data[ii] = -9999;
		}
		//std::cout << "lu no data " << noDataLu << std::endl;

		// Start geting the data and put them into the data
		int k;
		int index = 0;
		int val;

		for (i = 0; i < rows; i++)
		{
			// i is the row number, each row has cols number of columns.
			//std::cout << "Row: " << i << std::endl;


			index = i * cols;
			if (fgets(buf, MAX_COL_BYTES, fp) != NULL)
			{
				if ((i == 0) && (strlen(buf) >= MAX_COL_BYTES))
				{
					delete(buf);
					fatalError("Line too long from grid file, max is 50000 bytes");
				}

				// At this time, all validRows value is still all 1s.
 				if (validRows[i])
				{
					k = 0;
					while (buf[k] == ' ') { k++; }
					// Start working with columns.
					// Here later, the columns might be masked by the 
					// subnoinfield.
					for (int j = 0; j < cols; j++)
					{
						sscanf(&buf[k], "%d", &val);

						// Only read those within the watershed boundary
						if (ascwsbdy[index] != -9999) {
							
							// We should remove the no data values
							if (val != noDataLu) {
								//std::cout << "ws data" << ascwsbdy[index] << ", NodataWS: " << &noDataWs << std::endl;
								//std::cout << "before lu data: " << val << " Col: " << j << std::endl;
								//std::cout << "before lu data" << data[index] << " Col: " << j << std::endl;
								data[index] = val;
								//std::cout << "after lu data: "  << data[index] << " ...." << std::endl;
							}
						}
						//printf("Reading int..%d..%d..%d..%d..\n", i, j, index, data[index]);
						while ((buf[k] != ' ') && (buf[k] != '\n')) { k++; }
						while (buf[k] == ' ') { k++; }
						index++;
					}
				}
			}
		}
		delete(buf);
		fclose(fp);
	}
	else
	{
		snprintf(ebuf, sizeof(ebuf), "Can't find %s\n", file);
		fatalError(ebuf);
	}

	snprintf(buf2, sizeof(buf2), "Done Reading Grid: %s...\n", file);
	DisplayMessage(buf2);

	return data;

}







/*
** readArcviewIntPFlow()
**
** Reads an arcview grid file and stores it into an integer array.
**
*/
int *App::readArcviewIntPFlow(const char *file)
{
	// Declaring variables
	FILE *fp = fopen(file, "r");
	char buf2[512];
	char *buf;
	char ebuf[256];
	int i;
	int *data;

	// Initiate variables
	buf = NULL;
	rows = cols = 0;

	snprintf(buf2, sizeof(buf2), "Reading grid: %s ...\n", file);
	DisplayMessage(buf2);

	if (fp)
	{
		// reading the first 6 lines
		for (i = 0; i < 6; i++)
		{
			fgets(buf2, 256, fp);
			if (!strncmp(buf2, "nrows", 5))
			{
				// sscanf: read data from s and stores
				// them according to parameter formats
				// into the locations given by the additional
				// arguments: here &rows.
				sscanf(&buf2[6], "%d", &rows);
			}
			else if (!strncmp(buf2, "ncols", 5))
			{
				sscanf(&buf2[6], "%d", &cols);
			}
			else if (!strncmp(buf2, "cellsize", 8))
			{
				sscanf(&buf2[9], "%f", &cellsize);
			}
			else if (!strncmp(buf2, "NODATA_value", 6))
			{
				sscanf(&buf2[13], "%d", &noDataPFlow);
			}
		}

		// Start reading datalines
		// initiate the container data	
		data = new int[rows*cols];
		if (data == NULL)
		{
			fatalError("Out of memory in readArcviewInt()");
		}
		buf = new char[MAX_COL_BYTES + 1];
		if (buf == NULL)
		{
			fatalError("Out of memory in readArcviewInt()");
		}
		// memset(void *ptr, int value, std::size_t num);
		// sets the first num bytes of the block of memory pointed
		// by prt to the specified value
		// sizeof (int): return size in bytes of the object representation of type;
		// sizeof expression: return size in bytes of the object
		// representation of the type that would be returned by
		// expression. 
		memset(data, 0, sizeof(int)*rows*cols);
		// data is a one dimension array. The total number of elements
		// is rows*cols
		// In a fomer version, the value of dynamic memories were initialized
		// with memset. However, the memset function was sad to be safe for
		// intergers. That's the reason why the init values with memset has 
		// some wired values. 
		// A proper way is to use for loop and i will use that;
		for (i = 0; i < rows*cols; i++) {
			data[i] = -9999;
		}
		//std::cout << "lu no data " << noDataLu << std::endl;

		// Start geting the data and put them into the data
		int k;
		int index = 0;
		int val;

		for (i = 0; i < rows; i++)
		{
			// i is the row number, each row has cols number of columns.
			//std::cout << "Row: " << i << std::endl;
			index = i * cols;
			if (fgets(buf, MAX_COL_BYTES, fp) != NULL)
			{
				if ((i == 0) && (strlen(buf) >= MAX_COL_BYTES))
				{
					delete(buf);
					fatalError("Line too long from grid file, max is 50000 bytes");
				}

				// At this time, all validRows value is still all 1s.
				if (validRows[i])
				{
					k = 0;
					while (buf[k] == ' ') { k++; }
					// Start working with columns.
					// Here later, the columns might be masked by the 
					// subnoinfield.
					for (int j = 0; j < cols; j++)
					{
						sscanf(&buf[k], "%d", &val);

						// Only read those within the watershed boundary
						if (ascwsbdy[index] != -9999) {

							// We should remove the no data values
							if (val != noDataPFlow) {
								//std::cout << "ws data" << ascwsbdy[index] << ", NodataWS: " << &noDataWs << std::endl;
								//std::cout << "before lu data: " << val << " Col: " << j << std::endl;
								//std::cout << "before flowdir data" << data[index] << " Col: " << j << std::endl;
								data[index] = val;
								//std::cout << "after flowdir data: "  << data[index] << " ...." << std::endl;
							}
						}
						//printf("Reading int..%d..%d..%d..%d..\n", i, j, index, data[index]);
						while ((buf[k] != ' ') && (buf[k] != '\n')) { k++; }
						while (buf[k] == ' ') { k++; }
						index++;
					}
				}
			}
		}
		delete(buf);
		fclose(fp);
	}
	else
	{
		snprintf(ebuf, sizeof(ebuf), "Can't find %s\n", file);
		fatalError(ebuf);
	}

	snprintf(buf2, sizeof(buf2), "Done Reading Grid: %s...\n", file);
	DisplayMessage(buf2);

	return data;

}







/*
** readArcviewFloat()
**
** Reads and ArcView grid file of float values and stores them into a floating
** point array.
**
*/
float *App::readArcviewFloat(const char *file)
{
	// Declaring variables
	FILE *fp = fopen(file, "r");
	char buf2[512];
	char *buf;
	char ebuf[256];
	int i;
	float *data;

	// Initiate variables
	buf = NULL;
	rows = cols = 0;

	snprintf(buf2, sizeof(buf2), "Reading grid: %s ...\n", file);
	DisplayMessage(buf2);

	if (fp)
	{
		// reading the first 6 lines
		for (i = 0; i < 6; i++)
		{
			fgets(buf2, 256, fp);
			if (!strncmp(buf2, "nrows", 5))
			{
				// sscanf: read data from s and stores
				// them according to parameter formats
				// into the locations given by the additional
				// arguments: here &rows.
				sscanf(&buf2[6], "%d", &rows);
			}
			else if (!strncmp(buf2, "ncols", 5))
			{
				sscanf(&buf2[6], "%d", &cols);
			}
			else if (!strncmp(buf2, "cellsize", 8))
			{
				sscanf(&buf2[9], "%f", &cellsize);
			}
			else if (!strncmp(buf2, "NODATA_value", 6))
			{
				sscanf(&buf2[13], "%f", &noData);
			}
			else if (!strncmp(buf2, "xllcorner", 9))
			{
				sscanf(&buf2[13], "%f", &xllcorner);
			}
			else if (!strncmp(buf2, "yllcorner", 9))
			{
				sscanf(&buf2[13], "%f", &yllcorner);
			}
		}

		// Start reading datalines
		// initiate the container data	
		data = new float[rows*cols];
		if (data == NULL)
		{
			fatalError("Out of memory in readArcviewFloat()");
		}
		buf = new char[MAX_COL_BYTES + 1];
		if (buf == NULL)
		{
			fatalError("Out of memory in readArcviewFloat()");
		}
		// memset(void *ptr, int value, std::size_t num);
		// sets the first num bytes of the block of memory pointed
		// by prt to the specified value
		// sizeof (int): return size in bytes of the object representation of type;
		// sizeof expression: return size in bytes of the object
		// representation of the type that would be returned by
		// expression. 
		memset(data, 0, sizeof(float)*rows*cols);
		// data is a one dimension array. The total number of elements
		// In a fomer version, the value of dynamic memories were initialized
		// with memset. However, the memset function was sad to be safe for
		// intergers. That's the reason why the init values with memset has 
		// some wired values. 
		// A proper way is to use for loop and i will use that;
		for (i = 0; i < rows*cols; i++) {
			data[i] = -9999.0;
		}

		// Start geting the data and put them into the data
		int k;
		int index = 0;
		float val;

		for (i = 0; i<rows; i++)
		{
			// i is the row number, each row has cols number of columns.
			//std::cout << "Row: " << i << std::endl;
			index = i*cols;
			if (fgets(buf, MAX_COL_BYTES, fp) != NULL)
			{
				if ((i == 0) && (strlen(buf) >= MAX_COL_BYTES))
				{
					delete(buf);
					fatalError("Line too long from grid file, max is 50000 bytes");
				}
				// At this time, all validRows value is still all 1s.
				if (validRows[i])
				{
					k = 0;
					while (buf[k] == ' ') { k++; }
					// Start working with columns.
					// Here later, the columns might be masked by the 
					// subnoinfield.
					for (int j = 0; j<cols; j++)
					{
						sscanf(&buf[k], "%f", &val);
						// Originally, the program uses the value of bounds
						// from the outputs of Topaz. Here, we do not have a bounds
						// output from TauDEM. For the last program, we got the 
						// array of subwta. If subwta not in the list, 
						// the value is assigned to 0. We will do the 
						// same thing here. 
						// We should remove the no data values
						// Only read those within the watershed boundary
						if (ascwsbdy[index] != -9999) {

							// We should remove the no data values
							if (val != noData) {
								//std::cout << "ws data" << ascwsbdy[index] << ", NodataWS: " << &noDataWs << std::endl;
								//std::cout << "before lu data: " << val << " Col: " << j << std::endl;
								//std::cout << "before flowdir data" << data[index] << " Col: " << j << std::endl;
								data[index] = val;
								//std::cout << "after flowdir data: "  << data[index] << " ...." << std::endl;
							}
						}
						// Skipping the spaces
						while ((buf[k] != ' ') && (buf[k] != '\n')) { k++; }
						while (buf[k] == ' ') { k++; }
						index++;
					}
				}
			}
		}

		delete(buf);
		fclose(fp);
	}
	else
	{
		snprintf(ebuf, sizeof(ebuf), "Can't find %s\n", file);
		fatalError(ebuf);
	}

	snprintf(buf2, sizeof(buf2), "Done Reading Grid: %s...\n", file);
	DisplayMessage(buf2);

	return data;

}





/*
** readCoord()
**
** Reads the coord into an array.
**
*/
float *App::readCoord(const char *file)
{
	// Declaring variables
	FILE *fp = fopen(file, "r");
	char buf2[512];
	char *buf;
	char ebuf[256];
	int i;
	float *data;

	int coordRows = rows * cols;
	int coordCols = 7;

	// Initiate variables
	buf = NULL;

	snprintf(buf2, sizeof(buf2), "Reading grid: %s ...\n", file);
	DisplayMessage(buf2);

	if (fp)
	{
		// Start reading datalines
		// initiate the container data	
		// Here we have all cells to make sure that the container is larger enough.
		// The total length of the data has rows*cols lines.
		// Each line will have 7 variables
		data = new float[coordRows*coordCols];
		if (data == NULL)
		{
			fatalError("Out of memory in readArcviewFloat()");
		}
		buf = new char[MAX_COL_BYTES + 1];
		if (buf == NULL)
		{
			fatalError("Out of memory in readArcviewFloat()");
		}
		// memset(void *ptr, int value, std::size_t num);
		// sets the first num bytes of the block of memory pointed
		// by prt to the specified value
		// sizeof (int): return size in bytes of the object representation of type;
		// sizeof expression: return size in bytes of the object
		// representation of the type that would be returned by
		// expression. 
		memset(data, 0, sizeof(float)*rows*cols);
		// data is a one dimension array. The total number of elements
		// is rows*cols

		// Here, an initial value of -1 was assigned. 0 was not used because
		// some distance are 0s.
		// Rows*cols = number of lines
		// Each line will have 7 variables
		for (i = 0; i < coordRows*coordCols; i++) {
			data[i] = -9999.0;
		}
		// Start geting the data and put them into the data
		int k;
		int index = 0;
		float val;
		float rowdiff;
		float coldiff;

		// Calculate the row and column from the coordinates
		// There is a 15 meter difference for the xll.
		// XLL in the asc file refers to the lower left corner of
		// the lower left corner of the cell.
		// But the coordinates are for the center of the cell.
		float xllcorner2 = xllcorner + cellsize / (float) 2.0;
		// There is a 15 meter difference for the xll.
		// XLL in the asc file refers to the lower left corner of
		// the lower left corner of the cell.
		// But the coordinates are for the center of the cell.
		float yllcorner2 = yllcorner + cellsize / (float) 2.0;

		for (i = 0; i < coordRows; i++)
		{
			// i is the row number, each row has cols number of columns.
			//std::cout << "Row: " << i << std::endl;
			index = i * coordCols;
			if (fgets(buf, MAX_COL_BYTES, fp) != NULL)
			{
				if ((i == 0) && (strlen(buf) >= MAX_COL_BYTES))
				{
					delete(buf);
					fatalError("Line too long from grid file, max is 50000 bytes");
				}
				//printf(".bfrstr...%s...\n", buf);
				k = 0;
				while (buf[k] == '\t') { k++; }
				// Start working with columns.
				// Here later, the columns might be masked by the 
				// subnoinfield.
				for (int j = 0; j < coordCols; j++)
				{
					// For the first 5 variables, read from table
					if (j < 5)
					{ 
						sscanf(&buf[k], "%f", &val);
						// Originally, the program uses the value of bounds
						// from the outputs of Topaz. Here, we do not have a bounds
						// output from TauDEM. For the last program, we got the 
						// array of subwta. If subwta not in the list, 
						// the value is assigned to 0. We will do the 
						// same thing here. 
						// We should remove the no data values
						if (val != noData) {
							//printf(".Before...%d..%d..%lf..%d..%lf..\n", i, j, data[index], index, val);
							data[index] = val;
							//printf(".After...%d..%d..%d..%lf..%d..%lf..\n", i, j, k, data[index], index, val);
						}
						// Skipping the spaces
						while ((buf[k] != '\t') && (buf[k] != '\n')) { k++; }
						while (buf[k] == '\t') { k++; }
					}
					// J == 5 will be row number in the asc files
					// J == 6 will be the col number in the asc files
					else if (j == 5) {
						
						coldiff = (data[index - 5] - xllcorner2) / cellsize;
						//printf(".After....%d..%d..%lf..%lf..%lf...\n", j, coldiff, xllcorner, data[index - 5], cellsize);
						// The value of real row will be assigned to the data
						// For the row, real row = total row number - row diff. 
						// Since this rowdiff starts from the lowerleft corner
						data[index] = coldiff+1;
					}

					else if (j == 6) {
						// Calculate the row and column from the coordinates
						
						rowdiff = (data[index - 5] - yllcorner2) / cellsize;
						//printf(".After....%d..%d..%lf..%lf..%lf...%d..\n", j, rowdiff, yllcorner, data[index - 5], cellsize, rows);
						// different from the calculation for the row, columns need to be
						// added 1 for real col number
						data[index] = (float)rows - rowdiff;
					}

					index++;
				}
			}
			else { break; }
		}

		delete(buf);
		fclose(fp);
	}
	else
	{
		snprintf(ebuf, sizeof(ebuf) , "Can't find %s\n", file);
		fatalError(ebuf);
	}

	snprintf(buf2, sizeof(buf2), "Done Reading Grid: %s...\n", file);
	DisplayMessage(buf2);

	return data;

}




/*
** getDistStm2OltinCoord()
**
** This function get the distance from streamcell to outlet
** stored in the Coordln array, which is a 2d array.
**
*/
float App::getDistStm2OltinCoord(int cellrow, int cellcol)
{

	float dists2olt = 0.0;

	int coordRows = rows * cols;
	int coordCols = 7;
	int i;
	int index = 0;

	for (i = 0; i < coordRows; i++)
	{
		// i is the row number, each row has cols number of columns.
		//std::cout << "Row: " << i << std::endl;
		index = i * coordCols;

		// Find the corresponding value  for row and col pair.
		if (coordlns[index+6] == (float)cellrow && coordlns[index + 5] == (float)cellcol) {
			// This operation assigns the value of coordlns[index + 2];
			// to the 
			dists2olt = coordlns[index + 2];
		}

		// We still need to update the index for looping;
		for (int j = 0; j < coordCols; j++)
		{
			index++;
		}
	}

	return dists2olt;
}





/*
** getStrmCellandDistinCoord()
**
** This function get the distance from streamcell to outlet
** stored in the Coordln array, which is a 2d array.
**
*/
float App::getStrmCellandDistinCoord(int cellrow, int cellcol)
{

	float strmCellDist2Olt = 0.0;

	bool foundStrmCell = false;
	// Initiate two temp values for searching stream cells
	// The r and c here are not like other arrays. The array
	// is accessed by index. So, we need to calculate the 
	// index based on updated r and c values.
	int index1 = 0;
	int r = cellrow;
	int c = cellcol;
	index1 = r * cols + c;
	//printf(".starting cell distance..%d..%d..%f....\n", r, c, ascdistostrm[index1]);
	while (foundStrmCell == false) {
		// Get the starting cell's flowdirection
		//printf(".flowdirection..%d..%d..%d..\n", r, c, ascpflowdir[index1]);
		// Update the r and c based on flow direction
		switch (ascpflowdir[index1])
		{
		case 1:
			c++;
			break;
		case 2:
			r--;
			c++;
			break;
		case 3:
			r--;
			break;
		case 4:
			r--;
			c--;
			break;
		case 5:
			c--;
			break;
		case 6:
			r++;
			c--;
			break;
		case 7:
			r++;
			break;
		case 8:
			r++;
			c++;
			break;
		}

		// After modifying the r and c, calculate the index and
		// judge based on the dist2strm value to see whether we found
		// a stream cell. At this time, the r and c are the row and 
		// column of the stream cell
		index1 = r * cols + c;

		if (ascdistostrm[index1] == 0.0) {
			foundStrmCell = true;
			//printf(".stream to outlet incoord..%d..%d..%f....\n", r, c, getDistStm2OltinCoord(r + 1, c + 1));
		}
	}
	// After finding the stream cell, update the data by adding the 
	// distrance from stream to outlet in the Coord to the dist2stream value
	strmCellDist2Olt = getDistStm2OltinCoord(r + 1, c + 1);

	
	return strmCellDist2Olt;
}

/*
** getDistToOlt()
**
** This function create an array of ascdistoolt based on the
** dist2strm + distance to terminal from stream.
** The steps include find the corresponding steam cell
** for each cell in dist2strm and add the values from 
** coord.
**
*/
float *App::getDistToOlt()
{
	// Declaring variables
	char buf2[512];
	int i;
	float *data;


	snprintf(buf2, sizeof(buf2), "Generating distance to outlet ...\n");
	DisplayMessage(buf2);

	// Start reading datalines
	// initiate the container data	
	// New: dynamic memory is allocated using the operator new.
	// Followed by a data type specifier.
	// Now data points to a valid block of memory with space for 
	// elements of rows* cols number of element of f
	data = new float[rows*cols];
	if (data == NULL)
	{
		fatalError("Out of memory in readArcviewFloat()");
	}

	// In a fomer version, the value of dynamic memories were initialized
	// with memset. However, the memset function was sad to be safe for
	// intergers. That's the reason why the init values with memset has 
	// some wired values. 
	// A proper way is to use for loop and i will use that;
	// However, there are some error without memset, which allocated
	// the memoary first.
	memset(data, 0, sizeof(float)*rows*cols);
	for (i = 0; i < rows*cols; i++) {
		data[i] = -9999.0;
	}
	
	// Start geting the data and put them into the data
	int index = 0;

	// This variable is the one  found from the coord.txt
	// based on the row and column.

	for (i = 0; i < rows; i++)
	{
		// i is the row number, each row has cols number of columns.
		//std::cout << "Row: " << i << std::endl;
 		index = i * cols;

		for (int j = 0; j < cols; j++)
		{
			
			// Skip no data values
			if (ascwsbdy[index] != -9999) {
				// For each value in dist2strm,
				//printf(".Dist2Strm..%d..%d..%lf....\n", i, j, ascdistostrm[index]);
				//printf(".pFlowdir..%d..%d..%d....\n", i, j, ascpflowdir[index]);
				//printf(".dist2Outlet..%d..%d..%f....\n", i, j, data[index]);
				
				// If the distance to stream is 0, means that this cell
				// is a stream cell. Directly add the distance from the 
				// coordinate to the dist2outlet
				if (ascdistostrm[index] == 0.0) {
					//printf(".Dist2StrmOlt..%d..%d..%f....\n", i+1, j+1, getDistStm2OltinCoord(i+1, j+1));
					//dist = getDistStm2OltinCoord(i + 1, j + 1);
					data[index] = getDistStm2OltinCoord(i + 1, j + 1);;
					//printf(".Dist2StrmOlt..%d..%d..%lf....\n", i, j, data[index]);
				} 
				// If the distance to stream is not zero, we need to find out the cell
				// of stream, and then, get the distance from stream to outlet at the coord.
				else 
				{
					//dist = getStrmCellandDistinCoord(i, j);
					//printf(".Dist2StrmOlt..%d..%d....\n", i , j);
					data[index] = ascdistostrm[index] + getStrmCellandDistinCoord(i, j);
					//printf(".Dist2StrmOlt..%d..%d..%f....%f..%f..\n", i, j, data[index], ascdistostrm[index], getStrmCellandDistinCoord(i, j));
				}
			}
			index++;
		}
	}

	snprintf(buf2, sizeof(buf2), "Done Generating distance to outlet...\n");
	DisplayMessage(buf2);

	return data;

}









/*
** asc2ludata()
**
** This function put the data read from the ASC files into
** the corresponding array of land use data.
**
*/
App::Ludata *App::asc2ludata()
{
	char buf2[512];
	snprintf(buf2, sizeof(buf2), "Putting ascii data into corresponding land use data arrays!!\n");
	DisplayMessage(buf2);

	//Ludataarray psinksrc;
	Ludata *templudata = new Ludata;
	
	for (int luidx = 0; luidx < MAX_LUIDS; luidx++)
	{
		// Initialize the array
		if (allsrcsinklus[luidx] == -9999) { break; }
		else
		{
			templudata->elevarray[luidx] = new float[rows*cols];
			memset(templudata->elevarray[luidx], 0, sizeof(float)*rows*cols);
			for (int id1 = 0; id1 < rows*cols; id1++) {
				templudata->elevarray[luidx][id1] = -9999.0;
			}
			
			templudata->slopearray[luidx] = new float[rows*cols];
			memset(templudata->slopearray[luidx], 0, sizeof(float)*rows*cols);
			for (int id2 = 0; id2 < rows*cols; id2++) {
				templudata->slopearray[luidx][id2] = -9999.0;
			}
			
			templudata->distarray[luidx] = new float[rows*cols];
			memset(templudata->distarray[luidx], 0, sizeof(float)*rows*cols);
			for (int id3 = 0; id3 < rows*cols; id3++) {
				templudata->distarray[luidx][id3] = -9999.0;
			}
			
		}

		// Initialize the counter
		templudata->ludtctrarray[luidx] = 0;
		templudata->finaldistctr[luidx] = 0;
		templudata->finalelevctr[luidx] = 0;
		templudata->finalslpctr[luidx] = 0;

		// Initialize the luno
		templudata->luno = allsrcsinklus[luidx];
	}

	int tidx = 0;
	for (int rid = 0; rid < rows; rid++)
	{
		tidx = rid * cols;
		//std::cout << "Row: " << rid << std::endl;
		for (int cid = 0; cid < cols; cid++)
		{
			if (ascwsbdy[tidx] != -9999) {
				//printf(".landuse..%d..%d..%d..%d....\n", rid, cid,  tidx, asclu[tidx]);
				//printf(".elev..%d..%d..%d..%f....\n", rid, cid, tidx, ascelev[tidx]);
				//printf(".slope..%d..%d..%d..%f....\n", rid, cid, tidx, ascslope[tidx]);
				//printf(".dist..%d..%d..%d..%f....\n", rid, cid, tidx, ascdistoolt[tidx]);

				for (int luidx = 0; luidx < MAX_LUIDS; luidx++)
				{
					//for each lu, get the corresponding elev, slp and dist2olt values
					if (allsrcsinklus[luidx] == -9999) { break; }
					else if (asclu[tidx] == allsrcsinklus[luidx])
					{
						templudata->elevarray[luidx][templudata->ludtctrarray[luidx]] = ascelev[tidx];
						templudata->slopearray[luidx][templudata->ludtctrarray[luidx]] = ascslope[tidx];
						templudata->distarray[luidx][templudata->ludtctrarray[luidx]] = ascdistoolt[tidx];
						templudata->ludtctrarray[luidx]++;
					}
					templudata->finaldistctr[luidx] = templudata->ludtctrarray[luidx];
					templudata->finalelevctr[luidx] = templudata->ludtctrarray[luidx];
					templudata->finalslpctr[luidx] = templudata->ludtctrarray[luidx];
				}
			}
			tidx++;
		}
	}

	snprintf(buf2, sizeof(buf2), "Finished putting ascii data into corresponding land use data arrays!!\n");
	DisplayMessage(buf2);


	return templudata;
}



/*
** sortludata()
**
** This function sort the ludatas.
**
*/
void App::sortludata()
{
	char buf2[512];
	snprintf(buf2, sizeof(buf2), "Sorting distance, elevation and slope data!!\n");
	DisplayMessage(buf2);

	for (int luidx = 0; luidx < MAX_LUIDS; luidx++)
	{
		if (allsrcsinklus[luidx] != -9999)
		{
			// In the same loop, do the sorting:
			// Sort method is working;
			sort(rawludata->elevarray[luidx], 
				rawludata->elevarray[luidx]+rawludata->ludtctrarray[luidx]);
			sort(rawludata->slopearray[luidx],
				rawludata->slopearray[luidx] + rawludata->ludtctrarray[luidx]);
			sort(rawludata->distarray[luidx],
				rawludata->distarray[luidx] + rawludata->ludtctrarray[luidx]);

			// Block of code checking the data
			//printf("\n..Landuse...%d.....", allsrcsinklus[luidx]);
			//int tidx = 0;
			//for (int rid = 0; rid < rows; rid++)
			//{
			//	tidx = rid * cols;
			//	std::cout << "Row: " << rid << std::endl;
			//	for (int cid = 0; cid < cols; cid++)
			//	{
			//		printf(".elev..%d..%d..%d..%f....\n", rid, cid, tidx, rawludata->elevarray[luidx][tidx]);
			//		printf(".slp..%d..%d..%d..%f....\n", rid, cid, tidx, rawludata->slopearray[luidx][tidx]);
			//		printf("_distance.%d..%d._%d..%f__\n",rid, cid, tidx, rawludata->distarray[luidx][tidx]);
			//		tidx++;
			//		if (rawludata->distarray[luidx][tidx] == -9999.0) { break; }
			//	}
			//	if (rawludata->distarray[luidx][tidx] == -9999.0) { break; }
			//}

		}
		else { break; }
	}

	snprintf(buf2, sizeof(buf2), "Finished sorting distance, elevation and slope data!!\n");
	DisplayMessage(buf2);


}


/*
** calperludata()
**
** This function calculate the percent of the ludatas.
**
*/
App::Ludata *App::calperludata()
{
	char buf2[512];
	snprintf(buf2, sizeof(buf2), "Calculating percentage of distance, elevation and slope data!!\n");
	DisplayMessage(buf2);


	//Ludataarray psinksrc;
	Ludata *templudata = new Ludata;

	for (int luidx = 0; luidx < MAX_LUIDS; luidx++)
	{
		// Initialize the array
		if (allsrcsinklus[luidx] == -9999) { break; }
		else
		{
			//printf("\n..Landuse...%d.....", allsrcsinklus[luidx]);
			// To reduce the memory use, here, we will use the total number
			// of values
			templudata->elevarray[luidx] = new float[rawludata->ludtctrarray[luidx]];
			memset(templudata->elevarray[luidx], 0, sizeof(float)*rawludata->ludtctrarray[luidx]);
			for (int id1 = 0; id1 < rawludata->ludtctrarray[luidx]; id1++) {
				templudata->elevarray[luidx][id1] = -9999.0;
			}

			templudata->slopearray[luidx] = new float[rawludata->ludtctrarray[luidx]];
			memset(templudata->slopearray[luidx], 0, sizeof(float)*rawludata->ludtctrarray[luidx]);
			for (int id2 = 0; id2 < rawludata->ludtctrarray[luidx]; id2++) {
				templudata->slopearray[luidx][id2] = -9999.0;
			}
			

			templudata->distarray[luidx] = new float[rawludata->ludtctrarray[luidx]];
			memset(templudata->distarray[luidx], 0, sizeof(float)*rawludata->ludtctrarray[luidx]);
			for (int id3 = 0; id3 < rawludata->ludtctrarray[luidx]; id3++) {
				templudata->distarray[luidx][id3] = -9999.0;
			}
			
		}

		// Initialize the counter
		templudata->ludtctrarray[luidx] = rawludata->ludtctrarray[luidx];
		templudata->finaldistctr[luidx] = rawludata->ludtctrarray[luidx];
		templudata->finalelevctr[luidx] = rawludata->ludtctrarray[luidx];
		templudata->finalslpctr[luidx] = rawludata->ludtctrarray[luidx];

		//printf(".ludtctrarray..%d.....\n",  rawludata->ludtctrarray[luidx]);

		// Initialize the luno
		templudata->luno = allsrcsinklus[luidx];
	}

	
	for (int luidx = 0; luidx < MAX_LUIDS; luidx++)
	{
		if (allsrcsinklus[luidx] != -9999)
		{
			//printf("\n..Landuse...%d...%d..", allsrcsinklus[luidx], rawludata->ludtctrarray[luidx]);

			for (int index = 0; index < rawludata->ludtctrarray[luidx]; index++)
			{
				// There are m number of cells.
				// Each cell has an area of 1 and the total area is m cells.
				// Thus the accumulated area of each cell is index/m/
				// Here, we started index from 1 to represent the area of the first cell.
				templudata->slopearray[luidx][index] = (float)(index + 1) * (float)100. / (float)rawludata->ludtctrarray[luidx];
				templudata->distarray[luidx][index] = (float)(index + 1) * (float)100. / (float)rawludata->ludtctrarray[luidx];
				templudata->elevarray[luidx][index] = (float)(index + 1) * (float)100. / (float)rawludata->ludtctrarray[luidx];
				//printf("_total elev_..%d_%d__%f_\n", index, rawludata->ludtctrarray[luidx], templudata->elevarray[luidx][index]);
			}
		}
		else { break; }
	}

	snprintf(buf2, sizeof(buf2), "Finished calculating percentage of distance, elevation and slope data!!\n");
	DisplayMessage(buf2);

	return templudata;
}



/*
** removeDuplicates()
**
** This function remove duplicates in the array.
** Two arrays are taking here, dataarray stands for the ordar array.
** dataarray2 is for the percent, since the corresponding
** percentage value of the data need to be removed when the 
** value was removed.
**
*/
void App::removeDuplicates()
{

	char buf2[512];
	snprintf(buf2, sizeof(buf2), "Removing duplicates in distance, elevation and slope data!!\n");
	DisplayMessage(buf2);

	for (int luidx = 0; luidx < MAX_LUIDS; luidx++)
	{
		if (allsrcsinklus[luidx] != -9999)
		{
			//printf("\n..Landuse...%d.....", allsrcsinklus[luidx]);
			for (int idx = 0; idx < rawludata->ludtctrarray[luidx]; idx++)
				// Here, we use the final counter from the perludata.
				// This has been updated during the removal of duplicates.
			{
				if(rawludata->elevarray[luidx][idx] == rawludata->elevarray[luidx][idx+1])
				{
					//printf("_Init_%f..%f__\n", rawludata->elevarray[luidx][idx + 1], rawludata->elevarray[luidx][idx]);
					rawludata->elevarray[luidx][idx] = -9999.0;
					perludata->elevarray[luidx][idx] = -9999.0;
					perludata->finalelevctr[luidx]--;
				}

				if (rawludata->distarray[luidx][idx] == rawludata->distarray[luidx][idx + 1])
				{
					rawludata->distarray[luidx][idx] = -9999.0;
					perludata->distarray[luidx][idx] = -9999.0;
					perludata->finaldistctr[luidx]--;
				}

				if (rawludata->slopearray[luidx][idx] == rawludata->slopearray[luidx][idx + 1])
				{
					rawludata->slopearray[luidx][idx] = -9999.0;
					perludata->slopearray[luidx][idx] = -9999.0;
					perludata->finalslpctr[luidx]--;
				}

				//printf("_Final_%f..%f__\n", rawludata->distarray[luidx][idx + 1], rawludata->distarray[luidx][idx]);

			}

			//printf("_total slp_..%d__\n", perludata->finalslpctr[luidx]);
			//printf("_total elev_..%d__\n", perludata->finalelevctr[luidx]);
			//printf("_total dist_..%d__\n", perludata->finaldistctr[luidx]);

			sort(rawludata->elevarray[luidx],
				rawludata->elevarray[luidx] + rawludata->ludtctrarray[luidx],
				greater<float>());
			sort(rawludata->slopearray[luidx],
				rawludata->slopearray[luidx] + rawludata->ludtctrarray[luidx],
				greater<float>());
			sort(rawludata->distarray[luidx],
				rawludata->distarray[luidx] + rawludata->ludtctrarray[luidx],
				greater<float>());

			sort(perludata->elevarray[luidx],
				perludata->elevarray[luidx] + perludata->ludtctrarray[luidx],
				greater<float>());
			sort(perludata->slopearray[luidx],
				perludata->slopearray[luidx] + perludata->ludtctrarray[luidx],
				greater<float>());
			sort(perludata->distarray[luidx],
				perludata->distarray[luidx] + perludata->ludtctrarray[luidx],
				greater<float>());

			// Block of code checking the data
			//printf("\n..Landuse...%d....\n.", allsrcsinklus[luidx]);
			//for (int idx2 = 0; idx2 <= perludata->finalelevctr[luidx]; idx2++)
			//{
			//	printf("__.%f__", rawludata->elevarray[luidx][idx2]);
			//	printf("__.%f__\n", perludata->elevarray[luidx][idx2]);
			//	//if (rawludata->elevarray[luidx][idx2] == -9999.0) { break; }
			//}



		}
		else { break; }
	}

	snprintf(buf2, sizeof(buf2), "Finished removing duplicates in distance, elevation and slope data!!\n");
	DisplayMessage(buf2);

}



/*
** caltrapzarea()
**
** Calculates the area of a trapozoid shape.
** Four inputs are required. In this application:
** 1. the x axis value (elevation, distance or slope), 
**    will be used as height of the shape.
** 2. the y axis value (percentage calculated)
**    will be used as the top (x) and bottom (x+1).
*/
float App::caltrapzarea(float olu1, float olu2, float perlu1, float perlu2)
{
	float traparea = (olu2 - olu1)*(perlu1 + perlu2) / (float)2;
	return traparea;
}




/*
** callwli()
**
** This function calculates the lorenz curve data, incluging the 
** data points and the area under each curve.
**
*/
App::Ludata *App::callwli()
{

	char buf2[512];
	snprintf(buf2, sizeof(buf2), "Calculating curve areas for distance, elevation and slope data!!\n");
	DisplayMessage(buf2);
	
	//Ludataarray;
	// Here, the elevation array will only have one value for one 
	// land use, which will be the lwli value. It will be accumulated
	// during the loop.
	Ludata *templudata = new Ludata;

	

	for (int luidx = 0; luidx < MAX_LUIDS; luidx++)
	{
		// Initialize the array
		if (allsrcsinklus[luidx] == -9999) { break; }
		else
		{
			templudata->elevarray[luidx] = new float;
			memset(templudata->elevarray[luidx], 0, sizeof(float) * 1);
			//templudata->elevarray[luidx][0] = 0.0;
 			
			templudata->slopearray[luidx] = new float;
			memset(templudata->slopearray[luidx], 0, sizeof(float) * 1);
			//templudata->slopearray[luidx][0] = 0.0;
			
			templudata->distarray[luidx] = new float;
			memset(templudata->distarray[luidx], 0, sizeof(float) * 1);
			//templudata->distarray[luidx][0] = 0.0;
			
		}

		// Initialize the counter
		templudata->ludtctrarray[luidx] = 1;
		templudata->finaldistctr[luidx] = templudata->ludtctrarray[luidx];
		templudata->finalelevctr[luidx] = templudata->ludtctrarray[luidx];
		templudata->finalslpctr[luidx] = templudata->ludtctrarray[luidx];

		// Initialize the luno
		templudata->luno = allsrcsinklus[luidx];
	}


	for (int luidx = 0; luidx < MAX_LUIDS; luidx++)
	{
		if (allsrcsinklus[luidx] != -9999)
		{
			//printf("\n..Landuse...%d.....\n", allsrcsinklus[luidx]);
			// Here, we use the final counter from the perludata.
			// This has been updated during the removal of duplicates.
			
			// While calculating the area, there is one special case, which indeed
			// happened during the test. Sometimes, there might be only 1 cell
			// for a land use. When this happened, the area calculation will fail
			// because we are calculating a triangle or trapezoid area.
			// For the accumulated area, if there is only 1 value, the area
			// will be zero since there is no accumulated area under the curve.
			if (perludata->finalelevctr[luidx] == 1) {
				templudata->elevarray[luidx][0] = 0.0;
			}
			else 
			{
				//printf("\n..final percounter...%d.....\n", perludata->finalelevctr[luidx]);
				// Remember here, the array of rawludata->elevarray and 
				// perludata->elevarray for each land use were sorted
				// descendingly. After the final value, it will be -9999.
				// 
				for (int index = perludata->finalelevctr[luidx]-1; index >=0; index--)
				{
					// Here, we are looping through each value in the array 
					// (elevation, distance, slope) for each land use.
					// We need a function to calculate the area for each step.
					// check the final value numbers
					//printf("__.%f__", rawludata->elevarray[luidx][index]);
					//printf("__.%f__\n", perludata->elevarray[luidx][index]);

					templudata->elevarray[luidx][0] = templudata->elevarray[luidx][0]+ caltrapzarea(
										rawludata->elevarray[luidx][index],
										rawludata->elevarray[luidx][index + 1],
										perludata->elevarray[luidx][index],
										perludata->elevarray[luidx][index + 1]);
					//printf("__.%f__\n", templudata->elevarray[luidx][0]);
				}
			}
			
			if (perludata->finaldistctr[luidx] == 1) {
				templudata->distarray[luidx][0] = 0.0;
			}
			else
			{
				for (int index = perludata->finaldistctr[luidx] - 1; index >= 0; index--)
				{
					// Here, we are looping through each value in the array 
					// (elevation, distance, slope) for each land use.
					// We need a function to calculate the area for each step.
					// check the final value numbers
					//printf("__.%f__", rawludata->distarray[luidx][index]);
					//printf("__.%f__\n", perludata->distarray[luidx][index]);


					templudata->distarray[luidx][0] += caltrapzarea(
						rawludata->distarray[luidx][index],
						rawludata->distarray[luidx][index + 1],
						perludata->distarray[luidx][index],
						perludata->distarray[luidx][index + 1]);
				}
			}

			if (perludata->finalslpctr[luidx] == 1) {
				templudata->slopearray[luidx][0] = 0.0;
			}
			else
			{
				for (int index = perludata->finalslpctr[luidx] - 1; index >= 0; index--)
				{
					// Here, we are looping through each value in the array 
					// (elevation, distance, slope) for each land use.
					// We need a function to calculate the area for each step.
					// check the final value numbers
					templudata->slopearray[luidx][0] += caltrapzarea(
						rawludata->slopearray[luidx][index],
						rawludata->slopearray[luidx][index + 1],
						perludata->slopearray[luidx][index],
						perludata->slopearray[luidx][index + 1]);
				}
			}
		}
		else { break; }
	}

	snprintf(buf2, sizeof(buf2), "Finished calculating curve areas for distance, elevation and slope data!!\n");
	DisplayMessage(buf2);

	return templudata;
}


/*
** writeElevData()
**
** Write output files.
**
*/
void App::writeElevData(const char *file)
{
	char buf2[512];
	snprintf(buf2, sizeof(buf2), "Writing output data for elevation!!\n");
	DisplayMessage(buf2);

	FILE *fp = fopen(file, "w");
	
	if (fp)
	{
		fprintf(fp, "No duplicated data for %s\n", file);
		for (int luidx = 0; luidx < MAX_LUIDS; luidx++)
		{
			if (allsrcsinklus[luidx] != -9999)
			{
				// Here, we use the final counter from the perludata.
				// This has been updated during the removal of duplicates.
				fprintf(fp, "Value for land use NO: %d\n", allsrcsinklus[luidx]);
				for (int index = perludata->finalelevctr[luidx] - 1; index >= 1; index--)
				{
					fprintf(fp, "%f,", rawludata->elevarray[luidx][index]);
				}
				fprintf(fp, "%f\n", rawludata->elevarray[luidx][0]);
				//int index = perludata->finalslpctr[luidx]; index >= 0; index--
				// Here, we use the final counter from the perludata.
				// This has been updated during the removal of duplicates.
				fprintf(fp, "Percentage for land use NO: %d\n", allsrcsinklus[luidx]);
				for (int index = perludata->finalelevctr[luidx] - 1; index >= 1; index--)
				{
					fprintf(fp, "%f,", perludata->elevarray[luidx][index]);
				}
				fprintf(fp, "%f\n", perludata->elevarray[luidx][0]);
			}
			else { break; }
		}
	}
	
	fclose(fp);

	snprintf(buf2, sizeof(buf2), "Finished writing output data for elevation!!\n");
	DisplayMessage(buf2);

}

/*
** writeDistData()
**
** Write output files.
**
*/
void App::writeDistData(const char *file)
{
	char buf2[512];
	snprintf(buf2, sizeof(buf2), "Writing output data for Distance!!\n");
	DisplayMessage(buf2);

	FILE *fp = fopen(file, "w");

	if (fp)
	{
		fprintf(fp, "No duplicated data for %s\n", file);
		for (int luidx = 0; luidx < MAX_LUIDS; luidx++)
		{
			if (allsrcsinklus[luidx] != -9999)
			{
				// Here, we use the final counter from the perludata.
				// This has been updated during the removal of duplicates.
				fprintf(fp, "Value for land use NO: %d\n", allsrcsinklus[luidx]);
				for (int index = perludata->finaldistctr[luidx] - 1; index >= 1; index--)
				{
					fprintf(fp, "%f,", rawludata->distarray[luidx][index]);
				}
				fprintf(fp, "%f\n", rawludata->distarray[luidx][0]);
				//int index = perludata->finalslpctr[luidx]; index >= 0; index--
				// Here, we use the final counter from the perludata.
				// This has been updated during the removal of duplicates.
				fprintf(fp, "Percentage for land use NO: %d\n", allsrcsinklus[luidx]);
				for (int index = perludata->finaldistctr[luidx] - 1; index >= 1; index--)
				{
					fprintf(fp, "%f,", perludata->distarray[luidx][index]);
				}
				fprintf(fp, "%f\n", perludata->distarray[luidx][0]);
			}
			else { break; }
		}
	}

	fclose(fp);

	snprintf(buf2, sizeof(buf2), "Finished writing output data for Distance!!\n");
	DisplayMessage(buf2);
}


/*
** writeSlpData()
**
** Write output files.
**
*/
void App::writeSlpData(const char *file)
{
	char buf2[512];
	snprintf(buf2, sizeof(buf2), "Writing output data for Slope!!\n");
	DisplayMessage(buf2);


	FILE *fp = fopen(file, "w");

	if (fp)
	{
		fprintf(fp, "No duplicated data for %s\n", file);
		for (int luidx = 0; luidx < MAX_LUIDS; luidx++)
		{
			if (allsrcsinklus[luidx] != -9999)
			{
				// Here, we use the final counter from the perludata.
				// This has been updated during the removal of duplicates.
				fprintf(fp, "Value for land use NO: %d\n", allsrcsinklus[luidx]);
				for (int index = perludata->finalslpctr[luidx] - 1; index >= 1; index--)
				{
					fprintf(fp, "%f,", rawludata->slopearray[luidx][index]);
				}
				fprintf(fp, "%f\n", rawludata->slopearray[luidx][0]);
				//int index = perludata->finalslpctr[luidx]; index >= 0; index--
				// Here, we use the final counter from the perludata.
				// This has been updated during the removal of duplicates.
				fprintf(fp, "Percentage for land use NO: %d\n", allsrcsinklus[luidx]);
				for (int index = perludata->finalslpctr[luidx] - 1; index >= 1; index--)
				{
					fprintf(fp, "%f,", perludata->slopearray[luidx][index]);
				}
				fprintf(fp, "%f\n", perludata->slopearray[luidx][0]);
			}
			else { break; }
		}
	}

	fclose(fp);

	snprintf(buf2, sizeof(buf2), "Finished writing output data for Slope!!\n");
	DisplayMessage(buf2);

}


/*
** writeLwliData()
**
** Write Lwli files.
**
*/
void App::writeLwliData(const char *file)
{
	char buf2[512];
	snprintf(buf2, sizeof(buf2), "Writing output data for Lorenz curve!!\n");
	DisplayMessage(buf2);

	FILE *fp = fopen(file, "w");

	if (fp)
	{
		fprintf(fp, "Area under lorenz curve\n");
		fprintf(fp, "Landuse, Area_Elevation, Area_Distance, Area_Slope\n");
		for (int luidx = 0; luidx < MAX_LUIDS; luidx++)
		{
			if (allsrcsinklus[luidx] != -9999)
			{
				// Here, we use the final counter from the perludata.
				// This has been updated during the removal of duplicates.
				fprintf(fp, "Landuse_%d, ", allsrcsinklus[luidx]);
				fprintf(fp, "%f, %f, %f\n", 
							lwlis->elevarray[luidx][0],
							lwlis->distarray[luidx][0],
							lwlis->slopearray[luidx][0]);

			}
			else { break; }
		}
	}

	fclose(fp);

	snprintf(buf2, sizeof(buf2), "Finished writing output data for Lorenz curve!!\n");
	DisplayMessage(buf2);
}



/*
** writeOutputs()
**
** Write output files.
**
*/
void App::writeOutputs()
{
	// Write elevation outputs
	writeElevData("elev_dataperc.txt");
	writeDistData("dist_dataperc.txt");
	writeSlpData("slp_dataperc.txt");

	writeLwliData("LurenzCurveAreas.txt");

}




/*
** calAreaPercOverws()
**
** This function calculates the area of each land use
** over the watershed area. This will be achieved by
** count the total number of cells in the watershed
** and those in each of the sink and source landuse.
**
*/

void App::calAreaPercOverws()
{
	
	// input for this function will be
	// allsrcsinklus
	// asclu
	// Three counters will be needed:

	int totalluctr;
	int *allluctr;

	totalluctr = 0;

	// Initiate the counter values
	allluctr = new int[MAX_LUIDS];
	if (allluctr == NULL)
	{
		fatalError("Out of memory in calAreaPercOverws()");
	}
	memset(allluctr, 0, sizeof(int)*MAX_LUIDS);
	
	for (int lid = 0; lid < MAX_LUIDS; lid++) {
		allluctr[lid] = 0;
	}

	for (int index = 0; index<rows*cols; index++)
	{
		if (asclu[index] != -9999)
		{
			//printf("lu, counter ...%d.... %d....\n", asclu[index], totalluctr);
			totalluctr = totalluctr + 1;
		}

		for (int i = 0; i < MAX_LUIDS; i++)
		{
			if (allsrcsinklus[i] == -9999)
			{
				break;
			}
			else if (asclu[index] == allsrcsinklus[i])
			{
				allluctr[i] = allluctr[i] +1;
				//printf("lu, counter ...%d.... %d....\n", asclu[index], allluctr[i]);
			}
		}

	}



	// Then these will be written into a file
	char buf2[512];
	snprintf(buf2, sizeof(buf2), "Writing percentage of area for each land use over watershed area!\n");
	DisplayMessage(buf2);


	FILE *fp = fopen("luareaperc.txt", "w");

	if (fp)
	{
		fprintf(fp, "Percentage of area for each land use over watershed area\n");
		fprintf(fp, "Landuse, Total_cells, Percentage\n");

		for (int luidx = 0; luidx < MAX_LUIDS; luidx++)
		{
			if (allsrcsinklus[luidx] == -9999)
			{
				break;
			}
			else
			{
				fprintf(fp, "%d, %d, %f\n", 
					allsrcsinklus[luidx],
					allluctr[luidx],
					(float)allluctr[luidx]/(float)totalluctr);
			}
		}
	}

	fclose(fp);

	snprintf(buf2, sizeof(buf2), "Finished writing percentage of area for each land use over watershed area!\n");
	DisplayMessage(buf2);
}





/*
** readGisAsciiFiles()
**
** Reads in the grid files that are required. 
**
*/
void App::readGisAsciiFiles()
{
	for (int i = 0; i<MAX_ROWS; i++)
	{
		validRows[i] = 1;
	}

	// Read in the ascii files
	// Added by Qingyu Feng June 2020 
	// The wsbdy was included to identify the watershed boundary
	// Originally lu was used but lu some times
	// only show the extent of the watershed boundary
	ascwsbdy = readArcviewIntWs("wsBdyws.asc");

	asclu = readArcviewIntLu("luws.asc");
	
	// The order of calling function matters since we use the 
	// xllcorner and yllcorner for the calculation of coord  rows  
	// and columns. Here, we would like to use that for dist2strm.asc
	ascelev = readArcviewFloat("demws.asc");
	ascslope = readArcviewFloat("sd8ws.asc");
	ascpflowdir = readArcviewIntPFlow("pflowdir.asc");
	ascdistostrm = readArcviewFloat("dist2strm.asc");

	// Read the coord.txt
	coordlns = readCoord("coord.txt");

	// Get unique Land use values within watershed
	allsrcsinklus = getUniqueLu();

	// The next step is to update the ascdist2toolt based on
	// the pflowdir, diststrm and coord.
	ascdistoolt = getDistToOlt();

	// put the value into corresponding lu
	rawludata = asc2ludata();


	//int tidx = 0;
	//for (int rid = 0; rid < rows; rid++)
	//{
	//	tidx = rid * cols;
	//	std::cout << "Row: " << rid << std::endl;
	//	for (int cid = 0; cid < cols; cid++)
	//	{
	//		if (ascwsbdy[tidx] != -9999) {
	//			printf("...%d..%d..%f....\n", rid, cid, ascslope[tidx]);
	//			printf("...%d..%d..%f....\n", rid, cid, ascelev[tidx]);
	//			printf("...%d..%d..%f....\n", rid, cid, ascpflowdir[tidx]);
	//			printf("...%d..%d..%f....\n", rid, cid, ascdistoolt[tidx]);
	//		}
	//		tidx++;
	//	}
	//}

	// Testing Blocks for coordln
	//int tidx2 = 0;
	//for (int rid2 = 0; rid2 < 200; rid2++)
	//{
	//	tidx2 = rid2 * 7;
	//	std::cout << "Row: " << rid2 << std::endl;
	//	for (int cid2 = 0; cid2 < 7; cid2++)
	//	{
	//		if (coordlns[tidx2] != -9999.0) {
	//			printf(".wsbdy..%d..%d..%lf....\n", rid2, cid2, coordlns[tidx2]);
	//		}
	//		tidx2++;
	//	}
	//}


}














/*
** SortCalpercent()
**
** Sort rawdata and calculate the percent of the datas.
**
*/
void App::SortCalpercent()
{

	// Sort the data, will be stored in the orderludata
	sortludata();

	// Percent will be put into the perludata
	perludata = calperludata();

	// Remove duplicates 
	removeDuplicates();
}



void App::CalLWLI()
{
	// After processing the data, the next step is to 
	// calculate the Trapezoidal area of the data.
	// The equation is:
	// f(x) = delta x/2(y0 + 2*y1 + 2*y2 + ... + 2*yn-1 + yn)
	// C++ does not have a function to make the graphs.
	// I will use python to create the graphs.

	// Required:
	// Array of the data: orderludata, perludata.
	lwlis = callwli();
	
	// After calculation, it is time to write the 
	// output into text files.
	// Outputs to be written:
	// 1. elevation (lu1 orvalue, lu1 pertvalue, ...)
	// 2. distance (lu1 orvalue, lu1 pertvalue, ...)
	// 3. slope (lu1 orvalue, lu1 pertvalue, ...)
	// 4. Final Lwli values
	writeOutputs();

}


