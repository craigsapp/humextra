//
// Copyright 1998-2004 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon May 18 13:43:47 PDT 1998
// Last Modified: Wed Nov 29 10:24:29 PST 2000 Split from HumdrumFile class
// Last Modified: Wed Mar 21 17:38:23 PST 2001 Added spine width feature
// Last Modified: Thu Nov 15 12:38:49 PST 2001 Added getTrackExInterp
// Last Modified: Sat May  8 12:57:53 PDT 2004 Fix complicated spine tracing
// Last Modified: Wed Jun  2 15:19:57 PDT 2004 Fix fix in spine trace ! lines
// Last Modified: Fri Apr 24 15:01:35 PDT 2009 Fixed contructor with char*
// Last Modified: Wed Apr 29 17:14:44 PDT 2009 Fixed *x bug in dot analysis
// Last Modified: Mon May  4 12:11:13 PDT 2009 Fixed new? bug in dot analysis
// Last Modified: Fri Jun 12 22:58:34 PDT 2009 Renamed SigCollection class
// Last Modified: Mon Jun 22 15:03:44 PDT 2009 Added Humdrum/Http URIs
// Last Modified: Wed Sep  2 21:48:23 PDT 2009 Fixed problem with *x again
// Last Modified: Wed May 12 22:29:42 PDT 2010 Added embedded PDF parsing
// Last Modified: Mon Feb 14 08:14:53 PST 2011 Added VTS functions
// Last Modified: Tue Apr 24 16:37:34 PDT 2012 Added jrp:// URI
// Last Modified: Tue Dec 11 17:23:04 PST 2012 Added fileName, segmentLevel
// Last Modified: Mon Apr  1 16:44:32 PDT 2013 Added printNonemptySegmentLevel
// Last Modified: Sun Mar 13 17:10:48 PDT 2016 Switched to STL
// Filename:      ...sig/src/sigInfo/HumdrumFileBasic.cpp
// Web Address:   http://sig.sapp.org/src/sigInfo/HumdrumFileBasic.cpp
// Syntax:        C++
//
// Description:   Basic parsing functions for handling a Humdrum data file.
//                Special musical parsing of the Humdrum file is done in
//                the inherited class HumdrumFile.
//

#include "Convert.h"
#include "HumdrumFileBasic.h"
#include "PDFFile.h"
#include "CheckSum.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cctype>

#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

char HumdrumFileBasic::empty[1] = {0};


///////////////////////////////////////////////////////////////////////////
//
// HumdrumFileAddress class
//


//////////////////////////////
//
// HumdrumFileAddress::HumdrumFileAddress --
//

HumdrumFileAddress::HumdrumFileAddress(void) {
   // do nothing (address has uninitialized concents)
}

HumdrumFileAddress::HumdrumFileAddress(int aLine) {
    address[0] = 0;
    address[1] = aLine;
    address[2] = 0;
    address[3] = 0;
}

HumdrumFileAddress::HumdrumFileAddress(int aLine, int aField) {
    address[0] = 0;
    address[1] = aLine;
    address[2] = aField;
    address[3] = 0;
}

HumdrumFileAddress::HumdrumFileAddress(int aLine, int aField, int aSubfield) {
    address[0] = 0;
    address[1] = aLine;
    address[2] = aField;
    address[3] = aSubfield;
}

HumdrumFileAddress::HumdrumFileAddress(const HumdrumFileAddress& anAddress) {
    address[0] = 0;
    address[1] = anAddress.address[0];
    address[2] = anAddress.address[1];
    address[3] = anAddress.address[2];
}



//////////////////////////////
//
// HumdrumFileAddress::~HumdrumFileAddress --
//

HumdrumFileAddress::~HumdrumFileAddress() {
   // do nothing
}



//////////////////////////////
//
// HumdrumFileAddress::line --  Return the line in the Humdrum file.
//

int& HumdrumFileAddress::line(void)    { return address[1]; }
int& HumdrumFileAddress::getLine(void) { return address[1]; }
int& HumdrumFileAddress::row(void)     { return address[1]; }
int& HumdrumFileAddress::getRow(void)  { return address[1]; }
int& HumdrumFileAddress::i(void)       { return address[1]; }



//////////////////////////////
//
// HumdrumFileAddress::field -- Returns the column on the line.
//

int& HumdrumFileAddress::field(void)     { return address[2]; }
int& HumdrumFileAddress::getField(void)  { return address[2]; }
int& HumdrumFileAddress::column(void)    { return address[2]; }
int& HumdrumFileAddress::getColumn(void) { return address[2]; }
int& HumdrumFileAddress::col(void)       { return address[2]; }
int& HumdrumFileAddress::getCol(void)    { return address[2]; }
int& HumdrumFileAddress::j(void)         { return address[2]; }



//////////////////////////////
//
// HumdrumFileAddress::subfield --
//

int& HumdrumFileAddress::subfield(void)    { return address[3]; }
int& HumdrumFileAddress::getSubfield(void) { return address[3]; }
int& HumdrumFileAddress::subtoken(void)    { return address[3]; }
int& HumdrumFileAddress::getSubtoken(void) { return address[3]; }
int& HumdrumFileAddress::k(void)           { return address[3]; }



//////////////////////////////
//
// HumdrumFileAddress::zero -- Set all address components to 0.
//

void HumdrumFileAddress::zero(void) {
   address[0] = 0;
   address[1] = 0;
   address[2] = 0;
   address[3] = 0;
}



//////////////////////////////
//
// HumdrumFileAddress::operator= --
//

HumdrumFileAddress HumdrumFileAddress::operator=(const
      HumdrumFileAddress& anAddress) {
   if (this == &anAddress) {
      return *this;
   }
   address[0] = anAddress.address[0];
   address[1] = anAddress.address[1];
   address[2] = anAddress.address[2];
   address[3] = anAddress.address[3];
   return *this;
}



//////////////////////////////
//
// HumdrumFileAddress::setLineField --
//

void HumdrumFileAddress::setLineField(int aLine, int aField) {
   line() = aLine;
   field() = aField;
}



///////////////////////////////////////////////////////////////////////////
//
// HumdrumFileBasic class
//

//////////////////////////////
//
// HumdrumFileBasic::HumdrumFileBasic
//

HumdrumFileBasic::HumdrumFileBasic(void) {
   records.reserve(100000);          // initial storage size 100000 lines
   records.resize(0);
   maxtracks = 0;
   fileName = "";
   segmentLevel = 0;
   trackexinterp.resize(0);
}


HumdrumFileBasic::HumdrumFileBasic(const HumdrumFileBasic& aHumdrumFileBasic) {
   records.reserve(100000);          // initial storage size 100000 lines
   records.resize(0);
   maxtracks = 0;
   fileName = "";
   segmentLevel = 0;
   trackexinterp.resize(0);

   *this = aHumdrumFileBasic;
}


HumdrumFileBasic::HumdrumFileBasic(const char* filename) {
   records.reserve(100000);          // initial storage size 100000 lines
   records.resize(0);
   maxtracks = 0;
   fileName = "";
   segmentLevel = 0;
   trackexinterp.resize(0);

   ifstream infile(filename, ios::in);

   if (!infile) {
      cerr << "Error: cannot open file: " << filename << endl;
      exit(1);
   }

   read(filename);
   /*
   char templine[4096];
   while (!infile.eof()) {
      infile.getline(templine, 4096);
      if (infile.eof() && (strcmp(templine, "") == 0)) {
         break;
      } else {
         appendLine(templine);
      }
   }
   analyzeSpines();
   analyzeDots();
   */
}



