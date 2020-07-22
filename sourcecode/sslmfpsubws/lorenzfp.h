/*  sslmfpsub header

  Qingyu Feng
  RCEES
  June 29, 2020
  
*/

/*  Copyright (C) 2020  Qingyu Feng

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
email: qyfeng18@rcees.ac.cn
*/

#include <algorithm>

using namespace std;

#ifndef SSLMFPSUB_H
#define SSLMFPSUB_H


// Define a structure to store all of the datas
struct Ludata
{
	int thisluno;
	int thissubno;

	vector <float> elevarr;
	vector <float> distarr;
	vector <float> slparr;

	vector <float> elevperarr;
	vector <float> distperarr;
	vector <float> slpperarr;

	float elevarea;
	float distarea;
	float slparea;

	int totallucells;
	int totalsubcells;
	float luareaper;
};



int totallunos;

class HashTableEntry {
public:
	int k;
	Ludata *v;
	HashTableEntry(int k, Ludata *v) {
		this->k = k;
		this->v = v;
	}
};

class HashMapTable {
private:
	HashTableEntry **t;
public:
	HashMapTable() {
		t = new HashTableEntry *[totallunos];
		for (int i = 0; i < totallunos; i++) {
			t[i] = NULL;
		}
	}

	int HashFunc(int k) {
		return k % totallunos;
	}

	void Insert(int k, Ludata *v) {
		int h = HashFunc(k);
		while (t[h] != NULL && t[h]->k != k) {
			h = HashFunc(h + 1);
		}
		if (t[h] != NULL)
			delete t[h];
		t[h] = new HashTableEntry(k, v);
	}

	int SearchKey(int k) {
		int h = HashFunc(k);
		//printf("search key: h: %d, k: %d\n", h, k);
		while (t[h] != NULL && t[h]->k != k) {
			h = HashFunc(h + 1);
		}

		if (t[h] == NULL)
			return -1;
		else
			return t[h]->v->thisluno;
	}

	Ludata *getLuData(int k) {
		int h = HashFunc(k);
		while (t[h] != NULL && t[h]->k != k) {
			h = HashFunc(h + 1);
		}

		return t[h]->v;
	}


	void Remove(int k) {
		int h = HashFunc(k);
		while (t[h] != NULL) {
			if (t[h]->k == k)
				break;
			h = HashFunc(h + 1);
		}
		if (t[h] == NULL) {
			cout << "No Element found at key " << k << endl;
			return;
		}
		else {
			delete t[h];
		}
		cout << "Element Deleted" << endl;
	}


	// Sort the value in each lu data
	void sortHashElevDistSlp() {
		for (int i = 0; i < totallunos; i++) {
			if (t[i] != NULL)
			{
				sort(t[i]->v->elevarr.begin(), t[i]->v->elevarr.end());
				sort(t[i]->v->distarr.begin(), t[i]->v->distarr.end());
				sort(t[i]->v->slparr.begin(), t[i]->v->slparr.end());
			}
		}
	}


	// Calculate the percent values
	// This function calculates the perctntage over total numbers
	// It represents the area of one cell covering the subarea.
	void calPercElevDistSlp() {
		for (int i = 0; i < totallunos; i++) {
			if (t[i] != NULL)
			{
				float disttp, elevtp, slptp;
				for (std::vector<float>::size_type di = 0; di != t[i]->v->distarr.size(); di++) {
					disttp = (float)((float)(di+1.0)*100.0/ t[i]->v->distarr.size());
					t[i]->v->distperarr.push_back(disttp);
				}
				for (std::vector<float>::size_type ei = 0; ei != t[i]->v->elevarr.size(); ei++) {
					elevtp = (float)((float)(ei + 1.0)*100.0 / t[i]->v->distarr.size());
					t[i]->v->elevperarr.push_back(elevtp);
				}
				for (std::vector<float>::size_type si = 0; si != t[i]->v->slparr.size(); si++) {
					slptp = (float)((float)(si + 1.0)*100.0 / t[i]->v->slparr.size());
					t[i]->v->slpperarr.push_back(slptp);
				}

			}
		}
	}

