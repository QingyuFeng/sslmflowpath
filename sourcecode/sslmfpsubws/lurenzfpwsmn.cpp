/*  sslmfpsub

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


#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "commonLib.h"

int lorenzSub(char *pfile,
	char *distfile,
	char *wsfile,
	char *lufile,
	char *elevfile,
	char *slpfile,
	char *lzpvajs);

int main(int argc, char **argv)
{
	char pfile[MAXLN], distfile[MAXLN], wsfile[MAXLN], lufile[MAXLN], elevfile[MAXLN], slpfile[MAXLN];
	char lzpvajs[MAXLN]; // , lzareafile[MAXLN];
	int err, nmain, i;

	if (argc < 2)
	{
		printf("Error: To run this program, use either the Simple Usage option or\n");
		printf("the Usage with Specific file names option\n");
		goto errexit;
	}

	else if (argc > 2)
	{
		i = 1;
		//	printf("You are running %s with the Specific File Names Usage option.\n", argv[0]);
	}
	else {
		i = 2;
		//	printf("You are running %s with the Simple Usage option.\n", argv[0]);
	}

	while (argc > i)
	{
		if (strcmp(argv[i], "-p") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(pfile, argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if (strcmp(argv[i], "-d2wo") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(distfile, argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if (strcmp(argv[i], "-ws") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(wsfile, argv[i]);
				i++;
			}
			else goto errexit;
		}

		else if (strcmp(argv[i], "-lu") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(lufile, argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if (strcmp(argv[i], "-elev") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(elevfile, argv[i]);
				i++;
			}
			else goto errexit;
		}
		else if (strcmp(argv[i], "-slp") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(slpfile, argv[i]);
				i++;
			}
			else goto errexit;
		}

		else if (strcmp(argv[i], "-lzjsw") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(lzpvajs, argv[i]);
				i++;
			}
			else goto errexit;
		}

		/*else if (strcmp(argv[i], "-lzaw") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(lzareafile, argv[i]);
				i++;
			}
			else goto errexit;
		}*/

		else
		{
			goto errexit;
		}
	}

	if (argc == 2)
	{
		nameadd(pfile, argv[1], "p");
		nameadd(distfile, argv[1], "d2wo");
		nameadd(wsfile, argv[1], "ws");
		nameadd(lufile, argv[1], "lu");
		nameadd(elevfile, argv[1], "elev");
		nameadd(slpfile, argv[1], "slp");
		nameadd(lzpvajs, argv[1], "lzpvaws.json");
		//nameadd(lzareafile, argv[1], "lzareasws.txt");
	}

	if (err = lorenzSub(pfile, distfile, wsfile, lufile, elevfile, slpfile, lzpvajs) != 0)
		printf("Lorenz curve for watershed error %d\n", err);


	return 0;

errexit:
	printf("Simple Usage:\n %s <basefilename>\n", argv[0]);
	printf("Usage with specific file names:\n %s -p <pfile>\n", argv[0]);
	printf("-dist <distfile> -ws <wsfile>  -lu <lufile>\n");
	printf(" -elev <elevfile>  -slp <slpfile> \n");
	printf("<basefilename> is the name of the base digital elevation model\n");
	printf("<pfile> is the d8 flow direction input file.\n");
	printf("<distfile> is the distance to watershed outlet raster input file.\n");
	printf("<wsfile> is the watershed boundary raster input file.\n");
	printf("<lufile> is the land use raster input file.\n");
	printf("<elevfile> is the elevation raster input file.\n");
	printf("<slpfile> is the sd8 slope raster input file.\n");
	printf("<lzpvajs> is the lorenz point area json output file.\n");
	//printf("<lzareafile> is the lorenz area text output file.\n");
	printf("The following are appended to the file names\n");
	printf("before the files are opened:\n");
	printf("p      D8 flow directions (input)\n");
	printf("d2wo    distance to watershed outlet raster file (Input)\n");
	printf("ws     watershed boundary raster file (Input)\n");
	printf("lu   landuse raster file (input)\n");
	printf("elev   elevation raster file (input)\n");
	printf("slp   slope raster file (input)\n");
	printf("lzjsw   lorenz for watershed json file (output)\n");
	
	exit(0);
}
