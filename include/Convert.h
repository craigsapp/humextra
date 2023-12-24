//
// Copyright 1998-2010 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Jun  8 00:38:46 PDT 1998
// Last Modified: Tue Jun 23 14:00:23 PDT 1998
// Last Modified: Fri May  5 12:17:08 PDT 2000 (added kernToMidiNoteNumber())
// Last Modified: Wed Nov 29 12:28:00 PST 2000 (added base40ToMidiNoteNumber())
// Last Modified: Wed Jan  2 12:19:07 PST 2002 (added kotoToDuration)
// Last Modified: Wed Dec  1 01:36:29 PST 2004 (added base40ToIntervalAbbr())
// Last Modified: Sun Jun  4 21:04:50 PDT 2006 (added base40ToPerfViz())
// Last Modified: Fri Jun 12 22:58:34 PDT 2009 (renamed SigCollection class)
// Last Modified: Wed Nov 18 16:40:33 PST 2009 (added base40/trans converts)
// Last Modified: Sat May 22 11:02:12 PDT 2010 (added RationalNumber)
// Last Modified: Sun Dec 26 04:54:46 PST 2010 (added kernClefToBaseline)
// Last Modified: Sat Jan 22 17:13:36 PST 2011 (added kernToDurationNoDots)
// Filename:      ...sig/include/sigInfo/Convert.h
// Web Address:   http://sig.sapp.org/include/sigInfo/Convert.h
// Syntax:        C++ 
//
// Description:   This class contains static function that can be used
//                to convert from one data representation to another.
//


#ifndef _CONVERT_H_INCLUDED
#define _CONVERT_H_INCLUDED

#include "HumdrumEnumerations.h"
#include "ChordQuality.h"
#include "SigCollection.h"
#include "RationalNumber.h"

#include <vector>
#include <string>


class Convert {
   public: 
 
   // enumeration databases

      static EnumerationEI        exint;
      static EnumerationCQT       chordType;
      static EnumerationCQI       chordInversion;
      static EnumerationCQR       kernPitchClass;
      static EnumerationMPC       musePitchClass;
      static EnumerationInterval  intervalNames;

   // conversions dealing with humdrum data


   // conversions dealing with **kern data

      static int       kernToMidiNoteNumber      (const string& aKernString);
      static char*     durationToKernRhythm      (char* output, int outputMaxSize, double input, 
                                                   int timebase = 1);
      static char*     durationRToKernRhythm     (char* output, int outputMaxSize,
                                                  RationalNumber input, 
                                                  int timebase = 1);
      static string    durationToKernRhythm      (double input, int timebase = 1);
      static string    durationRToKernRhythm     (RationalNumber input, 
                                                  int timebase = 1);
      static double    kernToDuration            (const string& aKernString);
      static RationalNumber kernToDurationR      (const string& aKernString);
      static double    kernToDurationNoDots      (const string& aKernString);
      static RationalNumber kernToDurationNoDotsR (const string& aKernString);
      static double    kernTimeSignatureTop      (const string& aKernString);
      static double    kernTimeSignatureBottomToDuration   
                                                 (const string& aKernString);
      static int       kernToOctave              (const string& buffer);
      static int       kernToDiatonicPitch       (const string& buffer);
      static int       kernToDiatonicPitchClass  (const string& buffer);
      static int       kernToDiatonicPitchClassNumeric(const string& buffer);
      static int       kernToDiatonicAlteration  (const string& buffer);
      static int       kernClefToBaseline        (const string& buffer);
      static char*     musePitchToKernPitch      (char* kernOutput,
		                                            int outputMaxSize,
                                                  const string& museInput);
      static char*     museClefToKernClef        (char* kernOutput, int outputMaxSize,
                                                    int museInput);
      static int       kernKeyToNumber           (const string& aKernString);
      static const char* keyNumberToKern         (int number);

   // conversions dealing with **qual data

      static ChordQuality chordQualityStringToValue (const string& aString);
      static int       chordQualityToBaseNote    (const ChordQuality& aQuality);
      static void      chordQualityToNoteSet     (SigCollection<int>& noteSet, 
                                                  const ChordQuality& aQuality);
      static int       chordQualityToInversion   (const string& aQuality);
      static int       chordQualityToRoot        (const string& aQuality);
      static int       chordQualityToType        (const string& aQuality);
      static void      noteSetToChordQuality     (ChordQuality& cq, 
                                                  const SigCollection<int>& aSet);
      static void      noteSetToChordQuality     (ChordQuality& cq, 
                                                  const vector<int>& aSet);

