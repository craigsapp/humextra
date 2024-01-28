//
// Copyright 2002 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu Feb 14 23:40:51 PST 2002
// Last Modified: Fri Jun 12 22:58:34 PDT 2009 Renamed SigCollection class
// Last Modified: Sat Aug 25 18:20:06 PDT 2012 Renovated
// Filename:      ...sig/src/sigInfo/ScorePageBase.h
// Web Address:   http://sig.sapp.org/include/sigInfo/ScorePageBase.h
// Syntax:        C++
//
// Description:   Base class for ScorePage.  This class handles reading/writing
//                of a ScorePage, and handles all of the data variables for
//                a ScorePage.
//

#ifndef _SCOREPAGEBASE_H_INCLUDED
#define _SCOREPAGEBASE_H_INCLUDED

#include "ScoreRecord.h"

#include <iostream>
#include <vector>

using namespace std;


class ScorePageBase {
   public:
                     ScorePageBase     (void);
                     ScorePageBase     (ScorePageBase& aPage);
                    ~ScorePageBase     ();

      ScorePageBase& operator=         (ScorePageBase& aPage);
      void           clear             (void);
      void           clearPrintVariables(void);
      void           clearAll          (void);
      void           invalidateAnalyses(void);
   private:
      void           initializeTrailer (long serial = 0x50504153);

   public:

   // file reading and writing functions:
      void           readFile          (const char* filename, int verboseQ = 0);
      void           readFile          (std::istream& infile, int verboseQ = 0);
      void           readAscii         (const char* filename, int verboseQ = 0);
      void           readAscii         (std::istream& infile, int verboseQ = 0);
      void           readAsciiScoreLine(std::istream& infile, ScoreRecord& record,
                                          int verboseQ = 0);
      void           readBinary        (const char* filename, int verboseQ = 0);
      void           readBinary        (std::istream& infile, int verboseQ = 0);
      void           writeBinary       (const char* filename);
      std::ostream&  writeBinary       (std::ostream& outfile);
      void           writeBinary2Byte  (const char* filename);
      void           writeBinary4Byte  (const char* filename);
      void           printAscii        (std::ostream& out, int roundQ = 1,
		                        int verboseQ = 0);
      void           printAsciiWithExtraParameters(std::ostream& out, int roundQ,
                                        int verboseQ);

   private:
      float          readLittleFloat   (std::istream& instream);
      int            readLittleShort   (std::istream& input);
      void           writeLittleFloat  (std::ostream& out, float number);

   public:
   // data access/manipulation functions
      int            getSize            (void);
      ScoreRecord&   operator[]         (int index);

      void           appendItem         (ScoreRecord& aRecord);
      void           appendItem         (ScorePageBase& aPage);
      void           appendItem         (SigCollection<ScoreRecord>& recs);
      void           appendItem         (SigCollection<ScoreRecord*>& recs);

      void           addItem(ScoreRecord& aRecord) { appendItem(aRecord); };
      void           addItem(ScorePageBase& aPage) { appendItem(aPage); };
      void           addItem            (SigCollection<ScoreRecord>& recs)
                                           { appendItem(recs); };
   private:
      void           shrinkParameters   (void);

   public:
   // trailer access functions:
      void           setVersion         (float value);
      void           setVersionWinScore (void);
      float          getVersion         (void);
      void           setSerial          (long value);
      long           getSerial          (void);

   // basic analysis function (sorting).
      void           analyzeSort                (void);
      void           createDefaultPrintSortOrder(void);
      void           createLineStaffSequence    (void);

      ScoreRecord&   getItemByPrintOrder        (int index);
      int            getItemIndexByPrintOrder   (int index);

   protected:
      void           sortByHpos           (Array<int>& items);
      void           quickSortByDataIndex (Array<int>& indexes, int starti,
                                           int endi);

      int            isGreater           (int a, int b);
      int            isLess              (int a, int b);

      //////////////////////////////
      //
      // (1) Main data storage --
      //

      // The data array contains a list of all SCORE items on the page
      // in the order in which they were read from the input file.
      std::vector<ScoreRecord*> m_data;

      // Variable "trailer" is for storing the trailer of a SCORE binary file.
      // The trailer consists of at least 5 floats.  The numbers in
      // reverse order are:
      // 0: The last number is -9999.0 to indicate the end of the trailer
      // 1: The second to last number is a count of the number of 4-byte
      //    numbers in the trailer.  Typically this is 5.0, but may be
      //    larger in new versions of SCORE.
      // 2: The measurement unit code: 0.0 = inches, 1.0 = centimeters.
      // 3: The program version number which created the file.
      // 4: The program serial number (or 0.0 for version 3 or earlier)
      // 5: The last number in the trailer (i.e., the first number of the
      //    trailer in the file is 0.0.  Normally this is the position in
      //    the file which the parameter count for an item is given.
      //    Objects cannot have zero parameters, so when 0.0 is found,
      //    that indicates the start of the trailer.
      // The trailer is stored in the order that it is found in the binary
      // file (or the trailer will be empty if the data was read from an
      // ASCII PMX file).  In other words, -9999.0 should be the last
      // number in the trailer array if the data was read from a binary
      // SCORE file.
      Array<double> trailer;