HumdrumFileBasic::HumdrumFileBasic(const string& filename) {
   records.reserve(100000);          // initial storage size 100000 lines
   records.resize(0);
   maxtracks = 0;
   fileName = "";
   segmentLevel = 0;
   trackexinterp.resize(0);

   ifstream infile(filename.data(), ios::in);

   if (!infile) {
      cerr << "Error: cannot open file: " << filename << endl;
      exit(1);
   }

   read(filename);
   /*
   char templine[4096];
   while (!infile.eof()) {
      infile.getline(templine, 4096);
      if (infile.eof() && (strcmp(templine, "") == 0)) {
         break;
      } else {
         appendLine(templine);
      }
   }
   analyzeSpines();
   analyzeDots();
   */
}



////////////////////////////////////////
//
// HumdrumFileBasic::setAllocation -- set the expected number of
//     lines from the input file.
//

void HumdrumFileBasic::setAllocation(int allocation) {
   if (allocation <= 0) {
      return;
   }
   if (allocation > 10000000) {
      return;
   }
   int currentsize = records.size();
   if (currentsize < allocation) {
      records.reserve(allocation);
   }
}


//////////////////////////////
//
// HumdrumFileBasic::~HumdrumFileBasic
//

HumdrumFileBasic::~HumdrumFileBasic() {
   clear();
}



//////////////////////////////
//
// HumdrumFileBasic::analyzeSpines --
//

void HumdrumFileBasic::analyzeSpines(void) {
   privateSpineAnalysis();
}



//////////////////////////////
//
// HumdrumFileBasic::analyzeDots --
//

void HumdrumFileBasic::analyzeDots(void) {
   privateDotAnalysis();
}



//////////////////////////////
//
// HumdrumFileBasic::appendLine  -- adds a line to a humdrum file
//

void HumdrumFileBasic::appendLine(const char* aLine) {
   HumdrumRecord* aRecord;
   aRecord = new HumdrumRecord;
   aRecord->setLine(aLine);
   records.push_back(aRecord);
}


void HumdrumFileBasic::appendLine(HumdrumRecord& aRecord) {
   HumdrumRecord *tempRecord;
   tempRecord = new HumdrumRecord;
   *tempRecord = aRecord;
   records.push_back(tempRecord);
}


void HumdrumFileBasic::appendLine(HumdrumRecord* aRecord) {
   appendLine(*aRecord);
}



//////////////////////////////
//
// HumdrumFileBasic::clear -- removes all lines from the humdrum file
//

void HumdrumFileBasic::clear(void) {
   int i;
   for (i=0; i<(int)records.size(); i++) {
      if (records[i] != NULL) {
         // delete [] records[i];
         delete records[i];
         records[i] = NULL;
      }
   }
   records.resize(0);
   maxtracks = 0;
   fileName = "";
   segmentLevel = 0;
   for (i=0; i<(int)trackexinterp.size(); i++) {
      if (trackexinterp[i] != NULL) {
         delete [] trackexinterp[i];
         trackexinterp[i] = NULL;
      }
   }
   trackexinterp.resize(0);
}



//////////////////////////////
//
// changeField -- Change the contents of a token in the Humdrum file.
//

void HumdrumFileBasic::changeField(HumdrumFileAddress& add,
      const char* newField) {
   (*this)[add.line()].changeField(add.field(), newField);
}



//////////////////////////////
//
// HumdrumFileBasic::setFilename -- set the filename for the HumdrumFile
//     (used by HumdrumStream class management).
//

void HumdrumFileBasic::setFilename(const char* filename) {
   fileName = filename;
}



//////////////////////////////
//
// HumdrumFileBasic::getFilename -- returns the filename of the HumdrumFile.
//    Returns "" if no filename set (such as when reading from cin).
//

const char* HumdrumFileBasic::getFilename(void) {
   return fileName.c_str();
}



//////////////////////////////
//
// HumdrumFileBasic::printSegmentLabel --
//

ostream& HumdrumFileBasic::printSegmentLabel(ostream& out) {
   out << "!!!!SEGMENT";
   const char* filename = getFilename();
   int segment = getSegmentLevel();
   if (segment != 0) {
      if (segment < 0) {
         out << segment;
      } else {
         out << "+" << segment;
      }
   }
   out << ": " << filename << endl;
   return out;
}

//////////////////////////////
//
// HumdrumFileBasic::printNonemptySegmentLabel --
//

ostream& HumdrumFileBasic::printNonemptySegmentLabel(ostream& out) {
   if (strlen(getFilename()) > 0) {
      printSegmentLabel(out);
   }
   return out;
}



//////////////////////////////
//
// HumdrumFileBasic::getSegmentLevel -- return the segment level
//

int HumdrumFileBasic::getSegmentLevel(void) {
   return segmentLevel;
}



//////////////////////////////
//
// HumdrumFileBasic::setSegmentLevel -- return the segment level
//

void HumdrumFileBasic::setSegmentLevel(int level) {
   segmentLevel = level;
}



//////////////////////////////
//
// HumdrumFileBasic::extract --
//

HumdrumFileBasic HumdrumFileBasic::extract(int aField) {
   int fieldNumber = aField;
   HumdrumRecord aRecord;
   HumdrumFileBasic output;
   for (int i=0; i<getNumLines(); i++) {
      switch(records[i]->getType()) {
         case E_humrec_data:
         case E_humrec_data_comment:
         case E_humrec_data_kern_measure:
         case E_humrec_data_interpretation:
            if (aField < 0) {
               fieldNumber = records[i]->getFieldCount() + aField;
               if (fieldNumber < 0) {
                  fieldNumber = 0;
               }
            } else {
               fieldNumber = aField;
            }
            aRecord.setLine((*records[i])[fieldNumber]);
            aRecord.setExInterp(0, records[i]->getExInterpNum(fieldNumber));
            output.appendLine(aRecord);
            break;
         case E_humrec_empty:
            output.appendLine(records[i]);
            break;
         case E_humrec_none:
         case E_humrec_global_comment:
         default:
            output.appendLine(records[i]);
      }
   }

   return output;
}



//////////////////////////////
//
// HumdrumFileBasic::getNonNullAddress --
//

void HumdrumFileBasic::getNonNullAddress(HumdrumFileAddress& add) {
   HumdrumFileBasic& afile = *this;
   if (strcmp(afile[add], ".") != 0) {
      return;
   }
   int i = afile[add.line()].getDotLine(add.field());
   int j = afile[add.line()].getDotField(add.field());
   add.line() = i;
   add.field() = j;
}



///////////////////////////////
//
// HumdrumFileBasic::getDotValue --
//

const char* HumdrumFileBasic::getDotValue(int index, int spinei) {
   int line  = (*this)[index].getDotLine(spinei);
   int spine = (*this)[index].getDotSpine(spinei);

   if (line < 0 || spine < 0) {
      return HumdrumFileBasic::empty;
   } else {
      return (*this)[line][spine];
   }
}



//////////////////////////////
//
// HumdrumFileBasic::getMaxTracks --
//

int HumdrumFileBasic::getMaxTracks(void) {
   return maxtracks;
}



//////////////////////////////
//
// HumdrumFileBasic::getTrackExInterp --
//

const char* HumdrumFileBasic::getTrackExInterp(int track) {
   return trackexinterp[track-1];
}


//////////////////////////////
//
// HumdrumFileBasic::getTracksByExInterp -- return a list of track numbers
//     which have the matching exclusive interpretation type.
//

int HumdrumFileBasic::getTracksByExInterp(vector<int>& tracks,
      const char* exinterp) {
   int maxsize = getMaxTracks();
   tracks.reserve(maxsize);
   tracks.resize(0);
   int i;
   for (i=0; i<(int)trackexinterp.size(); i++) {
      if (strcmp(trackexinterp[i], exinterp) == 0) {
         tracks.push_back(i);
         tracks.back()++;
      } else if (strcmp(trackexinterp[i]+2, exinterp) == 0) {
         tracks.push_back(i);
         tracks.back()++;
      }
   }

   return tracks.size();
}



//////////////////////////////
//
// HumdrumFileBasic::getKernTracks --
//

int HumdrumFileBasic::getKernTracks(vector<int>& tracks) {
   return getTracksByExInterp(tracks, "**kern");
}



