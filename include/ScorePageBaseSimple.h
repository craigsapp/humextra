//
// Copyright 2002 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu Feb 14 23:40:51 PST 2002
// Last Modified: Fri Jun 12 22:58:34 PDT 2009 (renamed SigCollection class)
// Filename:      ...sig/src/sigInfo/ScorePageBaseSimple.h
// Web Address:   http://sig.sapp.org/include/sigInfo/ScorePageBaseSimple.h
// Syntax:        C++
//
// Description:   A page of SCORE data
//

#ifndef _SCOREPAGEBASESIMPLE_H_INCLUDED
#define _SCOREPAGEBASESIMPLE_H_INCLUDED

#include "ScoreRecord.h"

#include <iostream>
#include <vector>

using namespace std;

typedef Array<int> ArrayInt;

class ScorePageBaseSimple {
   public:
                     ScorePageBaseSimple(void);
                    ~ScorePageBaseSimple();

      void           clear             (void);
      ScorePageBaseSimple& operator=         (ScorePageBaseSimple &aPage);

      // data access functions
      int            getSize           (void);
      ScoreRecord&   operator[]        (int index);

      void           appendItem        (const ScoreRecord& aRecord);
      void           addItem(ScoreRecord& aRecord) { appendItem(aRecord); };

      void           appendItem        (ScorePageBaseSimple& aPage);
      void           addItem(ScorePageBaseSimple& aPage) { appendItem(aPage); };

      void           appendItem        (SigCollection<ScoreRecord>& recs);
      void           addItem           (SigCollection<ScoreRecord>& recs)
                                          { appendItem(recs); };

      void           getItemsPosition  (Array<int>& indices, float position,
                                        int staff, float tolerance = 0.01);

      // sorting functions
      int            sortByStaff       (void);
      int            findStaff         (int staffno);

      // file I/O and printing functions
      void           printAscii        (std::ostream& out, int roundQ = 1,
		                        int verboseQ = 0);
      void           readAscii         (const char* filename, int verboseQ = 0);
      void           readBinary        (const char* filename, int verboseQ = 0);
      void           readFile          (const char* filename, int verboseQ = 0);
      void           writeBinary       (const char* filename);
      void           writeBinary2Byte  (const char* filename);
      void           writeBinary4Byte  (const char* filename);

      void           setVersion        (float value);
      void           setVersionWinScore(void);
      float          getVersion        (void);
      void           setSerial         (long value);
      long           getSerial         (void);

   protected:
      std::vector<ScoreRecord> m_data;

      Array<float> trailer;      // data which occurs at the end of a file

      int sortQ;                 // 0 = data is not sorted, 1 = data is sorted

      // analyze musical systems on the page
      int systemAnalysisQ;       // for checking on if the system data is valid
      Array<int> staffsystem;    // system ownerships of staves on page
      Array<int> track;          // staff number on system
      Array<int> voice;          // voice number on system
      int systemCount;           // number of systems on the page
      int staffCount;            // number of staves on the page
      int maxStaffNumber;        // largest staff number on page
      Array<int> staffStart;     // starting index of the staff item
      Array<int> staffSize;      // number of items on each staff
      Array<int> systemind;      // indices for accessing by system
      Array<int> systemSize;     // number of items in each system
      Array<int> systemStart;    // starting index of items in systemind

   private:
      void           writeLittleFloat  (std::ostream& out, float number);
      float          readLittleFloat   (std::istream& instream);
      int            readLittleShort   (std::istream& input);
      static int     staffsearch       (const void* A, const void* B);
      void           shrinkParameters  (void);
      void           initializeTrailer (long serial = 0x50504153);
      void           readAsciiScoreLine(std::istream& infile, ScoreRecord& record,
                                        int verboseQ = 0);
   public:
      static int     compareStaff      (const void* A, const void* B);
      static int     compareSystem     (const void* A, const void* B);
};


class SystemRecord {
   public:
      SystemRecord(void) { clear(); }
     ~SystemRecord() { clear(); }
      void clear(void) { system = 0; index = 0; ptr = NULL; }
      int system;
      int index;
      ScoreRecord* ptr;
};

#endif /* _SCOREPAGEBASESIMPLE_H_INCLUDED */


// md5sum: c3202867c565c42152c072c1ae65ffbe ScorePageBaseSimple.h [20050403]
