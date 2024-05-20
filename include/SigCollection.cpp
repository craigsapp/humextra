//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Feb  5 19:42:53 PST 1997
// Last Modified: Wed Apr 23 22:08:34 GMT-0800 1997
// Last Modified: Fri Sep 14 15:50:52 PDT 2001 added last() function
// Last Modified: Wed Mar 30 14:00:16 PST 2005 Fixed for compiling in GCC 3.4
// Last Modified: Fri Jun 12 22:58:34 PDT 2009 renamed SigCollection class
// Last Modified: Fri Aug 10 09:17:03 PDT 2012 added reverse()
// Filename:      ...sig/maint/code/base/SigCollection/SigCollection.cpp
// Web Address:   http://sig.sapp.org/src/sigBase/SigCollection.cpp
// vim:           ts=3:nowrap
// Syntax:        C++
//
// Description:   A dynamic array which can grow as necessary.
//                This class can hold any type of item, but the
//                derived Array class is specifically for collections
//                of numbers.
//

#ifndef _SIGCOLLECTION_CPP_INCLUDED
#define _SIGCOLLECTION_CPP_INCLUDED

#include "SigCollection.h"

#include <cstdlib>
#include <iostream>


using namespace std;

//////////////////////////////
//
// SigCollection::SigCollection --
//

template<class type>
SigCollection<type>::SigCollection(void) {
	this->m_allocSize = 0;
	this->m_size = 0;
	this->m_array = NULL;
	this->m_allowGrowthQ = 0;
	this->m_growthAmount = 8;
	this->m_maxSize = 0;
}

template<class type>
SigCollection<type>::SigCollection(int arraySize) {
	// this->m_array = new type[arraySize];
	this->m_array = std::make_unique<type[]>(arraySize);

	this->m_size = arraySize;
	this->m_allocSize = arraySize;
	this->m_allowGrowthQ = 0;
	this->m_growthAmount = arraySize;
	this->m_maxSize = 0;
}



template<class type>
SigCollection<type>::SigCollection(int arraySize, type *aSigCollection) {
	this->m_size = arraySize;
	this->m_allocSize = arraySize;
	// this->m_array = new type[this->m_size];
	this->m_array = std::make_unique<type[]>(this->m_size);
	for (int i=0; i<this->m_size; i++) {
		this->m_array[i] = aSigCollection[i];
	}
	this->m_growthAmount = arraySize;
	this->m_allowGrowthQ = 0;
	this->m_maxSize = 0;
}


//template<class type>
//SigCollection<type>::SigCollection(SigCollection<type>& aSigCollection) {
//	this->m_size = aSigCollection.m_size;
//	this->m_allocSize = this->m_size;
//	this->m_array = new type[this->m_size];
//	for (int i=0; i<this->m_size; i++) {
//		this->m_array[i] = aSigCollection.m_array[i];
//	}
//	this->m_allowGrowthQ = aSigCollection.m_allowGrowthQ;
//	this->m_growthAmount = aSigCollection.m_growthAmount;
//	this->m_maxSize = aSigCollection.m_maxSize;
//}


template<class type>
SigCollection<type>::SigCollection(const SigCollection<type>& aSigCollection) {
	this->m_size = aSigCollection.m_size;
	this->m_allocSize = this->m_size;
	this->m_array = std::make_unique<type[]>(this->m_size);
	for (int i=0; i<this->m_size; i++) {
		this->m_array[i] = aSigCollection.m_array[i];
	}
	this->m_allowGrowthQ = aSigCollection.m_allowGrowthQ;
	this->m_growthAmount = aSigCollection.m_growthAmount;
	this->m_maxSize = aSigCollection.m_maxSize;
}



//////////////////////////////
//
// SigCollection::~SigCollection --
//

//template<class type>
//SigCollection<type>::~SigCollection() {
//	if (this->getAllocSize() != 0) {
//		delete [] this->m_array;
//		this->m_array = NULL;
//	}
//}

