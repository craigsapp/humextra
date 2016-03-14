//
// Copyright 2003 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Jan  1 16:19:42 PST 2003 (extracted from HumdrumFile.h)
// Creation Date: Wed Jan  1 16:38:30 PST 2003
// Filename:      ...sig/include/sigInfo/Maxwell.h
// Web Address:   http://sig.sapp.org/include/sigInfo/Maxwell.h
// Syntax:        C++ 
//
// Description:   Harmony analysis functions based on Maxwell's dissertation.
//

#ifndef _MAXWELL_H_INCLUDED
#define _MAXWELL_H_INCLUDED

#include "HumdrumFile.h"
#include "EnumerationInterval.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

//
// Maxwell::analyzeVerticalDissonance defines
//

#define CONSONANT_VERTICAL   0
#define DISSONANT_VERTICAL   1
#define UNDEFINED_VERTICAL  -1

//
// Maxwell::analyzeTertian defines
//

#define TERTIAN_YES       0
#define TERTIAN_NO        1
#define TERTIAN_UNKNOWN  -1

//
// Maxwell::analyzeAccent defines
//

#define ACCENT_YES       1
#define ACCENT_NO        0
#define ACCENT_UNKNOWN  -1

//
// Maxwell::analyzeTertianDissonanceLevel defines
//

#define TERTIAN_UNKNOWN  -1
#define TERTIAN_0         0   /* i.e., all rests or single notes */
#define TERTIAN_1         1
#define TERTIAN_1_5       1.5 /* dominant sevenths (=2) */
#define TERTIAN_2         2
#define TERTIAN_2_5       2.5 /* incomplete minor minor sevenths (=2) */
#define TERTIAN_3         3
#define TERTIAN_4         4
#define TERTIAN_5         5

//
// HumdrumFile::analyzeMaxwellDissonantInContext defines
//

#define DISSIC_UNDEFINED -1
#define DISSIC_YES        0
#define DISSIC_NO         1

//
// Maxwell::analyzeDissonantNotes defines
//

typedef vector<int> ArrayInt;

#define NOTEDISSONANT_UNKNOWN  -1
#define NOTEDISSONANT_NO        0
#define NOTEDISSONANT_YES       1

//
// Maxwell::analyzeSonorityRelations defines
//

#define CHORD_UNKNOWN 'X'
#define CHORD_CHORD   'c'
#define CHORD_PREVSUB 'p'
#define CHORD_NEXTSUB 'n'
#define CHORD_PASSING 'g'
#define CHORD_SUSPEND 's'
#define CHORD_SUBORD  'u'


class Maxwell {
   public:
            Maxwell                       (void);
           ~Maxwell                       ();

      static void  analyzeVerticalDissonance(HumdrumFile& score, 
                                             vector<int>& vertdis);
      static void  analyzeTertian           (HumdrumFile& score,
                                             vector<int>& tertian);
      static void  analyzeAccent            (HumdrumFile& score,
                                             vector<int>& accent, 
                                             int flag = AFLAG_COMPOUND_METER);
      static void  analyzeTertianDissonanceLevel (HumdrumFile& score,
                                             vector<double>& terdis);
      static void  analyzeDissonantInContext(HumdrumFile& score,
                                             vector<int>& dissic,
                                             vector<int>& vertdis, 
                                             vector<int>& tertian, 
                                             vector<double>& terdis, 
                                             vector<int>& accent,
                                             int flag = AFLAG_COMPOUND_METER);
      static void  analyzeDissonantInContext(HumdrumFile& score,
                                             vector<int>& dissic, 
                                             int flag = AFLAG_COMPOUND_METER);
      static void  analyzeDissonantNotes   (HumdrumFile& score, 
                                             vector<ArrayInt>& notediss);
      static void  analyzeDissonantNotes   (HumdrumFile& score,
                                             vector<ArrayInt>& notediss,
                                             vector<int>& vertdis, 
                                             vector<int>& tertian, 
                                             vector<double>& terdis, 
                                             vector<int>& accent, 
                                             vector<int>& dissic);
      static void  analyzeSonorityRelations (HumdrumFile& score, 
                                             vector<int>&sonrel,
                                             vector<int>& vertdis, 
                                             vector<int>& tertian, 
                                             vector<double>& terdis, 
                                             vector<int>& accent, 
                                             vector<int>& dissic,
                                             vector<double>& beatdur, 
                                             vector<ChordQuality>& cq, 
                                             int flag = AFLAG_COMPOUND_METER);
      static void  analyzeSonorityRelations (HumdrumFile& score,
                                             vector<int>&sonrel, 
                                             int flag = AFLAG_COMPOUND_METER);
           
   private:

      // for use with analyzeTertian
      static void  rotateNotes         (vector<int>& notes);
 
      // for use with analyzeDissonantNotes
      static int   measureNoteDissonance(HumdrumFile& score, int line, int note, 
                                       vector<int>& vertdis, vector<int>& accent, 
                                       vector<int>& dissic);

      // for use with analyzeSonorityRelations
      static int   measureChordFunction1(HumdrumFile& score, int line, 
                                       vector<int>& vertdis, vector<int>& tertian, 
                                       vector<double>& terdis, vector<int>& accent,
                                       vector<int>& dissic, vector<double>& beatdur,
                                       vector<ChordQuality>& cq);
      static int   measureChordFunction2(HumdrumFile& score, int line, 
                                       vector<int>& vertdis, vector<int>& tertian, 
                                       vector<double>& terdis, vector<int>& accent, 
                                       vector<int>& dissic, vector<ChordQuality>& cq);

};

#endif /* _MAXWELL_H_INCLUDED */



// md5sum: 907896b19322bc5058f12cf37385d37d Maxwell.h [20050403]