   // conversions dealing with base 40 system of notation

      static char*     base40ToKern               (char* output, int outputMaxSize, int aPitch);
      static char*     base40ToKernTranspose      (char* output, int outputMaxSize, int transpose,
                                                     int keysignature);
      static int       base40ToMidiNoteNumber     (int base40value);
      static int       base40ToAccidental         (int base40value);
      static int       base40IntervalToLineOfFifths(int base40interval);
      static int       base40IntervalToDiatonic   (int base40interval);
      static int       kernToBase40               (const string& kernfield);
      static int       kernToBase40Class          (const string& kernfield);
      static string    kernToScientificNotation   (const string& kernfield,
											const string& flat = "b",
                                 const string& sharp = "#",
		                           const string& doubleflat = "bb",
		                           const string& doublesharp = "x");
      static int       kernNoteToBase40           (const string& name);
      static SigCollection<int> keyToScaleDegrees (int aKey, int aMode);
      static int       museToBase40               (const string& pitchString);
      static int       base40ToScoreVPos          (int pitch, int clef);
      static char*     base40ToMuse               (int base40, char* buffer, int outputMaxSize);
      static int       base40ToDiatonic           (int pitch);
      static char*     base40ToIntervalAbbr       (char* output, int outputMaxSize,
                                                   int base40value);
      static char*     base40ToIntervalAbbrWrap   (char* output, int outputMaxSize,
                                                   int base40value);
      static char*     base40ToIntervalAbbr2      (char* output, int outputMaxSize,
                                                   int base40value);
      static char*     base40ToPerfViz            (char* output, int outputMaxSize,
                                                   int base40value);
      static char*     base40ToTrans              (char* buffer, int outputMaxSize, int base40);
      static int       transToBase40              (const string& buffer);

   // conversions dealing with MIDI base-12 system 
   
      static char*     base12ToKern               (char* output, int outputMaxSize, int aPitch);
      static char*     base12ToPitch              (char* output, int outputMaxSize, int aPitch);
      static int       base12ToBase40             (int aPitch);
      static int       base7ToBase12              (int aPitch, int alter = 0);
      static int       base7ToBase40              (int aPitch, int alter = 0);

   // conversions from frequency in hertz to MIDI note number
      static int       freq2midi                  (double freq, 
                                                   double a440 = 440.0);

   // conversions dealing with **koto data
      static double    kotoToDuration             (const string& aKotoString);
      static RationalNumber kotoToDurationR       (const string& aKotoString);

   // conversions related to serial interval descriptions

      static const char* base12ToTnSetName        (Array<int>& base12);
      static void        base12ToTnSetNameAllSubsets(Array<int>& list, 
                                                   Array<int>& notes);
      static void      base12ToTnNormalForm       (Array<int>& tnorm, 
                                                   Array<int>& base12);
      static void      base12ToNormalForm         (Array<int>& nform, 
                                                   Array<int>& base12);
      static void      base40ToIntervalVector     (Array<int>& iv,  
                                                      Array<int>& base40);
      static void      base12ToIntervalVector     (Array<int>& iv,  
                                                      Array<int>& base12);

      static string    base12ToTnSetName          (vector<int>& base12);
      static void      base12ToTnSetNameAllSubsets(vector<int>& list, 
                                                   vector<int>& notes);
      static void      base12ToTnNormalForm       (vector<int>& tnorm, 
                                                   vector<int>& base12);
      static void      base12ToNormalForm         (vector<int>& nform, 
                                                   vector<int>& base12);
      static void      base40ToIntervalVector     (vector<int>& iv,  
                                                   vector<int>& base40);
      static void      base12ToIntervalVector     (vector<int>& iv,  
                                                   vector<int>& base12);

   protected:
      // findBestNormalRotation used with bse12ToNormalForm
      static int     findBestNormalRotation   (Array<int>& input, int asize, 
                                               Array<int>& choices);

      static int     calculateInversion       (int aType, int bassNote, 
                                               int root);
      static int     checkChord               (const SigCollection<int>& aSet);
      static int     intcompare               (const void* a, const void* b);
      static void    rotatechord              (SigCollection<int>& aChord);
      static void    addCombinations          (Array<Array<int> >& combinations,
                                               Array<int>& input, 
                                               Array<int>& temp, 
                                               int q=0, int r=0);


};


#endif /* _CONVERT_H_INCLUDED */



// md5sum: 77d7b4adfff9e35c6cbd857dfa03ad7a Convert.h [20050403]