//////////////////////////////
//
// HumdrumFileBasic::getType -- returns the enumerated type of the record.
//

int HumdrumFileBasic::getType(int index) {
   return (*this)[index].getType();
}



//////////////////////////////
//
// HumdrumFileBasic::getLine -- returns the character string of the
//	specified line
//

const char* HumdrumFileBasic::getLine(int index) {
   if (index >= getNumLines()) {
      cerr << "Error: maximum line index is: " << getNumLines() - 1
           << ", but you asked for line index: " << index << endl;
      exit(1);
   }

   return records[index]->getLine();
}



//////////////////////////////
//
// HumdrumFileBasic::getBibValue -- add an optional parameter later which
//    selects the nth occurance of the bibliographic key.
//

const char* HumdrumFileBasic::getBibValue(char* buffer, const char* key) {
   int i;
   buffer[0] = '\0';
   HumdrumFileBasic& hfile = *this;

   char newkey[1024] = {0};
   if (strncmp(key, "!!!", 3) == 0) {
      strcpy(newkey, key);
   } else {
      strcpy(newkey, "!!!");
      strcat(newkey, key);
   }

   for (i=0; i<hfile.getNumLines(); i++) {
      if (!hfile[i].isBibliographic()) {
         continue;
      }
      if (strncmp(hfile[i][0], newkey, strlen(newkey)) != 0) {
         continue;
      }
      hfile[i].getBibValue(buffer);
      break;
   }

   return buffer;
}



//////////////////////////////
//
// HumdrumFileBasic::getNumLines  -- returns the number of lines
//	in the HumdrumFileBasic
//

int HumdrumFileBasic::getNumLines(void) {
   return records.size();
}



//////////////////////////////
//
// HumdrumFileBasic::getRecord -- returns the record of the specified line
//

HumdrumRecord& HumdrumFileBasic::getRecord(int index) {
   if (index >= getNumLines()) {
      cerr << "Error: maximum line index is: " << getNumLines() - 1
           << ", but you asked for line index: " << index << endl;
      exit(1);
   }

   return *records[index];
}


//////////////////////////////
//
// HumdrumFileBasic::getSegmentCount -- will eventually return the
//    number of independent musical segments (which each start with
//    /^\*\*/ and end with /^\*-/).
//
// also later, add a function called getSegmentStart(int index) which
// returns the staring index in the file for the segment.
//

int HumdrumFileBasic::getSegmentCount(void) {
   return 1;
}



//////////////////////////////
//
// HumdrumFileBasic::getSpineCount -- returns the record of the specified line
//

int HumdrumFileBasic::getSpineCount(int index) {
   int type = ((*this)[index]).getType();
   if ((type & E_humrec_data) == E_humrec_data) {
      return ((*this)[index]).getFieldCount();
   } else {
      return 0;
   }
}



//////////////////////////////
//
// HumdrumFileBasic::operator=
//

HumdrumFileBasic& HumdrumFileBasic::operator=(const HumdrumFileBasic& aFile) {
   // don't copy onto self
   if (&aFile == this) {
      return *this;
   }

   // delete any current contents
   int i;
   for (i=0; i<(int)records.size(); i++) {
      delete records[i];
      records[i] = NULL;
   }

   records.resize(aFile.records.size());
   for (i=0; i<(int)aFile.records.size(); i++) {
      records[i] = new HumdrumRecord;
      *(records[i]) = *(aFile.records[i]);
   }

   maxtracks = aFile.maxtracks;

   for (i=0; i<(int)trackexinterp.size(); i++) {
      if (trackexinterp[i] != NULL) {
         delete [] trackexinterp[i];
         trackexinterp[i] = NULL;
      }
   }
   trackexinterp.resize(aFile.trackexinterp.size());
   int length;
   for (i=0; i<(int)aFile.trackexinterp.size(); i++) {
      length = strlen(aFile.trackexinterp[i]);
      trackexinterp[i] = new char[length+1];
      strcpy(trackexinterp[i], aFile.trackexinterp[i]);
   }
   return *this;
}



//////////////////////////////
//
// HumdrumFileBasic::read -- read in a Humdrum file.
//

void HumdrumFileBasic::read(const string& filename) {
   read(filename.data());
}


void HumdrumFileBasic::read(const char* filename) {
#ifdef USING_URI
   if (strstr(filename, "://") != NULL) {
      if (strncmp(filename, "http://", strlen("http://")) == 0) {
         readFromHttpURI(filename);
         return;
      }
      if (strncmp(filename, "jrp://", strlen("jrp://")) == 0) {
         readFromJrpURI(filename);
         return;
      }
      if (strncmp(filename, "humdrum://", strlen("humdrum://")) == 0) {
         readFromHumdrumURI(filename);
         return;
      }
      if (strncmp(filename, "hum://", strlen("hum://")) == 0) {
         readFromHumdrumURI(filename);
         return;
      }
      if (strncmp(filename, "h://", strlen("h://")) == 0) {
         readFromHumdrumURI(filename);
         return;
      }
   }
#endif

   int i;
   for (i=0; i<(int)records.size(); i++) {
      delete records[i];
      records[i] = NULL;
   }
   records.resize(0);

   ifstream infile(filename, ios::in);

   if (!infile) {
      cerr << "Error: could not open file: " << filename << endl;
      exit(1);
   }

   if (infile.peek() == '%') {
      stringstream embeddedData;
      extractEmbeddedDataFromPdf(embeddedData, infile);
      read(embeddedData);
      return;
   }

   char templine[4096];
   while (!infile.eof()) {
      infile.getline(templine, 4096, '\n');
      if (infile.eof() && (strcmp(templine, "") == 0)) {
         break;
      } else {
         appendLine(templine);
      }
   }
   analyzeSpines();
   analyzeDots();
}


void HumdrumFileBasic::read(istream& inStream) {
   char* templine;
   templine = new char[4096];
   int linecount = 0;

   if (inStream.peek() == '%') {
      stringstream embeddedData;
      extractEmbeddedDataFromPdf(embeddedData, inStream);
      read(embeddedData);
      return;
   }

   while (!inStream.eof()) {
      inStream.getline(templine, 4096);
#ifdef USING_URI
      if ((linecount++ == 0) && (strstr(templine, "://") != NULL)) {
         if (strncmp(templine, "http://", strlen("http://")) == 0) {
            readFromHttpURI(templine);
            delete [] templine;
            return;
         }
         if (strncmp(templine, "humdrum://", strlen("humdrum://")) == 0) {
            readFromHumdrumURI(templine);
            delete [] templine;
            return;
         }
         if (strncmp(templine, "hum://", strlen("hum://")) == 0) {
            readFromHumdrumURI(templine);
            delete [] templine;
            return;
         }
         if (strncmp(templine, "h://", strlen("h://")) == 0) {
            readFromHumdrumURI(templine);
            delete [] templine;
            return;
         }
      }
#endif
      if (inStream.eof() && (strcmp(templine, "") == 0)) {
         break;
      } else {
         appendLine(templine);
      }
   }
   analyzeSpines();
   analyzeDots();
   delete [] templine;
}



//////////////////////////////
//
// HumdrumFileBasic::removeNullRecords
//

HumdrumFileBasic HumdrumFileBasic::removeNullRecords(void) {
   HumdrumFileBasic output;
   int j;
   int nulltest = 1;
   for (int i=0; i<getNumLines(); i++) {
      nulltest = 1;
      switch(records[i]->getType()) {
         case E_humrec_data:
            for (j=0; j<records[i]->getFieldCount(); j++) {
               if (strcmp((*records[i])[j], ".") != 0) {
                  nulltest = 0;
                  break;
               }
            }
            if (nulltest) {
               // do nothing
            } else {
               output.appendLine(records[i]);
            }
            break;
         default:
            output.appendLine(records[i]);
      }
   }

   return output;
}



//////////////////////////////
//
// HumdrumFileBasic::operator[]
//