	/*
	** countTotalCellinSub
	*/
	void countTotalCellinSub() {

		int totalcell = 0;
		
		for (int i = 0; i < totallunos; i++) {
			if (t[i] != NULL)
			{
				// Initialize the total cell value
				t[i]->v->totalsubcells = 0;
				t[i]->v->totallucells = 0;
				totalcell += t[i]->v->distarr.size();
				//printf("totalcell : %d, size: %d\n", totalcell, t[i]->v->distarr.size());
			}
		}
		//printf("final talcell : %d\n", totalcell);
		for (int i = 0; i < totallunos; i++) {
			if (t[i] != NULL)
			{
				t[i]->v->totalsubcells = totalcell;
				t[i]->v->luareaper = (float)t[i]->v->distarr.size()/(float)totalcell;
				t[i]->v->totallucells = t[i]->v->distarr.size();
				//printf("final tal land use cell : %d\n", t[i]->v->totallucells);
			}
		}
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
	float caltrapzarea(float olu1, float olu2, float perlu1, float perlu2)
	{
		float traparea = (olu2 - olu1)*(perlu1 + perlu2) / (float)2;
		return traparea;
	}



	// calAccArea
	// This function calculates the area covered by each
	// curve of elevper, distper, and slpper of each land use.
	void calAccAreaLuElevDistSlp() {
		for (int i = 0; i < totallunos; i++) {
			if (t[i] != NULL)
			{		
				//Initialize the area values
				t[i]->v->elevarea = 0.0;
				t[i]->v->distarea = 0.0;
				t[i]->v->slparea = 0.0;

				if (t[i]->v->distarr.size() == 1) {
					t[i]->v->distarea = 0.0;
				}else
				{
					for (int di = 1; di < t[i]->v->distarr.size(); di++) 
					{
						t[i]->v->distarea += caltrapzarea(
							t[i]->v->distarr[di-1],
							t[i]->v->distarr[di],
							t[i]->v->distperarr[di-1],
							t[i]->v->distperarr[di]);
					}
				}

				if (t[i]->v->elevarr.size() == 1) {
					t[i]->v->elevarea = 0.0;
				}
				else
				{
					for (int ei = 1; ei < t[i]->v->elevarr.size(); ei++)
					{
						t[i]->v->elevarea += caltrapzarea(
							t[i]->v->elevarr[ei-1],
							t[i]->v->elevarr[ei],
							t[i]->v->elevperarr[ei-1],
							t[i]->v->elevperarr[ei]);
					}
				}

				if (t[i]->v->slparr.size() == 1) {
					t[i]->v->slparea = 0.0;
				}
				else
				{
					for (int si = 1; si < t[i]->v->slparr.size(); si++)
					{
						t[i]->v->slparea += caltrapzarea(
							t[i]->v->slparr[si-1],
							t[i]->v->slparr[si],
							t[i]->v->slpperarr[si-1],
							t[i]->v->slpperarr[si]);
					}
				}
			}
		}
	}

	bool compare_float(float x, float y, float epsilon = 0.001f) {
		if (fabs(x - y) < epsilon)
			return true; //they are same
		return false; //they are not same
	}


	// Remove Duplicates in array
	// This function removes the duplicates within the 
	// elev, distance and slope array.
	void removeVecDuplicates() {
		for (int i = 0; i < totallunos; i++) {
			if (t[i] != NULL)
			{
				//float rmslpv, rmslpp;
				//printf("Landuse No: first: %d\n", t[i]->v->thisluno);
				for (int di = 1; di < t[i]->v->distarr.size(); di++) {
					if (t[i]->v->distarr[di-1] == t[i]->v->distarr[di])
					{
						t[i]->v->distarr.erase(t[i]->v->distarr.begin() + di - 1);
						t[i]->v->distperarr.erase(t[i]->v->distperarr.begin() + di -1);
						di--;
					}
				}
				for (int ei = 1; ei < t[i]->v->elevarr.size() ; ei++) {
					if (t[i]->v->elevarr[ei-1] == t[i]->v->elevarr[ei])
					{
						t[i]->v->elevarr.erase(t[i]->v->elevarr.begin() + ei-1);
						t[i]->v->elevperarr.erase(t[i]->v->elevperarr.begin() + ei-1);
						ei--;
					}
				}
				for (int si = 1; si < t[i]->v->slparr.size(); si++) {
					if (compare_float(t[i]->v->slparr[si-1], t[i]->v->slparr[si]))
					{
						//printf("Value slope to be removed: %f\n", t[i]->v->slpperarr[si - 1]);
						//rmslpp = t[i]->v->slpperarr[si - 1];
						t[i]->v->slparr.erase(t[i]->v->slparr.begin() + (si-1));
						t[i]->v->slpperarr.erase(t[i]->v->slpperarr.begin() + (si - 1));
						//t[i]->v->slpperarr.erase(std::remove(t[i]->v->slpperarr.begin(),
						//	t[i]->v->slpperarr.end(), rmslpp), t[i]->v->slpperarr.end());
						si--;
					}
				}
			}
		}
	}


	// display the values and vectors within the hash table 
	void displayHash() {
		for (int i = 0; i < totallunos; i++) {
			if (t[i] != NULL)
			{
				int subnot = t[i]->v->thissubno;
				printf("subno: %d...", subnot);
				int lunot = t[i]->v->thisluno;
				printf("luno: %d...", lunot);


				int slppersize= t[i]->v->slpperarr.size();
				printf("slppersize: %d...\n", slppersize);
				//int elevsize = t[i]->v->elevperarr.size();
				//printf("elevsize: %d\n", elevsize);
				//int distsize = t[i]->v->distperarr.size();
				//printf("elevsize: %d\n", distsize);

				//float areap = t[i]->v->luareaper;
				//printf("areaper: %f\n", areap);
				//vector <float> temp;
				//temp = t[i]->v->slpperarr;
				//displayFloatVector(temp);
				//temp = t[i]->v->elevarr;
				//displayFloatVector(temp);
				//temp = t[i]->v->slparr;
				//displayFloatVector(temp);
			}
		}
	}

	void displayFloatVector(vector <float> vec) {
		for (auto & it : vec)
		{
			printf("%f\n", it);
		
		}
	}


	~HashMapTable() {
		for (int i = 0; i < totallunos; i++) {
			if (t[i] != NULL)
				delete t[i];
			delete[] t;
		}
	}

	bool checkMissedSubNo() {
		// Judge whether this subarea is missing
		bool isMissingSubNo;
		int hasValueCount = 0;
		for (int i = 0; i < totallunos; i++) {
			if (t[i] != NULL) {
				hasValueCount = hasValueCount + 1;
			}
		}

		if (hasValueCount > 0) { isMissingSubNo = false; }
		else { isMissingSubNo = true; }

		return isMissingSubNo;

		
	}

};





#endif
