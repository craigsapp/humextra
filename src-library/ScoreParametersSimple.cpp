//
// Copyright 2002 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Feb 10 19:42:45 PST 2002
// Last Modified: Tue Mar 24 18:40:22 PST 2009
// Filename:      ...sig/src/sigInfo/ScoreParametersSimple.cpp
// Web Address:   http://sig.sapp.org/src/sigInfo/ScoreParametersSimple.cpp
// Syntax:        C++ 
//
// Description:   Base class for SCORE musical objects.
//

#include "ScoreParametersSimple.h"

#include <string.h>

#ifndef OLDCPP
   using namespace std;
#endif



///////////////////////////////
//
// ScoreParametersSimple::ScoreParametersSimple --
//

ScoreParametersSimple::ScoreParametersSimple(void) { 
   fixedParameters.setSize(0);
   keyParameters.setSize(0);
}


ScoreParametersSimple::ScoreParametersSimple(ScoreParametersSimple& item) { 
   fixedParameters = item.fixedParameters;
   clearKeyParams();
   keyParameters.setSize(item.keyParameters.getSize());
   keyParameters.setSize(0);
   int i;
   for (i=0; i<item.keyParameters.getSize(); i++) {
      appendKeyParameter(item.keyParameters[i].getBase());
   }
}



///////////////////////////////
//
// ScoreParametersSimple::clear --
//

void ScoreParametersSimple::clear(void) {
   fixedParameters.setSize(0);
   clearKeyParams();
}



///////////////////////////////
//
// ScoreParametersSimple::getValue -- starting offset at 1.
//

float ScoreParametersSimple::getValue(int index) { 
   float output = 0.0;
   if (index < fixedParameters.getSize()) {
      output = fixedParameters[index];
   }
   return output;
}



///////////////////////////////
//
// ScoreParametersSimple::getPValue -- starting offset at 1.
//

float ScoreParametersSimple::getPValue(int index) { 
   float output = 0.0;
   if (index-1 < fixedParameters.getSize()) {
      output = fixedParameters[index-1];
   }
   return output;
}



///////////////////////////////
//
// ScoreParametersSimple::setValue -- starting offset at 0.
//

void  ScoreParametersSimple::setValue(int index, float value) { 
   if (index < fixedParameters.getSize()) {
      fixedParameters[index] = value;
   } else {
      int oldsize = fixedParameters.getSize();
      fixedParameters.setSize(index+1);
      for (int i=oldsize; i<index+1; i++) {
         fixedParameters[i] = 0.0;
      }
      fixedParameters[index] = value;
   }
}



///////////////////////////////
//
// ScoreParametersSimple::setPValue -- starting offset at 1.
//

void  ScoreParametersSimple::setPValue(int index, float value) { 
   if (index-1 < fixedParameters.getSize()) {
      fixedParameters[index-1] = value;
   } else {
      int oldsize = fixedParameters.getSize();
      fixedParameters.setSize(index);
      for (int i=oldsize; i<index; i++) {
         fixedParameters[i] = 0.0;
      }
      fixedParameters[index-1] = value;
   }
}



///////////////////////////////
//
// ScoreParametersSimple::operator= --
//

ScoreParametersSimple& ScoreParametersSimple::operator=(ScoreParametersSimple& anItem) {
   if (&anItem == this) {
      return *this;
   }
   fixedParameters = anItem.fixedParameters;
   keyParameters = anItem.keyParameters;

   return *this;
}



//////////////////////////////
//
// ScoreParametersSimple::getFixedSize --
//

int ScoreParametersSimple::getFixedSize(void) {
   return fixedParameters.getSize();
}



//////////////////////////////
//
// ScoreParametersSimple::setFixedSize --
//

void ScoreParametersSimple::setFixedSize(int value) {
   fixedParameters.setSize(value);
}



//////////////////////////////
//
// ScoreParametersSimple::getKeySize --
//

int ScoreParametersSimple::getKeySize(void) {
   return keyParameters.getSize();
}



//////////////////////////////
//
// ScoreParametersSimple::print --
//

void ScoreParametersSimple::print(void) {
   int i;
   for (i=0; i<fixedParameters.getSize(); i++) {
      cout << fixedParameters[i] << " ";
   }
   cout << endl;
   for (i=0; i<keyParameters.getSize(); i++) {
      cout << keyParameters[i].getBase() << endl;
   }
}



//////////////////////////////
//
// ScoreParametersSimple::setAllocSize -- set the allocation size of the 
//   class if the current allocation size is smaller.
//

void ScoreParametersSimple::setAllocSize(int aSize) {
   if (aSize > fixedParameters.getAllocSize()) {
      int oldsize = fixedParameters.getSize();
      fixedParameters.setSize(aSize);
      fixedParameters.setSize(oldsize);
   }
}



//////////////////////////////////////////////////////////////////////////
//
// Private Functions
// 


//////////////////////////////
//
// ScoreParametersSimple::clearKeyParams --
//

void ScoreParametersSimple::clearKeyParams(void) {
   keyParameters.setSize(0);
}


////////////////////////////////
//
// ScoreParametersSimple::appendKeyParameter --
//

void ScoreParametersSimple::appendKeyParameter(const char* string) {
    int length = (int)strlen(string);
    int index = keyParameters.getSize();
    keyParameters.setSize(keyParameters.getSize()+1);
    keyParameters[index].setSize(length+1);
    strcpy(keyParameters[index].getBase(), string);
}




// md5sum: 28ccec826d946c540a9937f0129ce915 ScoreParametersSimple.cpp [20050403]