template<class type>
SigCollection<type>::~SigCollection() {
   // No need to manually delete[] with std::unique_ptr
   // It will automatically release the allocated memory when it goes out of scope
   // this->m_array = NULL; // No need for this line
}



//////////////////////////////
//
// SigCollection::allowGrowth --
//	default value: status = 1
//

template<class type>
void SigCollection<type>::allowGrowth(int status) {
	if (status == 0) {
		this->m_allowGrowthQ = 0;
	} else {
		this->m_allowGrowthQ = 1;
	}
}



//////////////////////////////
//
// SigCollection::append --
//

template<class type>
void SigCollection<type>::append(type& element) {
	if (this->m_size == this->getAllocSize()) {
		this->grow();
	}
	this->m_array[this->m_size] = element;
	this->m_size++;
}

template<class type>
void SigCollection<type>::appendcopy(type element) {
	if (this->m_size == this->getAllocSize()) {
		this->grow();
	}
	this->m_array[this->m_size] = element;
	this->m_size++;
}

template<class type>
void SigCollection<type>::append(type *element) {
	if (this->m_size == this->getAllocSize()) {
		this->grow();
	}
	this->m_array[m_size] = *element;
	this->m_size++;
}



//////////////////////////////
//
// SigCollection::grow --
// 	default parameter: growamt = -1
//

//template<class type>
//void SigCollection<type>::grow(long growamt) {
//   this->m_allocSize += growamt > 0 ? growamt : this->m_growthAmount;
//   if (this->m_maxSize != 0 && this->getAllocSize() > this->m_maxSize) {
//      std::cerr << "Error: Maximum size allowed for array exceeded." << std::endl;
//      exit(1);
//   }
//
//   type *temp = new type[this->getAllocSize()];
//   for (int i=0; i<this->m_size; i++) {
//      temp[i] = this->m_array[i];
//   }
//   delete [] this->m_array;
//   this->m_array = temp;
//}

template<class type>
void SigCollection<type>::grow(long growamt) {
	if (growamt <= 0) {
		growamt = this->m_growthAmount;
	}
	if (growamt <= 0) {
		growamt = 8;
	}
	if (this->m_size > this->m_allocSize) {
		cerr << "Error: m_size " << this->m_size << " is larger than m_allocSize: " << this->m_allocSize << endl;
		exit(1);
	}
	std::unique_ptr<type[]> temp(new type[this->getAllocSize() + growamt]);
	for (int i=0; i<this->m_size; i++) {
		temp[i] = this->m_array[i];
	}
	this->m_allocSize += growamt;
	// The old array will be automatically deleted when temp goes out of scope
	this->m_array = std::move(temp);
}

   

//////////////////////////////
//
// SigCollection::pointer --
//

template<class type>
type* SigCollection<type>::pointer(void) {
	return this->m_array;
}



//////////////////////////////
//
// SigCollection::getBase --
//

template<class type>
type* SigCollection<type>::getBase(void) const {
	// return this->m_array;
   return this->m_array.get();
}



//////////////////////////////
//
// SigCollection::getAllocSize --
//

template<class type>
long SigCollection<type>::getAllocSize(void) const {
	return this->m_allocSize;
}



//////////////////////////////
//
// SigCollection::getSize --
//

template<class type>
long SigCollection<type>::getSize(void) const {
	return this->m_size;
}



//////////////////////////////
//
// SigCollection::last --
//      default value: index = 0
//

template<class type>
type& SigCollection<type>::last(int index) {
	return this->m_array[getSize()-1-abs(index)];
}



//////////////////////////////
//
// SigCollection::setAllocSize --
//

template<class type>
void SigCollection<type>::setAllocSize(long aSize) {
	if (aSize < this->getSize()) {
		std::cerr << "Error: cannot set allocated size smaller than actual size.";
		std::cerr << std::endl;
		exit(1);
	}

	if (aSize <= this->getAllocSize()) {
		this->shrinkTo(aSize);
	} else {
		this->grow(aSize-this->getAllocSize());
		this->m_size = aSize;
	}
}



