//
// Copyright 1998-2000 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon May 18 13:52:59 PDT 1998
// Last Modified: Fri May  5 13:13:32 PDT 2000 Added sub-spine access
// Last Modified: Fri Oct 13 12:12:15 PDT 2000 Added spine path tracing
// Last Modified: Fri Dec 10 00:03:59 PST 2004 Added isSpineManipulator
// Last Modified: Tue Apr 28 14:34:13 PDT 2009 Added isTandem
// Last Modified: Fri Jun 12 22:58:34 PDT 2009 Renamed SigCollection class
// Last Modified: Sat Aug  8 23:50:10 PDT 2009 Added isExInterp
// Last Modified: Sat May 22 10:13:30 PDT 2010 Added RationalNumber
// Last Modified: Wed Mar 16 14:25:53 PDT 2011 Added getIntervalVector
// Last Modified: Tue Jun 26 09:51:28 PDT 2012 Added interpretation type funcs.
// Last Modified: Mon Dec 10 10:14:08 PST 2012 Added Array<char> getToken
// Last Modified: Sat Apr 20 12:15:42 PDT 2013 Added isNulToken()
// Filename:      ...sig/include/sigInfo/HumdrumRecord.h
// Webpage:       http://sig.sapp.org/include/sigInfo/HumdrumRecord.h
// Syntax:        C++
//
// Description:   Stores the data for one line in a HumdrumFile and
//                segments data into spines.
//

#ifndef _HUMDRUMRECORD_H_INCLUDED
#define _HUMDRUMRECORD_H_INCLUDED

#include "SigCollection.h"
#include "Array.h"
#include "EnumerationEI.h"
#include "Enum_humdrumRecord.h"
#include "RationalNumber.h"

#include <vector>
#include <iostream>

class HumdrumRecord {
   public:
                        HumdrumRecord      (void);
                        HumdrumRecord      (const char* aLine,
                                              int aLineNum = -1);
                        HumdrumRecord      (const HumdrumRecord& aRecord);
                       ~HumdrumRecord      ();

      void              appendField        (const char* aField,
                                              int anInterp = E_unknown,
                                              const char* spinetrace = "");
      void              appendFieldEI      (const char* aField,
                                             const char* anInterp = "**unknown",
                                             const char* spinetrace = "");
      void              changeField        (int aField, const char* aString);
      void              copySpineInfo      (vector<string>& info, int line = 0);
      void              copySpineInfo      (HumdrumRecord& aRecord,
                                              int line = 0);

