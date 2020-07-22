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
-------------------------------------------------------------------------------------------------------------
** File statement:
** This app class is the class that contain all of the functions
** used in the process.
**
**
-------------------------------------------------------------------------------------------------------------
*/

// This is start of the header guard.
// ADD_H can be any unique name.  
// By convention, we use the name of the header file.
#ifndef APP_H
#define APP_H


// This is the content of the .h file, 
//	which is where the declarations go
// int add(int x, int y); // function prototype for add.h -- don't forget the semicolon!
// This is the end of the header guard

// Declare class
class App;

#define MAX_ROWS   10000000
#define MAX_COL_BYTES 10000000
#define MAX_LUIDS 100
// Declare class
class App;

#include <vector>
#include <string>
using namespace std;

// Declare class
class App;


// Declare class
class App;

// Define class
class App
{
public:
	// Constructor and distructor 
	// of the app for manage memories
	App();
	~App();

	// Variables to store the data read from
	// the asc files
	int *asclu;
	int *ascwsbdy;
	float *ascelev;
	float *ascslope;
	float *ascdistoolt;
	float *ascdistostrm;
	float *coordlns;
	int *ascpflowdir;
	int *allsrcsinklus;


	void readGisAsciiFiles();

	// Define a structure to store all of the datas
	struct Ludata
	{
		int luno;
		// Stores all data
		float *elevarray[MAX_LUIDS];
		float *slopearray[MAX_LUIDS];
		float *distarray[MAX_LUIDS];
		// Stores the counter
		int ludtctrarray[MAX_LUIDS];

		// stores the final number of each data value
		int finalelevctr[MAX_LUIDS];
		int finaldistctr[MAX_LUIDS];
		int finalslpctr[MAX_LUIDS];

	};

	Ludata *rawludata;
	Ludata *perludata;
	Ludata *lwlis;


	void SortCalpercent();

	void CalLWLI();

	void calAreaPercOverws();


	// Clean memory after running
	void cleanMemory();

private:

	// Functions for reading input data
	// from text files
	int *readArcviewIntWs(const char *file);
	float *readArcviewFloat(const char *file);
	int *readArcviewIntLu(const char *file);
	int *readArcviewIntPFlow(const char *file);

	float *readCoord(const char *file);

	float *getDistToOlt();

	float getDistStm2OltinCoord(int cellrow, int cellcol);
	float getStrmCellandDistinCoord(int cellrow, int cellcol);


	int *getUniqueLu();

	Ludata *asc2ludata();
	void sortludata();
	Ludata *calperludata();
	Ludata *callwli();

	void removeDuplicates();

	float caltrapzarea(float olu1, float olu2, float perlu1, float perlu2);

	void writeOutputs();
	void writeElevData(const char *file);
	void writeDistData(const char *file);
	void writeSlpData(const char *file);
	void writeLwliData(const char *file);

	

	int rows;
	int cols;
	float cellsize;
	float noData;
	int noDataLu;
	int noDataWs;
	int noDataPFlow;


	int validRows[MAX_ROWS];
	float xllcorner, yllcorner;


};











#endif

