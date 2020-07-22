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

  
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "commonLib.h"

int subindexmap(char *wsfile, 
	char *subidxjson,
	char *subidxmap
	);

int main(int argc,char **argv)
{
   char wsfile[MAXLN];
   char subidxjson[MAXLN]; 
   char subidxmap[MAXLN];
   int err,nmain, i;
   
   if(argc < 2)
    {  
       printf("Error: To run this program, use either the Simple Usage option or\n");
	   printf("the Usage with Specific file names option\n");
	   goto errexit;
    }
   
   else if(argc > 2)
	{
		i = 1;
	//	printf("You are running %s with the Specific File Names Usage option.\n", argv[0]);
	}
	else {
		i = 2;
	//	printf("You are running %s with the Simple Usage option.\n", argv[0]);
	}

	while(argc > i)
	{
		if (strcmp(argv[i], "-ws") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(wsfile, argv[i]);
				i++;
			}
			else goto errexit;
		}

		
		else if (strcmp(argv[i], "-ijs") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(subidxjson, argv[i]);
				i++;
			}
			else goto errexit;
		}

		else if (strcmp(argv[i], "-ims") == 0)
		{
			i++;
			if (argc > i)
			{
				strcpy(subidxmap, argv[i]);
				i++;
			}
			else goto errexit;
		}

		else 
		{
			goto errexit;
		}
	}   

	if(argc == 2)
	{
		nameadd(wsfile, argv[1], "ws");
		nameadd(subidxjson,argv[1],"finalsslmidxsub.json");
		nameadd(subidxmap, argv[1], "ims");
	}

    if(err= subindexmap(wsfile, subidxjson, subidxmap) != 0)
        printf("Creating map for subarea sslm index error %d\n",err);


	return 0;

	errexit:
	   printf("Simple Usage:\n %s <basefilename>\n",argv[0]);
       printf("Usage with specific file names:\n %s -ws <wsfile>\n",argv[0]);
	   printf(" -ijs <subidxjson>  -ijm <subidxmap> \n");
  	   printf("<basefilename> is the name of the base sslmfp model\n");
	   printf("<wsfile> is the watershed boundary raster input file.\n");
	   printf("<subidxjson> is the sslm index json input file.\n");
	   printf("<subidxmap> is the sslmindex map for subarea output file.\n");
       printf("The following are appended to the file names\n");
       printf("before the files are opened:\n");
	   printf("ws     watershed boundary raster file (Input)\n");
	   printf("ijs   sslm index json for subarea (input)\n");
	   printf("ims   sslm index tiff map for subarea (output)\n");
       exit(0);
} 