      int               equalDataQ         (const char* aValue);
      int               equalFieldsQ       (void);
      int               equalFieldsQ       (int anInterp);
      int               equalFieldsQ       (const char* anInterp);
      int               equalFieldsQ       (int anInterp, const char*
                                              compareString);
      int               equalFieldsQ       (const char* anInterp,
                                              const char* compareString);
      double            getAbsBeat         (void) const;
      RationalNumber    getAbsBeatR        (void) const;
      double            getBeat            (void) const;
      RationalNumber    getBeatR           (void) const;
      double            getMeasureDuration (void) const;
      RationalNumber    getMeasureDurationR(void) const;
      int               getDotLine         (int index);
      int               getDotSpine        (int index);
      int               getDotField        (int index)
                                              { return getDotSpine(index); }
      double            getDuration        (void) const;
      RationalNumber    getDurationR       (void) const;
      int               getExInterpNum     (int fieldIndex) const;
      const char*       getExInterp        (int fieldIndex) const;
      int               getFieldCount      (void) const;
      int               getFieldCount      (const char* exinterp) const;
      int               getFieldCount      (int exinterp) const;
      int               getFieldsByExInterp(Array<int>& fields,
                                            const char* exinterp);
      int               getTracksByExInterp(Array<int>& tracks,
                                            const char* exinterp);
      char*             getBibKey          (char* buffer, int maxsize = 0);
      char*             getBibValue        (char* buffer, int maxsize = 0);
      char*             getBibKey          (Array<char>& buffer);
      char*             getBibValue        (Array<char>& buffer);
      string            getBibKey          (const string& buffer);
      string            getBibValue        (const string& buffer);
      const char*       getBibLangIso639_2 (const char* string = NULL);
      static const char*getLanguageName    (const char* code);
      static const char*getBibliographicMeaning(Array<char>& output,
                                            const char* code);
      static string&    getBibliographicMeaning(string& output,
                                            const string& code);
      const char*       getLine            (void);
      int               getLineNum         (void) const;
      int               getPrimaryTrack    (int spineNumber);
      double            getTrack           (int spineNumber);
      int               getTrackColumn     (int track);
      const string&     getSpineInfo       (int index) const;
      int               getSpinePrediction (void);
      int               getSpineWidth      (void);
      void              changeToken        (int spineIndex, int tokenIndex,
                                            const char* newtoken,
                                            char separator = ' ');
      int               getTokenCount      (int fieldIndex,
                                              char separator = ' ');
      char*             getToken           (char* buffer, int fieldIndex,
                                              int tokenIndex,
                                              int buffersize = -1,
                                              char separator = ' ');
      string&           getToken           (string& buffer, int fieldIndex,
                                              int tokenIndex,
                                              int buffersize = -1,
                                              char separator = ' ');
      string            getToken           (int fieldIndex,
                                              int tokenIndex,
                                              int buffersize = -1,
                                              char separator = ' ');
      void              getTokens          (Array<Array<char> >& tokens,
                                              int fieldIndex,
                                              char separator = ' ');
      void              getTokens          (vector<string>& tokens,
                                              int fieldIndex,
                                              char separator = ' ');
      char*             getToken           (Array<char>& buffer,
                                              int fieldIndex,
                                              int tokenIndex,
                                              int buffersize = -1,
                                              char separator = ' ');
      int               getType            (void) const;

      int               isData             (void) const;

      int               isEmpty            (void) const;
      int               isGlobalComment    (void) const;
      int               isBibliographic    (void) const;
      int               isReferenceRecord  (void) const
                              { return isBibliographic(); }
      int               isReference        (void) const
                              { return isBibliographic(); }
      int               isMeasure          (void) const;
      int               isBarline          (void) const { return isMeasure(); }
      int               isLocalComment     (void) const;
      int               isComment          (void) const;
      int               isExInterp         (int index, const char* string);
      int               isKern             (int index);
      int               hasSpines          (void) const;
      int               isSpineLine        (void) { return hasSpines(); }
      int               isNullToken        (int index);
      int               hasNoteAttack      (int field);
      int               isRest             (int field);


      int               dataQ              (void) const;
      int               measureQ           (void) const;
      int               localCommentQ      (void) const;
      int               globalCommentQ     (void) const;
      int               bibRecordQ         (void) const;
      int               interpretationQ    (void) const;
      int               nullQ              (void) const;
      int               hasExclusiveQ      (void) const;
      int               hasPathQ           (void) const;

      // Interpretation record functions:
      int               isInterpretation   (void) const;
      int               isExclusiveInterpretation (int index) const;
      int               isExInterp         (int index) const
			      { return isExclusiveInterpretation(index); }
      int               isTandem           (void) const;
      int               isSpineManipulator (int index);
      int               isSpineManipulator (void);
      int               isSpineManip       (void) { return isSpineManipulator(); }
      int               hasSpineManip      (void) { return isSpineManipulator(); }

      int               isClef             (int index);
      int               isOriginalClef     (int index);
      int               isKey              (int index);
      int               isKeySig           (int index);
      int               isTempo            (int index);
      int               isTimeSig          (int index);
      int               isMetSig           (int index);
      int               isTranspose        (int index);
      int               isInstrumentType   (int index);
      int               isInstrumentClass  (int index);
      int               isInstrumentName   (int index);
      int               isInstrumentAbbr   (int index);
      int               isInstrumentNum    (int index);
      int               isLabelExpansion   (int index);
      int               isLabelVariant     (int index);
      int               isLabelMarker      (int index);
      int               isStaffNumber      (int index);
      int               isSysStaffNumber   (int index);

