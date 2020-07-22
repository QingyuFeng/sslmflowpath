#  Makefile for building executables on a UNIX System.
#
#    David Tarboton, Dan Watson, Jeremy Neff
#    Utah State University
#    May 23, 2010
#
#  Copyright (C) 2010  David Tarboton, Utah State University
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License 
#  version 2, 1991 as published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  A copy of the full GNU General Public License is included in file 
#  gpl.html. This is also available at:
#  http://www.gnu.org/copyleft/gpl.html
#  or from:
#  The Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
#  Boston, MA  02111-1307, USA.
#
#  If you wish to use or incorporate this program (or parts of it) into 
#  other software that does not meet the GNU General Public License 
#  conditions contact the author to request permission.
#  David G. Tarboton  
#  Utah State University 
#  8200 Old Main Hill 
#  Logan, UT 84322-8200 
#  USA 
#  http://www.engineering.usu.edu/dtarb/ 
#  email:  dtarb@usu.edu 
#
#
#  This software is distributed from http://hydrology.usu.edu/taudem/



#SHAPEFILES includes all files in the shapefile library
#These should be compiled using the makefile in the shape directory

#OBJFILES includes classes, structures, and constants common to all files
OBJFILES = commonLib.o tiffIO.o

D8DIST2SUBOLT = dist2suboltmn.o dist2subolt.o $(OBJFILES)
D8DIST2WSOLT = dist2wsoltmn.o dist2wsolt.o $(OBJFILES)
SSLMFPSUB = sslmfpsubmn.o sslmfpsub.o $(OBJFILES)
SSLMFPWSV2 = sslmfpwsv2mn.o sslmfpwsv2.o $(OBJFILES)

#The following are compiler flags common to all building rules
#CC = mpic++
CC = mpicxx
#CFLAGS=-g -Wall -DDEBUG -std=c++11
CFLAGS=-O2 -std=c++11
LARGEFILEFLAG= -D_FILE_OFFSET_BITS=64
#INCDIRS=-I/usr/lib/openmpi/include -I/usr/include/gdal
INCDIRS=`gdal-config --cflags`
INCDIRS+=`nc-config --cflags`
#LIBDIRS=-lgdal
#LDLIBS=-L/usr/local/lib -lgdal
LDLIBS=`gdal-config --libs`
LDLIBS+=`nc-config --libs`

#Rules: when and how to make a file
all : ../../bin/dist2subolt ../../bin/dist2wsolt ../../bin/sslmfpsub ../../bin/sslmfpws clean

../../bin/dist2wsolt : $(D8DIST2WSOLT)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(D8DIST2WSOLT) $(LDLIBS) $(LDFLAGS)

../../bin/sslmfpwsv2 : $(SSLMFPWSV2)
    $(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(SSLMFPWSV2) $(LDLIBS) $(LDFLAGS)

../../bin/sslmfpsub : $(SSLMFPSUB)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(SSLMFPSUB) $(LDLIBS) $(LDFLAGS)

../../bin/dist2subolt : $(D8DIST2SUBOLT)
	$(CC) $(CFLAGS) -o $@ $(LIBDIRS) $(D8DIST2SUBOLT) $(LDLIBS) $(LDFLAGS)


#Inference rule - states a general rule for compiling .o files
%.o : %.cpp
	$(CC) $(CFLAGS) $(INCDIRS) -c $< -o $@

clean :
	rm *.o 