HumdrumRecord& HumdrumFileBasic::operator[](int index) {
   if (index >= getNumLines()) {
      cerr << "Error: maximum line index is: " << getNumLines() - 1
           << ", but you asked for line index: " << index << endl;
      exit(1);
   }

   return *records[index];
}

const char* HumdrumFileBasic::operator[](HumdrumFileAddress& add) {
   return (*this)[add.line()][add.field()];
}



//////////////////////////////
//
// HumdrumFileBasic::write -- write out a humdrum file.
//

void HumdrumFileBasic::write(const char* filename) {

   #ifndef OLDCPP
      ofstream outfile(filename, ios::out);
   #else
      ofstream outfile(filename, ios::out | ios::noreplace);
   #endif

   if (!outfile) {
      cerr << "Error: could not open file for writing: " << filename << endl;
      cerr << "Perhaps it already exists?" << endl;
      exit(1);
   }

   for (int i=0; i<(int)records.size(); i++) {
      outfile << records[i]->getLine() << endl;
   }
}


void HumdrumFileBasic::write(ostream& outStream) {
   for (int i=0; i<(int)records.size(); i++) {
      outStream << records[i]->getLine() << endl;
   }
}



///////////////////////////////////////////////////////////////////////////



//////////////////////////////
//
// operator<<
//

ostream& operator<<(ostream& out, HumdrumFileBasic& aHumdrumFileBasic) {
   aHumdrumFileBasic.printNonemptySegmentLabel(out);
   for (int i=0; i<aHumdrumFileBasic.getNumLines(); i++) {
      out << aHumdrumFileBasic[i].getLine() << '\n';
   }
   return out;
}



///////////////////////////////////////////////////////////////////////////
//
// spine analysis functions
//


///////////////////////////////
//
// HumdrumFileBasic::privateSpineAnalysis -- assign the spine paths to the
//     individual fields in each spine for the Humdrum File.
//

void HumdrumFileBasic::privateSpineAnalysis(void) {

   int init = 0;
   int spineid = 0;

   vector<char*> spineinfo;
   vector<int> exinterps;

   exinterps.reserve(100);
   exinterps.resize(0);
   exinterps.push_back(0);

   spineinfo.reserve(1000);
   spineinfo.resize(0);

   int currentwidth = 0;
   int prediction = 0;
   int length;
   int tlen = 0;
   int i, n;
   int type;
   char* tptr = NULL;
   char buffer[1024] = {0};
   char* bp;

   for (i=0; i<(int)trackexinterp.size(); i++) {
      if (trackexinterp[i] != NULL) {
         delete [] trackexinterp[i];
         trackexinterp[i] = NULL;
      }
   }
   trackexinterp.reserve(100);
   trackexinterp.resize(0);

   int linecount = getNumLines();
   for (n=0; n<linecount; n++) {
      ((*this)[n]).setLineNum(n+1);
      type = ((*this)[n]).getType();
      if (type == E_humrec_data || type == E_humrec_data_measure ||
            type == E_humrec_data_comment) {
         if (init == 0) {
            cout << (*this);
            cout << "Error on line " << n+1
                 << " of data: no starting interpretation" << endl;
            exit(1);
         }
         ((*this)[n]).copySpineInfo(spineinfo, n+1);
         currentwidth = (*this)[n].getFieldCount();
         (*this)[n].setSpineWidth(currentwidth);
      } else if (type == E_humrec_interpretation) {
         currentwidth = (*this)[n].getFieldCount();
         if (!init) {
            init = 1;
cout << "TEST " << (*this)[n] << endl;
cout << "TYPE " << (*this)[n].getType() << endl;
cout << "EXCLUSIVE " << (*this)[n].hasExclusiveQ() << endl;
            if (!((*this)[n]).hasExclusiveQ()) {
               cout << "Error on line " << n+1 << " of file: "
                    << "No starting exclusive interpretation" << endl;
               cout << "The file contains: " << endl;
               cout << (*this) << endl;
               cout << "LINE WITH MISSING INTERPRETATION:" << endl;
               cout << (*this)[n] << endl;
               exit(1);
            }
            if (spineinfo.size() != 0) {
               cout << "Error on line " << n+1 << endl;
               exit(1);
            }
            for (i=0; i<getSpineCount(n); i++) {
               if (strncmp("**", getRecord(n)[i], 2) != 0) {
                  cout << "Error on line " << n+1 << ": nonexclusive" << endl;
               }
               tlen = strlen(getRecord(n)[i]);
               tptr = new char[tlen + 1];
               strcpy(tptr, getRecord(n)[i]);
               trackexinterp.push_back(tptr);
               spineid++;
               sprintf(buffer, "%d", spineid);
               length = strlen(buffer);
               bp = new char[length + 1];
               strcpy(bp, buffer);
               spineinfo.push_back(bp);
               int value;
               value = Convert::exint.getValue(getRecord(n)[i]);
               if (spineid != (int)exinterps.size()) {
                  cout << "Error in exclusive interpretation allocation.";
                  cout << "Line: " << n+1 << endl;
                  exit(1);
               }
               if (value == E_unknown) {
                  value = Convert::exint.add(getRecord(n)[i]);
                  exinterps.push_back(value);
               } else {
                  exinterps.push_back(value);
               }
            }
            ((*this)[n]).copySpineInfo(spineinfo, n+1);
            (*this)[n].setSpineWidth(currentwidth);
         } else if (((*this)[n]).hasExclusiveQ() || ((*this)[n]).hasPathQ()) {
            prediction = predictNewSpineCount(((*this)[n]));
            (*this)[n].setSpineWidth(currentwidth);
            currentwidth = prediction;
            int w;
            int ii;
            w = n+1;
            while (w < linecount && getSpineCount(w) == 0) {
               w++;
            }
            if ((w < linecount) && (prediction != getSpineCount(w))) {
               cerr << "Error on line " << w+1 << ": "
                    << "spine count does not match:"
                    << " prediction = " << prediction
                    << " actual = " << getSpineCount(w)
                    << endl;
               cerr << "Data up to error: " << endl;
               for (ii=0; ii<w+1; ii++) {
                  cout << (*this)[ii] << endl;
               }
               exit(1);
            } else if ((w >= linecount) && prediction != 0) {
               cerr << "Error in termination of humdrum data" << endl;
            }
            ((*this)[n]).copySpineInfo(spineinfo, n+1);
            makeNewSpineInfo(spineinfo, ((*this)[n]), prediction, spineid,
                 exinterps);

            if (prediction == 0) {
               init = 0;
            }
         } else {
            // plain tandem interpretation
            if (init == 0) {
               cerr << "Error on first line of data: no starting interpretation"
                    << endl;
               exit(1);
            }
            ((*this)[n]).copySpineInfo(spineinfo, n+1);
            (*this)[n].setSpineWidth(currentwidth);
         }
      } else {
        // do nothing: global comment, bibliography information, or null line
        (*this)[n].setSpineWidth(currentwidth);
      }
   }

   // delete the contents of spineinfo
   for (i=0; i<(int)spineinfo.size(); i++) {
      if (spineinfo[i] != NULL) {
         delete [] spineinfo[i];
         spineinfo[i] = NULL;
      }
   }

   spineinfo.resize(0);

   // provide Exclusive Interpretations ownerships to the record spines

   int spineindex;
   linecount = getNumLines();
   int m;
   const char* ptr;
   for (n=0; n<linecount; n++) {
      type = ((*this)[n]).getType();
      if ((type & E_humrec_data) == E_humrec_data) {
         for (m=0; m<getSpineCount(n); m++) {
            ptr = ((*this)[n]).getSpineInfo(m);
            while (ptr[0] != '\0' && !std::isdigit(ptr[0])) {
               ptr++;
            }
            sscanf(ptr, "%d", &spineindex);
            ((*this)[n]).setExInterp(m, exinterps[spineindex]);
         }
      }
   }

   maxtracks = spineid;
}