      int               isAllClef             (void);
      int               isAllOriginalClef     (void);
      int               isAllKey              (void);
      int               isAllKeySig           (void);
      int               isAllTempo            (void);
      int               isAllTimeSig          (void);
      int               isAllMetSig           (void);
      int               isAllTranspose        (void);
      int               isAllInstrumentType   (void);
      int               isAllInstrumentClass  (void);
      int               isAllInstrumentName   (void);
      int               isAllInstrumentAbbr   (void);
      int               isAllInstrumentNum    (void);
      int               isAllLabelExpansion   (void);
      int               isAllLabelVariant     (void);
      int               isAllLabelMarker      (void);
      int               isAllStaffNumber      (void);
      int               isAllSysStaffNumber   (void);

      int               isNull                (void);

      void              insertField        (int index, const char* aField,
                                              int anInterp = E_unknown,
                                              const char* spinetrace = "");
      void              insertField        (int index, const char* aField,
                                              const char* anInterp,
                                              const char* spinetrace = "");
      void              setSize            (int asize);
      void              setAllFields       (const char* astring);
      HumdrumRecord&    operator=          (const HumdrumRecord& aRecord);
      HumdrumRecord&    operator=          (const HumdrumRecord* aRecord);
      HumdrumRecord&    operator=          (const char* aRecord);
      const char*       operator[]         (int index) const;
      void              setAbsBeat         (double aValue);
      void              setAbsBeat         (int top, int bottom);
      void              setAbsBeatR        (int top, int bottom);
      void              setAbsBeat         (const RationalNumber& aValue);
      void              setAbsBeatR        (const RationalNumber& aValue);
      void              setBeat            (double aValue);
      void              setBeat            (int top, int bottom);
      void              setBeatR           (int top, int bottom);
      void              setBeat            (const RationalNumber& aValue);
      void              setBeatR           (const RationalNumber& aValue);
      void              setDotLine         (int index, int value);
      void              setDotSpine        (int index, int value);
      void              setDuration        (double aValue);
      void              setDuration        (int top, int bottom);
      void              setDurationR       (int top, int bottom);
      void              setDuration        (RationalNumber aValue);
      void              setDurationR       (RationalNumber aValue);
      void              setExInterp        (int fieldIndex, int interpretation);
      void              setExInterp        (int fieldIndex,
                                              const char* interpretation);
      void              setLine            (const char* aString);
      void              setToken           (int index, const char* aString);
      void              setToken           (int index, const string& aString);
      void              setLineNum         (int aLine);
      void              setSpineID         (int index, const char* anID);
      void              setSpineWidth      (int aSize);


   protected:
      int                  lineno;         // line number of record in a file
      int                  type;           // category of humdrum record
      int                  spinewidth;     // for size of spines in comments
      char*                recordString;   // record string
      int                  modifiedQ;      // boolen for if need to make Rstring
      SigCollection<char*> recordFields;   // data for humdrum text record
      vector<string>       spineids;       // spine tracing ids
      Array<int>           interpretation; // exclusive interpretation of data

      Array<int>           dotline;        // for resolving meaning of "."'s
      Array<int>           dotspine;       // for resolving meaning of "."'s


      // data storage for rhythmic analysis in relation to entire Humdrum File.
      float             duration;       // duration of the record
      RationalNumber    durationR;      // duration of the record
      float             meterloc;       // metric position of the record
      RationalNumber    meterlocR;      // metric position of the record
      float             absloc;         // absolute beat location of the record
      RationalNumber    abslocR;        // absolute beat location of the record

      // private functions
      int               determineFieldCount(const char* aLine) const;
      int               determineType      (const char* aLine) const;
      void              makeRecordString   (void);
      void              storeRecordFields  (void);
      int               isParticularType   (const char* regexp,
                                            const char* exinterp);
};


ostream& operator<<(ostream& out, HumdrumRecord& aRecord);


#endif  /* _HUMDRUMRECORD_H_INCLUDED */