//////////////////////////////
//
// SigCollection::setGrowth --
//	default parameter: growth = -1
//

template<class type>
void SigCollection<type>::setGrowth(long growth) {
	if (growth > 0) {
		this->m_growthAmount = growth;
	}
}



//////////////////////////////
//
// SigCollection::setSize --
//

template<class type>
void SigCollection<type>::setSize(long newSize) {
	if (newSize <= this->getAllocSize()) {
		this->m_size = newSize;
	} else {
		this->grow(newSize-this->getAllocSize());
		this->m_size = newSize;
	}
}


////////////////////////////////////////////////////////////////////////////////
//
// SigCollection operators
//

//////////////////////////////
//
// SigCollection::operator[] --
//

template<class type>
type& SigCollection<type>::operator[](int elementIndex) {
	if (this->m_allowGrowthQ && elementIndex == this->m_size) {
		if (this->m_size == this->getAllocSize()) {
		   this->grow();
		}
		this->m_size++;
	} else if ((elementIndex >= this->m_size) || (elementIndex < 0)) {
		std::cerr << "Error: accessing invalid array location "
		     << elementIndex
		     << " Maximum is " << this->m_size-1 << std::endl;
		exit(1);
	}
	return this->m_array[elementIndex];
}



//////////////////////////////
//
// SigCollection::operator[] const --
//

template<class type>
type SigCollection<type>::operator[](int elementIndex) const {
	if ((elementIndex >= this->m_size) || (elementIndex < 0)) {
		std::cerr << "Error: accessing invalid array location "
		     << elementIndex
		     << " Maximum is " << this->m_size-1 << std::endl;
		exit(1);
	}
	return this->m_array[elementIndex];
}



//////////////////////////////
//
// SigCollection::shrinkTo --
//

template<class type>
void SigCollection<type>::shrinkTo(long aSize) {
	if (aSize < this->getSize()) {
		exit(1);
	}

	// type *temp = new type[aSize];
   std::unique_ptr<type[]> temp = std::make_unique<type[]>(aSize);

	for (int i=0; i<this->m_size; i++) {
		temp[i] = this->m_array[i];
	}
	// delete [] this->m_array;
	// this->m_array = temp;
   this->m_array = std::move(temp);

	this->m_allocSize = aSize;
	if (this->m_size > this->m_allocSize) {
		this->m_size = this->m_allocSize;
	}
}



//////////////////////////////
//
// SigCollection::increase -- equivalent to setSize(getSize()+addcount)
//

template<class type>
int SigCollection<type>::increase(int addcount) {
	if (addcount > 0) {
		this->setSize(this->getSize() + addcount);
	}
	return this->getSize();
}



//////////////////////////////
//
// SigCollection::decrease -- equivalent to setSize(getSize()-subcount)
//

template<class type>
int SigCollection<type>::decrease(int subcount) {
	if (this->getSize() - subcount <= 0) {
		this->setSize(0);
	} else if (subcount > 0) {
		this->setSize(this->getSize() - subcount);
	}
	return this->getSize();
}



//////////////////////////////
//
// SigCollection::reverse -- reverse the order of items in the list.
//

template<class type>
void SigCollection<type>::reverse(void) {
	int i;
	type tempval;
	int mirror;
	int pivot = this->getSize() / 2;
	for (i=0; i<pivot; i++) {
		tempval = this->m_array[i];
		mirror = this->getSize() - i - 1;
		this->m_array[i] = this->m_array[mirror];
		this->m_array[mirror] = tempval;
	}
}


#endif  /* _SIGCOLLECTION_CPP_INCLUDED */



// md5sum: e5d20829760eaa880e5753116883784c SigCollection.cpp [20050403]