//////////////////////////////
//
// HumdrumFileBasic::makeNewSpineInfo -- take the previous spine information
//    and generate a new spine information set based on the HumdrumRecord
//    line that has at least one path indicator
//

void HumdrumFileBasic::makeNewSpineInfo(vector<char*>&spineinfo,
   HumdrumRecord& aRecord, int newsize, int& spineid,
   vector<int>& ex) {

   vector<char*> newinfo;
   newinfo.resize(newsize);
   int i, j;
   for (i=0; i<newsize; i++) {
      newinfo[i] = new char[1024];
      newinfo[i][0] = '\0';
   }

   int spinecount = aRecord.getFieldCount();
   int subcount;
   int inindex = 0;
   int outindex = 0;

   for (inindex=0; inindex<spinecount; inindex++) {
      if (strcmp("*^", aRecord[inindex]) == 0) {
         strcpy(newinfo[outindex], "(");
         strcat(newinfo[outindex], spineinfo[inindex]);
         strcat(newinfo[outindex], ")a");
         outindex++;
         strcpy(newinfo[outindex], "(");
         strcat(newinfo[outindex], spineinfo[inindex]);
         strcat(newinfo[outindex], ")b");
         outindex++;
      } else if (strcmp("*-", aRecord[inindex]) == 0) {
         // don't increment outindex
      } else if (strcmp("*+", aRecord[inindex]) == 0) {
         strcpy(newinfo[outindex], spineinfo[inindex]);
         outindex++;
         spineid++;
         sprintf(newinfo[outindex], "%d", spineid);
         outindex++;
      } else if (strcmp("*x", aRecord[inindex]) == 0) {
         strcpy(newinfo[outindex], spineinfo[inindex+1]);
         outindex++;
         strcpy(newinfo[outindex], spineinfo[inindex]);
         outindex++;
         inindex++;
      } else if (strcmp("*v", aRecord[inindex]) == 0) {
         subcount = 1;
         strcpy(newinfo[outindex], spineinfo[inindex]);
         for (j=inindex+1; j<spinecount; j++) {
            if (strcmp("*v", aRecord[j]) == 0) {
               subcount++;
               if (subcount > 1) {
                  strcat(newinfo[outindex], " ");
                  strcat(newinfo[outindex], spineinfo[inindex+subcount-1]);
                  simplifySpineString(newinfo[outindex]);
               }
            } else {
               break;
            }
         }
         if (subcount == 1) {
            cout << "Error: single *v path indicator on line: "
                 << aRecord.getLineNum() << "\n"
                 << aRecord.getLine()
                 << endl;
            exit(1);
         } else {
            inindex += subcount-1;
         }
         // check to see if the spine info can be simplified:
         simplifySpineInfo(newinfo, outindex);

         outindex++;
      } else if (strncmp("**", aRecord[inindex], 2) == 0) {
         int value;
         value = Convert::exint.getValue(aRecord[inindex]);
         if (spineid != (int)ex.size()) {
            cout << "Error in exclusive interpretation allocation"
                 << endl;
            exit(1);
         }
         if (value == E_unknown) {
            value = Convert::exint.add(aRecord[inindex]);
            ex.push_back(value);
         } else {
            ex.push_back(value);
         }
         strcpy(newinfo[outindex], spineinfo[inindex]);
         outindex++;
      } else {
         strcpy(newinfo[outindex], spineinfo[inindex]);
         outindex++;
      }
   }

   if (outindex != (int)newinfo.size()) {
      cout << "Error in HumdrumFileBasic path parsing" << endl;
      exit(1);
   }

   if (inindex != (int)spineinfo.size()) {
      cout << "Error in HumdrumFileBasic path parsing" << endl;
      exit(1);
   }

   // delete the old information:
   for (i=0; i<(int)spineinfo.size(); i++) {
      if (spineinfo[i] != NULL) {
         delete [] spineinfo[i];
         spineinfo[i] = NULL;
      }
   }
   spineinfo.resize(newinfo.size());

   // copy the new spine path indicators
   int length;
   for (i=0; i<(int)spineinfo.size(); i++) {
      length = strlen(newinfo[i]);
      spineinfo[i] = new char[length + 1];
      strcpy(spineinfo[i], newinfo[i]);
      delete [] newinfo[i];
      newinfo[i] = NULL;
   }

   newinfo.resize(0);
}



//////////////////////////////
//
// HumdrumFileBasic::simplifySpineString --
//     function added Sat May  8 12:57:37 PDT 2004
//

void HumdrumFileBasic::simplifySpineString(char* spinestring) {
   int length = strlen(spinestring);
   if (spinestring[length-1] == ' ') {
      spinestring[length-1] = '\0';
   }

   if (strchr(spinestring, ' ') == NULL) {
      // no multiple sources, so already in simplest form
      return;
   }
   if (strchr(spinestring, '(') == NULL) {
      // no sub-spine data, so already in simplest form
      return;
   }

   char buffer[1024] = {0};
   strcpy(buffer, spinestring);
   vector<char*> ptrs;
   vector<int> lengths;

   ptrs.reserve(100);
   ptrs.resize(0);

   lengths.reserve(100);
   lengths.resize(0);

   char* ptr = strtok(buffer, " ");
   while (ptr != NULL) {
      ptrs.push_back(ptr);
      length = strlen(ptr);
      lengths.push_back(length);
      ptr = strtok(NULL, " ");
   }


   int i, j;
   int k;
   for (i=0; i<(int)lengths.size()-1; i++) {
      for (j=i+1; j<(int)lengths.size(); j++) {
         if (lengths[i] == lengths[j]) {
            if (strncmp(ptrs[i], ptrs[j], lengths[i]-1) == 0) {
               spinestring[0] = '\0';
               for (k=0; k<(int)ptrs.size(); k++) {
                  if (k == j) continue;
                  if (k == i) {
                     strncat(spinestring, &(ptrs[k][1]), lengths[k]-3);
                     strcat(spinestring, " ");
                     continue;
                  }
                  strcat(spinestring, ptrs[k]);
                  strcat(spinestring, " ");
               }
               simplifySpineString(spinestring);
               goto endofspinefunction;
            }
         }
      }
   }

endofspinefunction:
   length = strlen(spinestring);
   if (spinestring[length-1] == ' ') {
      spinestring[length-1] = '\0';
   }

   return;
}



//////////////////////////////
//
// HumdrumFileBasic::simplifySpineInfo -- just a simple simplification for
//     now: if two spines are joined (such as an ossia) then go back
//     to the original number.
//
//  Other cases to add:  1a 1b 2 simplifies to 1 2
//                       1a 2 1b simplifies to 1 2
//                       1b 1a 2 simplifies to 1 2
//

void HumdrumFileBasic::simplifySpineInfo(vector<char*>& info, int index) {
   char* temp;
   int length;
   length = strlen(info[index]);
   temp = new char[length + 1];
   strcpy(temp, info[index]);
   int count = 0;
   int i;
   for (i=0; i<length; i++) {
      if (temp[i] == ' ') {
         count++;
      }
   }

   char* p1;
   char* p2;
   if (count == 1) {
      p1 = strtok(temp, " ");
      p2 = strtok(NULL, " ");

      p1 += 1;
      p2 += 1;

      int len1 = strlen(p1);
      int len2 = strlen(p2);

      if (len1 > 2 && len2 > 2) {
         p1[len1-2] = '\0';
         p2[len2-2] = '\0';

         if (strcmp(p1, p2) == 0) {
            strcpy(info[index], p1);
         }
      }
   }

   delete [] temp;
}



//////////////////////////////
//
// HumdrumFileBasic::predictNewSpineCount --
//