      //////////////////////////////
      //
      // (2) Page Print Parameters -- These parameters are needed for printing
      //     a page on paper (via PostScript), or to be able to calculate the
      //     location of items in a bitmapped image of the page.
      //

      // The rotate variable is for setting the orientation of the music
      // on the page.  Currently this variable is fixed to portrait.  If
      // rotate = 1, then rotate 90 degrees (clockwise?) so that the page
      // is in landscape mode.
      int rotate;

      // The musicSize variable is used to scale the music on the page.
      // The default value is 1.0.  If the value is 0.5, then the music
      // will be scaled by 50%.  The origin of the scaling is the point
      // where P3=0.0 and P4=0.0 for staff 1 on the page (bottom left
      // corner of music excluding page margins).
      int scale;

      // The SCORE print menu selects the paper type.  In this case
      // the paper type will be described as two variables.  The width
      // of the page (pageWidth) and the height of the page (pageHeight),
      // both stored in units of inches.  These variables are not
      // really needed for printing, but the pageHeibght is needed when
      // converting/calculating bitmap images, since the origin for
      // PostScript is the bottom left corner of the page, while for bitmaps
      // the origin is the top left corner.
      double pageWidth;
      double pageHeight;

      // The left margin setting in the SCORE print menu (parameter 4,
      // first number).  This is the distance from the left margin of the
      // page to the P3=0.0 point on the staff, not including an extra
      // width (called Lbuffer below) of 0.025 inches.  The default setting
      // for this variable in SCORE is 0.5 inches (which is the default
      // value for instances of this class).
      double lmargin;

      // The bottom margin setting in the SCORE print menu (parameter 4,
      // second number).  This is the distance from the bottom margin of the
      // page to the bottom line of the first staff when its P4=0.  This
      // bottom margin does not include and extra width (called Bbuffer
      // below) of 0.0625 inches.  The default setting for this variable
      // in SCORE is 0.75 inches (which is the default value for instances
      // of this class).
      double bmargin;

      // The resolution print setting is for calculating the width of a line
      // in physical units.  The line width is specified as a pixel width.
      // tDPI is equivalent to the RESOLUTION parameter from the SCORE
      // print menu. The oDPI variable is the actually desired DPI for a
      // bitmap image of the page (not necessarily the same as tDPI, but
      // it usually is).  The default RESOLUTION is 600 dots per inch.
      int tDPI;
      int oDPI;

      // The lineWidth variable is the pixel width of lines in SCORE.  This
      // controls the width of staff lines in particular, but also the width
      // of stems, and strokes around all graphical objects other than beams.
      // (Beams are handled from a setting in the SCORE preferences file
      // found in the lib directory (prefs-4.scr).  The default value is
      // 4.0 pixels when the RESOLUTION is 600.
      double lineWidth;

      // The mirror variable is used to control the reversal of the page
      // printing.  This is used for printing music backwards for photo
      // engraving.  Current cannot be changed from regular orientation.
      // If mirror == 1, then the music will be reverse from left to right.
      int mirror;

      // The setStroke variable is used to control insertion of setAdjustStroke
      // functionality into the PostScript output.  This is used to generate
      // uniform line thicknesses in bitmap renderings of the PostScript
      // output.  By default this option is turned on when printing in
      // SCORE.  setStroke == 0 will turn off the setStroke option.
      int setStroke;


      /// Printing Constants //////////////////////////////////////////////
      //
      // These are variables which cannot be changed with the SCORE
      // editor, so they are preserved here as constantns.
      //

      static double Lbuffer;  // = 0.025;
      static double Bbuffer;  // = 0.0625;
      static double StaffLen; // = 7.5;
      // Newer compilers:
      // static constexpr double Lbuffer;  // = 0.025;
      // static constexpr double Bbuffer;  // = 0.0625;
      // static constexpr double StaffLen; // = 7.5;
      static const int    sDPI = 4000;

      //////////////////////////////
      //
      // (3) Sequence Data -- Basic array organization of data on page
      //                      sorted by horizontal position on staff/system
      //
      // Indexing of data in various configurations with scope of line, page
      // and pageset.  These will be filled by ScorePage::analayzeContent().
      //
      // pagePrintSequence -- Sorted by order found in data file.
      // lineStaffSequence -- Indexed by staff P2 value (note offset from 1
      // 		 	at bottom of page).  Sorted horizontally.
      // lineSystemSequence -- Indexed by system number (note offset from 0)
      //                       at top of page). Sorted horizontally.
      //
      // The following two arrays sort by time on the page:
      //
      // pageSystemSequence -- Sorted by lineSystem then horizontally.
      // pageStaffSequence  -- Indexed by system staff (note offset from 0
      //                       at bottom of system).  Sorted horizontally.
      //

