/*  idxhash header

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

#ifndef IDSHASH_H
#define IDSHASH_H

int totalsubnos;

class HashTableEntry {
public:
	int k;
	float v;
	HashTableEntry(int k, float v) {
		this->k = k;
		this->v = v;
	}
};
class HashMapTable {
private:
	HashTableEntry **t;
public:
	HashMapTable() {
		t = new HashTableEntry *[totalsubnos];
		for (int i = 0; i < totalsubnos; i++) {
			t[i] = NULL;
		}
	}

	int HashFunc(int k) {
		return k % totalsubnos;
	}

	void Insert(int k, float v) {
		int h = HashFunc(k);
		while (t[h] != NULL && t[h]->k != k) {
			h = HashFunc(h + 1);
		}
		if (t[h] != NULL)
			delete t[h];
		t[h] = new HashTableEntry(k, v);
	}

	float SearchKey(int k) {
		int h = HashFunc(k);
		while (t[h] != NULL && t[h]->k != k) {
			h = HashFunc(h + 1);
		}
		if (t[h] == NULL)
			return (float)-1;
		else
			return t[h]->v;
	}


	float getValue(int k) {
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

	// display the values and vectors within the hash table 
	void displayHash() {
		for (int i = 0; i < totalsubnos; i++) {
			if (t[i] != NULL)
			{
				int lunot = t[i]->v;
				printf("subno: %d, subidx: %f\n", t[i]->k, t[i]->v);
			}
		}
	}

	~HashMapTable() {
		for (int i = 0; i < totalsubnos; i++) {
			if (t[i] != NULL)
				delete t[i];
			//delete[] t;
		}
	}
};




#endif