int HumdrumFileBasic::predictNewSpineCount(HumdrumRecord& aRecord) {
   int output = 0;
   int spinecount = aRecord.getFieldCount();
   int subcount;
   int i;
   int j;
   for (i=0; i<spinecount; i++) {
      if (strcmp("*^", aRecord[i]) == 0) {
         output += 2;
      } else if (strcmp("*-", aRecord[i]) == 0) {
         // should this be something else?:
         //output = output;
      } else if (strcmp("*+", aRecord[i]) == 0) {
         output += 2;
      } else if (strcmp("*x", aRecord[i]) == 0) {
         output++;
      } else if (strcmp("*v", aRecord[i]) == 0) {
         subcount = 1;
         for (j=i+1; j<spinecount; j++) {
            if (strcmp("*v", aRecord[j]) == 0) {
               subcount++;
            } else {
               break;
            }
         }
         if (subcount == 1) {
            cout << "Error: single *v pathindicator on line: "
                 << aRecord.getLineNum() << "\n"
                 << aRecord.getLine()
                 << endl;
            exit(1);
         }
         output++;
         i = j-1;
      } else {
         output++;
      }
   }

   return output;
}


///////////////////////////////////////////////////////////////////////////
//
// dot representation analysis functions
//

//////////////////////////////
//
// HumdrumFileBasic::privateDotAnalysis --
//

void HumdrumFileBasic::privateDotAnalysis(void) {
   vector<int> lastline;
   vector<int> lastspine;

   int i, j;
   int count = 0;
   int newcount = 0;
   int length = (*this).getNumLines();
   for (i=0; i<length; i++) {
      if (strncmp((*this)[i][0], "**", 2) == 0) {
         count = (*this)[i].getFieldCount();
         lastline.resize(count);
         lastspine.resize(count);
         for (j=0; j<count; j++) {
            lastline[j] = -1;
            lastspine[j] = -1;
         }
         continue;
      } else if ((*this)[i].hasPathQ()) {
         count = (*this)[i].getFieldCount();
         newcount = predictNewSpineCount((*this)[i]);
	 // if (newcount == count) {
         //    continue;
         // }
         readjustDotArrays(lastline, lastspine, (*this)[i], newcount);
         continue;
      }

      if ((*this)[i].getType() == E_humrec_data) {
         if (newcount != 0 && newcount != (*this)[i].getFieldCount()) {
            cout << "Error on line " << i+1 << ": invalid number of spines"
                 << endl;
            exit(1);
         } else {
            count = newcount;
            newcount = 0;
         }
         count = (*this)[i].getFieldCount();
         for (j=0; j<count; j++) {
            if (strcmp((*this)[i][j], ".") == 0) {
               (*this)[i].setDotLine(j, lastline[j]);
               (*this)[i].setDotSpine(j, lastspine[j]);
            } else {
               lastline[j] = i;
               lastspine[j] = j;
            }
         }
      }
   }
}



//////////////////////////////
//
// HumdrumFileBasic::readjustDotArrays -- handle spine path changes
//

void HumdrumFileBasic::readjustDotArrays(vector<int>& lastline,
      vector<int>& lastspine, HumdrumRecord& record, int newsize) {

   vector<int> newline(newsize);
   vector<int> newspine(newsize);
   fill(newline.begin(), newline.end(), -1);
   fill(newspine.begin(), newspine.end(), -1);

   int i, j;

   int spinecount = record.getFieldCount();
   int subcount;
   int inindex = 0;
   int outindex = 0;

   for (inindex=0; inindex<spinecount; inindex++) {
      if (strcmp("*^", record[inindex]) == 0) {
         newline[outindex] = lastline[inindex];
         newspine[outindex] = lastspine[inindex];
         outindex++;
         newline[outindex] = lastline[inindex];
         newspine[outindex] = lastspine[inindex];
         outindex++;
      } else if (strcmp("*-", record[inindex]) == 0) {
         // don't increment outindex
      } else if (strcmp("*+", record[inindex]) == 0) {
         newline[outindex] = lastline[inindex];
         newspine[outindex] = lastline[inindex];
         outindex++;
         newline[outindex] = -1;
         newspine[outindex] = -1;
         outindex++;
      } else if (strcmp("*x", record[inindex]) == 0) {
         newline[outindex] = lastline[inindex+1];
         newspine[outindex] = lastspine[inindex+1];
         newline[outindex+1] = lastline[inindex];
         newspine[outindex+1] = lastspine[inindex];
         outindex+=2;
         inindex++;
      } else if (strcmp("*v", record[inindex]) == 0) {
         subcount = 1;
         newline[outindex] = lastline[outindex];
         newspine[outindex] = lastspine[outindex];
         for (j=inindex+1; j<spinecount; j++) {
            if (strcmp("*v", record[j]) == 0) {
               subcount++;
               if (subcount > 1) {
                  newline[outindex] = lastline[inindex+subcount-1];
                  newspine[outindex] = lastspine[inindex+subcount-1];
               }
            } else {
               break;
            }
         }
         if (subcount == 1) {
            cout << "Error: single *v path indicator on line: "
                 << record.getLineNum() << "\n"
                 << record.getLine()
                 << endl;
            exit(1);
         } else {
            inindex += subcount-1;
         }
         outindex++;
      } else if (strncmp("**", record[inindex], 2) == 0) {
         newline[outindex] = -1;
         newspine[outindex] = -1;
         outindex++;
      } else {
         newline[outindex] = lastline[inindex];
         newspine[outindex] = lastspine[inindex];
         outindex++;
      }
   }

   if (outindex != (int)newline.size()) {
      cout << "Error in HumdrumFileBasic path parsing" << endl;
      exit(1);
   }

   if (inindex != (int)lastline.size()) {
      cout << "Error in HumdrumFileBasic path parsing" << endl;
      exit(1);
   }

   // copy the new information
   lastline.resize(newline.size());
   lastspine.resize(newspine.size());
   for (i=0; i<(int)newline.size(); i++) {
      lastline[i] = newline[i];
      lastspine[i] = newspine[i];
   }

}



//////////////////////////////
//
// HumdrumFileBasic::extractEmbeddedDataFromPdf --
//

void HumdrumFileBasic::extractEmbeddedDataFromPdf(ostream& outputData,
      istream& inputData) {
   PDFFile pdffile;
   pdffile.process(inputData);
   int filecount = pdffile.getEmbeddedFileCount();
   int i;
   for (i=0; i<filecount; i++) {
      if (pdffile.isEmbeddedHumdrumFile(i)) {
         pdffile.getEmbeddedFileContents(i, outputData);
      }
   }
}



///////////////////////////////////////////////////////////////////////////
//
// Code for downloading data from the internet.
//


//////////////////////////////
//
// HumdrumFileBasic::getUriToUrlMapping --
//

char* HumdrumFileBasic::getUriToUrlMapping(char* buffer, int buffsize,
      const char* uri) {

   if (strncmp(uri, "http://", strlen("http://")) == 0) {
      // URI is URL, just copy:
      // the strncpy function will not terminate the string if the buffer
      // is shorter than uri...
      strncpy(buffer, uri, buffsize);
      return buffer;
   }

   if ((strncmp(uri, "humdrum://", strlen("humdrum://")) == 0) ||
       (strncmp(uri, "hum://", strlen("hum://")) == 0) ||
       (strncmp(uri, "h://", strlen("h://")) == 0) ) {

      const char* ptr = uri;
      // skip over the staring portion of the address:
      if (strncmp(ptr, "humdrum://", strlen("humdrum://")) == 0) {
         ptr += strlen("humdrum://");
      } else if (strncmp(ptr, "hum://", strlen("hum://")) == 0) {
         ptr += strlen("hum://");
      } else if (strncmp(ptr, "h://", strlen("h://")) == 0) {
         ptr += strlen("h://");
      }
      string location = ptr;
      string filename = "";

      size_t found = location.rfind("/");
      if (found != string::npos) {
         filename = location.substr(found+1);
         location.resize(found);
      }

      stringstream httprequest;
      httprequest << "http://" << "kern.ccarh.org";
      if (strchr(filename.c_str(), '.') != NULL) {
         httprequest << "/cgi-bin/ksdata?file=";
         if (filename.size() > 0) {
            httprequest << filename;
         }
         httprequest << "&l=";
         if (location.size() > 0) {
            httprequest << location;
         }
      } else {
         httprequest << "/cgi-bin/ksdata?l=";
         if (location.size() > 0) {
            httprequest << location;
            httprequest << "/";
            httprequest << filename;
         }
      }
      httprequest << "&format=kern";
      httprequest << ends;

      strcpy(buffer, httprequest.str().c_str());
      return buffer;
   }

   if (strncmp(uri, "jrp://", strlen("jrp://")) == 0) {
      const char* ptr = uri;
      // skip over the staring portion of the address:
      if (strncmp(ptr, "jrp://", strlen("jrp://")) == 0) {
         ptr += strlen("jrp://");
      } else if (strncmp(ptr, "jrp:", strlen("jrp:")) == 0) {
         ptr += strlen("jrp:");
      }

      stringstream httprequest;
      httprequest << "http://" << "jrp.ccarh.org";
      httprequest << "/cgi-bin/jrp?a=humdrum&f=";
      httprequest << ptr;
      httprequest << ends;

      strcpy(buffer, httprequest.str().c_str());
      return buffer;
   }

   // not familiar with the URI, just assume that it is a URL:
   strcpy(buffer, uri);
   return buffer;
}