      // pagePrintSequence is an array of item in the order in which they
      // are to be printed.  When reading a file, the print order will be
      // defined by the order in which the items are read from the file.
      // In WinScore there is a parameter for print layer which is not
      // considered in the code yet.
      Array<int> pagePrintSequence;

      // lineStaffSequence is a array of items sorted left to right for
      // each staff on the page.  The first index number matches the P2 staff
      // number of the SCORE item.  Note that the lineStaffSequence[0] list
      // is not used (possibly might be mapped to hidden parts on the line).
      // The size is lineStaffSequence is set to 100 staves.  If a staff does
      // not have any items on it, it will have a size of 0;
      Array<Array<int> > lineStaffSequence;

      // lineSystemSequence is similar to lineStaffSequence, but sorts all
      // staves in the given system line from left to right.  The first index
      // number matches to the system line on the page.  The first system line
      // (index 0) is at the top of the page, and the last is at the bottom
      // of the page.  Each system contains a grouping of staves which are
      // analyzed based on barlines in the analyzeSystems() function.
      Array<Array<int> > lineSystemSequence;

      // lineSystemStaffSequence is a array of items sorted left to right for
      // each staff on each system on the page.  The first index number matches
      // the system number on the page (from the top of the page down). The
      // second index number matches staff number f the SCORE item.
      Array<Array<Array<int> > > lineSystemStaffSequence;

      // pageSystemSequence is an ordering of items as a single stream of
      // individual system lines (lineSystemSequence).  This is a concatenation
      // of lineSystemSequence data into a single dimension.
      Array<int> pageSystemSequence;

      // pageStaffSequence is an ordering of items similar to
      // pageSystemSequence, but separated out by invidual system staves.
      // The first index value is the system staff index (not the P4 staff
      // number on the page).
      Array<Array<int> > pageStaffSequence;


      //////////////////////////////
      //
      // (4) Lookup tables -- Indexing mappings of staves and systems on a page.
      //

      // maxStaffNumber contains the largest staff number found on the page.
      int maxStaffNumber;

      // pageStaffList indexes the number of staves on the page.  Its size
      // may be smaller than maxStaffNumber if there are non-consecutive
      // staff numbers.  The staves are indexed from 0 starting at the bottom
      // of the page (pageStaffList[0] is the P2 value of the lowest staff
      // on the page) going upwards (pageStaffList.last() is the P2 value
      // of the highest staff on the page.
      Array<int> pageStaffList;

      // pageStaffListReverse is a reverse mapping of P2 value to consecutive
      // staff index.  The size of pageStaffListReverse is 100 elements, with
      // invalid/unused staves indicated by -1 reverse mapping.
      Array<int> pageStaffListReverse;

      // itemSystemStaffIndex -- reverse lookup of lineStaffSequence
      Array<int> itemSystemStaffIndex;

      // staffToSystemIdx -- mapping from pageStaff to systemIndex
      // The size of this list is 100 elements.  Staves which are not
      // associated with a staff (such as invisible staves and staves
      // which do not exist in the data) are set to -1.  The system
      // index starts at 0 for the top system of the page and increases
      // downward.
      Array<int> P2ToSystemIdx;

      // staffToSystemStaffIdx -- mapping from P2 to systemStaff
      // This list stores the mapping of P2 staff number to system
      // staff index.
      Array<int> P2ToSystemStaffIdx;

      // systemStaffIdxToP2
      Array<Array<int> > systemStaffIdxToP2;

      /////////////////////////////
      //
      // (5) Analytic data -- filled by ScorePage class.
      //

      // systemRhythm -- durnation from start of system to systemObjects item
      Array<Array<double> > systemRhythm;

      // systemDuration -- durations of each system (P7 values of rests & notes)
      Array<double> systemDuration;

      // pageDuration -- sum of systemDurations
      double pageDuration;


      /////////////////////////////
      //
      // (6) Analytic booleans --
      //

      // sortAnalysisQ: used to check if basic sorting was done.  Affects
      // the variables pagePrintSequence and lineStaffSequence.
      int sortAnalysisQ;

      // printAnalysisQ: used to create the printing order.  This is
      // usually the order of the data in the input file.  However, the
      // print order can be modified by other member functions, or could
      // be refined to conform with the WinScore print layer parameter.
      int printAnalysisQ;

      int systemAnalysisQ;       // for checking if system analysis was done
      int pitchAnalysisQ;        // for checking if pitch  analysis was done
      int rhythmAnalysisQ;       // for checking if rhythm analysis was done

      /////////////////////////////
      //
      // (7) Lyrics --
      //

      Array<int> pageStaffLyrics;
      Array<int> sysStaffLyrics;

   public:

      //////////////////////
      //
      // Item sorting functions for use with qsort().
      //

      static int     compareStaff        (const void* A, const void* B);
      static int     compareSystem       (const void* A, const void* B);
      static int     compareSystemIndexes(const void* A, const void* B);
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

#endif /* _SCOREPAGEBASE_H_INCLUDED */


// md5sum: c3202867c565c42152c072c1ae65ffbe ScorePageBase.h [20050403]