#ifdef USING_URI

//////////////////////////////
//
// readFromHumdrumURI -- Read a Humdrum file from an humdrum:// web address
//
// Example:
// maps: humdrum://osu/classical/haydn/london/sym099a.krn
// into:
// http://kern.ccarh.org/cgi-bin/ksdata?file=sym099a.krn&l=/osu/classical/haydn/london&format=kern
//

void HumdrumFileBasic::readFromHumdrumURI(const char* humdrumaddress) {
   const char* ptr = humdrumaddress;
   // skip over the staring portion of the address:
   if (strncmp(ptr, "humdrum://", strlen("humdrum://")) == 0) {
      ptr += strlen("humdrum://");
   } else if (strncmp(ptr, "hum://", strlen("hum://")) == 0) {
      ptr += strlen("hum://");
   } else if (strncmp(ptr, "h://", strlen("h://")) == 0) {
      ptr += strlen("h://");
   }
   string location;
   location = ptr;

   string filename;
   filename = "";

   location = ptr;


   size_t found = location.rfind("/");
   if (found != string::npos) {
      filename = location.substr(found+1);
      location.resize(found);
   }

   stringstream httprequest;
   httprequest << "http://" << "kern.ccarh.org";
   httprequest << "/cgi-bin/ksdata?file=";
   if (filename.size() > 0) {
      httprequest << filename;
   }
   httprequest << "&l=";
   if (location.size() > 0) {
      httprequest << location;
   }
   httprequest << "&format=kern";
   httprequest << ends;

   readFromHttpURI(httprequest.str().c_str());
}



//////////////////////////////
//
// readFromJrpURI -- Read a Humdrum file from a jrp:// web-style address
//
// Example:
// maps:
//    jrp://Jos2721-La_Bernardina
// into:
//    http://jrp.ccarh.org/cgi-bin/jrp?a=humdrum&f=Jos2721-La_Bernardina
//

void HumdrumFileBasic::readFromJrpURI(const char* jrpaddress) {
   const char* ptr = jrpaddress;
   // skip over the staring portion of the address:
   if (strncmp(ptr, "jrp://", strlen("jrp://")) == 0) {
      ptr += strlen("jrp://");
   } else if (strncmp(ptr, "jrp:", strlen("jrp:")) == 0) {
      ptr += strlen("jrp:");
   }

   stringstream httprequest;
   httprequest << "http://" << "jrp.ccarh.org";
   httprequest << "/cgi-bin/jrp?a=humdrum&f=";
   httprequest << ptr;
   httprequest << ends;

   readFromHttpURI(httprequest.str().c_str());
}



//////////////////////////////
//
// readFromHttpURI -- Read a Humdrum file from an http:// web address
//

void HumdrumFileBasic::readFromHttpURI(const char* webaddress) {
   string hostname;

   string location;
   location.resize(0);

   const char* ptr = webaddress;
   const char* filename = NULL;

   if (strncmp(webaddress, "http://", strlen("http://")) == 0) {
      // remove the "http://" portion of the webaddress
      ptr += strlen("http://");
   }

   hostname = ptr;
   size_t found = hostname.rfind("/");
   if (found != string::npos) {
      hostname.resize(found);
   }

   if ((filename = strchr(ptr, '/')) != NULL) {
      location = filename;
   }
   if (location.size() == 0) {
      location = "/";
   }

   char newline[3] = {0x0d, 0x0a, 0};

   stringstream request;
   request << "GET "   << location << " HTTP/1.1" << newline;
   request << "Host: " << hostname  << newline;
   request << "User-Agent: HumdrumFile Downloader 1.0 ("
           << __DATE__ << ")" << newline;
   request << "Connection: close" << newline;  // this line is necessary
   request << newline;
   request << ends;

   // cout << "HOSTNAME: " << hostname  << endl;
   // cout << "LOCATION: " << location << endl;
   // cout << request.str().c_str() << endl;
   // cout << "-------------------------------------------------" << endl;

   int socket_id = open_network_socket(hostname.c_str(), 80);
   if (::write(socket_id, request.str().c_str(), strlen(request.str().c_str())) == -1) {
      exit(-1);
   }
   #define URI_BUFFER_SIZE (10000)
   char buffer[URI_BUFFER_SIZE];
   int message_len;
   stringstream inputdata;
   stringstream header;
   int foundcontent = 0;
   int i;
   int newlinecounter = 0;

   // read the response header:
   while ((message_len = ::read(socket_id, buffer, 1)) != 0) {
      header << buffer[0];
      if ((buffer[0] == 0x0a) || (buffer[0] == 0x0d)) {
               newlinecounter++;
      } else {
               newlinecounter = 0;
      }
      if (newlinecounter == 4) {
         foundcontent = 1;
         break;
      }
   }
   if (foundcontent == 0) {
      cerr << "Funny error trying to read server response" << endl;
      exit(1);
   }

   // now read the size of the rest of the data which is expected
   int datalength = -1;

   // also, check for chunked transfer encoding:

   int chunked = 0;

   header << ends;
   // cout << header.str().c_str() << endl;
   // cout << "-------------------------------------------------" << endl;
   while (header.getline(buffer, URI_BUFFER_SIZE)) {
      int len = strlen(buffer);
      for (i=0; i<len; i++) {
         buffer[i] = std::tolower(buffer[i]);
      }
      if (strstr(buffer, "content-length") != NULL) {
         for (i=14; i<len; i++) {
            if (std::isdigit(buffer[i])) {
               sscanf(&buffer[i], "%d", &datalength);
               if (datalength == 0) {
                  cerr << "Error: no data found for URI, probably invalid\n";
                  exit(1);
               }
               break;
            }
         }
      } else if ((strstr(buffer, "transfer-encoding") != NULL) &&
         (strstr(buffer, "chunked") != NULL)) {
         chunked = 1;
      }
      // if (datalength >= 0) {
      //    break;
      // }
   }

   // once the length of the remaining data is known (or not), read it:
   if (datalength > 0) {
      getFixedDataSize(socket_id, datalength, inputdata, buffer,
            URI_BUFFER_SIZE);

   } else if (chunked) {
      int chunksize;
      int totalsize = 0;
      do {
         chunksize = getChunk(socket_id, inputdata, buffer, URI_BUFFER_SIZE);
         totalsize += chunksize;
      } while (chunksize > 0);
      if (totalsize == 0) {
         cerr << "Error: no data found for URI (probably invalid)\n";
         exit(1);
      }
   } else {
      // if the size of the rest of the data cannot be found in the
      // header, then just keep reading until done (but this will
      // probably cause a 5 second delay at the last read).
      while ((message_len = ::read(socket_id, buffer, URI_BUFFER_SIZE)) != 0) {
         if (foundcontent) {
            inputdata.write(buffer, message_len);
         } else {
            for (i=0; i<message_len; i++) {
               if (foundcontent) {
                  inputdata << buffer[i];
               } else {
                  header << buffer[i];
                  if ((buffer[i] == 0x0a) || (buffer[i] == 0x0d)) {
                     newlinecounter++;
                  } else {
                     newlinecounter = 0;
                  }
                  if (newlinecounter == 4) {
                     foundcontent = 1;
                     continue;
                  }
               }

            }
         }
      }
   }

   close(socket_id);

   inputdata << ends;

   // cout << "CONTENT:=======================================" << endl;
   // cout << inputdata.str().c_str();
   // cout << "===============================================" << endl;

   HumdrumFileBasic::read(inputdata);
}



//////////////////////////////
//
//  HumdrumFileBasic::getChunk --
//
// http://en.wikipedia.org/wiki/Chunked_transfer_encoding
// http://tools.ietf.org/html/rfc2616
//
// Chunk Format
//
// If a Transfer-Encoding header with a value of chunked is specified in
// an HTTP message (either a request sent by a client or the response from
// the server), the body of the message is made of an unspecified number
// of chunks ending with a last, zero-sized, chunk.
//
// Each non-empty chunk starts with the number of octets of the data it
// embeds (size written in hexadecimal) followed by a CRLF (carriage
// return and linefeed), and the data itself. The chunk is then closed
// with a CRLF. In some implementations, white space chars (0x20) are
// padded between chunk-size and the CRLF.
//
// The last chunk is a single line, simply made of the chunk-size (0),
// some optional padding white spaces and the terminating CRLF. It is not
// followed by any data, but optional trailers can be sent using the same
// syntax as the message headers.
//
// The message is finally closed by a last CRLF combination.

int HumdrumFileBasic::getChunk(int socket_id, stringstream& inputdata,
      char* buffer, int bufsize) {
   int chunksize = 0;
   int message_len;
   char digit[2] = {0};
   int founddigit = 0;

   // first read the chunk size:
   while ((message_len = ::read(socket_id, buffer, 1)) != 0) {
      if (isxdigit(buffer[0])) {
         digit[0] = buffer[0];
         chunksize = (chunksize << 4) | strtol(digit, NULL, 16);
         founddigit = 1;
      } else if (founddigit) {
         break;
      } // else skipping whitespace before chunksize
   }
   if ((chunksize <= 0) || (message_len == 0)) {
      // next chunk is zero, so no more primary data0:w
      return 0;
   }

   // read the 0x0d and 0x0a characters which are expected (required)
   // after the size of chunk size:
   if (buffer[0] != 0x0d) {
      cerr << "Strange error occurred right after reading a chunk size" << endl;
      exit(1);
   }

   // now expect 0x0a:
   message_len = ::read(socket_id, buffer, 1);
   if ((message_len == 0) || (buffer[0] != 0x0a)) {
      cerr << "Strange error after reading newline at end of chunk size"<< endl;
      exit(1);
   }

   return getFixedDataSize(socket_id, chunksize, inputdata, buffer, bufsize);
}



//////////////////////////////
//
// getFixedDataSize -- read a know amount of data from a socket.
//

int HumdrumFileBasic::getFixedDataSize(int socket_id, int datalength,
      stringstream& inputdata, char* buffer, int bufsize) {
   int readcount = 0;
   int readsize;
   int message_len;

   while (readcount < datalength) {
      readsize = bufsize;
      if (readcount + readsize > datalength) {
         readsize = datalength - readcount;
      }
      message_len = ::read(socket_id, buffer, readsize);
      if (message_len == 0) {
         // shouldn't happen, but who knows...
         break;
      }
      inputdata.write(buffer, message_len);
      readcount += message_len;
   }

   return readcount;
}



//////////////////////////////
//
// HumdrumFileBasic::prepare_address -- Store a computer name, such as
//    www.google.com into a sockaddr_in structure for later use in
//    open_network_socket.
//

void HumdrumFileBasic::prepare_address(struct sockaddr_in *address,
      const char *hostname, unsigned short int port) {

   memset(address, 0, sizeof(struct sockaddr_in));
   struct hostent *host_entry;
   host_entry = gethostbyname(hostname);

   if (host_entry == NULL) {
      cerr << "Could not find address for" << hostname << endl;
      exit(1);
   }

   // copy the address to the sockaddr_in struct.
   memcpy(&address->sin_addr.s_addr, host_entry->h_addr_list[0],
         host_entry->h_length);

   // set the family type (PF_INET)
   address->sin_family = host_entry->h_addrtype;
   address->sin_port = htons(port);
}



//////////////////////////////
//
// open_network_socket -- Open a connection to a computer on the internet.
//    Intended for downloading a Humdrum file from a website.
//

int HumdrumFileBasic::open_network_socket(const char *hostname,
      unsigned short int port) {
   int inet_socket;                 // socket descriptor
   struct sockaddr_in servaddr;     // IP/port of the remote host

   prepare_address(&servaddr, hostname, port);

   // socket(domain, type, protocol)
   //    domain   = PF_INET(internet/IPv4 domain)
   //    type     = SOCK_STREAM(tcp) *
   //    protocol = 0 (only one SOCK_STREAM type in the PF_INET domain
   inet_socket = socket(PF_INET, SOCK_STREAM, 0);

   if (inet_socket < 0) {
      // socket returns -1 on error
      cerr << "Error opening socket to computer " << hostname << endl;
      exit(1);
   }

   // connect(sockfd, serv_addr, addrlen)
   if (connect(inet_socket, (struct sockaddr *)&servaddr,
         sizeof(struct sockaddr_in)) < 0) {
      // connect returns -1 on error
      cerr << "Error opening connection to coputer: " << hostname << endl;
      exit(1);
   }

   return inet_socket;
}

#endif



//////////////////////////////
//
// HumdrumFileBasic::makeVts -- create a !!!VTS: record for the
// given Humdrum data.  The function will ignore all reference records
// which start with the string "!!!VTS" when calculating the VTS data.
// The data is always assumbed to be encoded with Unix newlines (0x0a).
//

void HumdrumFileBasic::makeVts(string& vtsstring) {
   HumdrumFileBasic::makeVts(vtsstring, (*this));
}


void HumdrumFileBasic::makeVts(string& vtsstring,
      HumdrumFileBasic& infile) {
   int i;
   stringstream tstream;
   for (i=0; i<infile.getNumLines(); i++) {
      if (strncmp(infile[i][0], "!!!VTS", 6) == 0) {
         continue;
      }
      tstream << infile[i] << (char)0x0a;
   }
   tstream << ends;
   int len = strlen(tstream.str().c_str());
   unsigned long checksum = CheckSum::crc32(tstream.str().c_str(), len);
   char buffer[128] = {0};
   sprintf(buffer, "!!!VTS: %lu", checksum);
   vtsstring = buffer;
}


//////////////////////////////
//
// HumdrumFileBasic::makeVtsData -- create a !!!VTS-data: record
// for the Humdrum file.  Ignore any lines which are not data
// when creating the result.  (not comments, barline, interpretations).
//

void HumdrumFileBasic::makeVtsData(string& vtsstring) {
   HumdrumFileBasic::makeVtsData(vtsstring, (*this));
}

void HumdrumFileBasic::makeVtsData(string& vtsstring,
      HumdrumFileBasic& infile) {
   int i;
   stringstream tstream;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isData()) {
         continue;
      }
      tstream << infile[i] << (char)0x0a;
   }
   tstream << ends;
   int len = strlen(tstream.str().c_str());
   unsigned long checksum = CheckSum::crc32(tstream.str().c_str(), len);
   char buffer[128] = {0};
   sprintf(buffer, "!!!VTS-data: %lu", checksum);
   vtsstring = buffer;
}


// md5sum: ade20f80d810702755a5e2aac8395c32 HumdrumFileBasic.cpp [20050403]
