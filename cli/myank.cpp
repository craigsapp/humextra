//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Dec 26 17:03:54 PST 2010
// Last Modified: Fri Jan 14 17:06:32 PST 2011 Added --mark and --mdsep
// Last Modified: Wed Feb  2 12:13:11 PST 2011 Added *met extraction
// Last Modified: Mon Apr  1 00:28:01 PDT 2013 Enabled multiple segment input
// Last Modified: Tue Feb 23 04:40:04 PST 2016 Added --section option
// Filename:      ...sig/examples/all/myank.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/myank.cpp
// Syntax:        C++; museinfo
//
// Description:   Extract measures from input data.
//

#include "humdrum.h"
#include "PerlRegularExpression.h"
#include "MuseData.h"

#include <string.h>
#include <math.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace std;


class Coord {
   public:
           Coord(void) { clear(); }
      void clear(void) { x = y = -1; }
      int isValid(void) { return ((x < 0) || (y < 0)) ? 0 : 1; }
      int x;
      int y;
};

ostream& operator<<(ostream& out, Coord& value) {
   out << "(" << value.x << "," << value.y << ")";
   return out;
}

class MeasureInfo {
   public:
      MeasureInfo(void) { clear(); }
      void clear(void)  { num = seg = start = stop = -1;
         sclef.setSize(0); skeysig.setSize(0); skey.setSize(0);
         stimesig.setSize(0); smet.setSize(0); stempo.setSize(0);
         eclef.setSize(0); ekeysig.setSize(0); ekey.setSize(0);
         etimesig.setSize(0); emet.setSize(0); etempo.setSize(0);
         file = NULL;
      }
      void setTrackCount(int tcount) {
         sclef.setSize(tcount+1);
         skeysig.setSize(tcount+1);
         skey.setSize(tcount+1);
         stimesig.setSize(tcount+1);
         smet.setSize(tcount+1);
         stempo.setSize(tcount+1);
         eclef.setSize(tcount+1);
         ekeysig.setSize(tcount+1);
         ekey.setSize(tcount+1);
         etimesig.setSize(tcount+1);
         emet.setSize(tcount+1);
         etempo.setSize(tcount+1);
         int i;
         for (i=0; i<tcount+1; i++) {
            sclef[i].clear();
            skeysig[i].clear();
            skey[i].clear();
            stimesig[i].clear();
            smet[i].clear();
            stempo[i].clear();
            eclef[i].clear();
            ekeysig[i].clear();
            ekey[i].clear();
            etimesig[i].clear();
            emet[i].clear();
            etempo[i].clear();
         }
         tracks = tcount;
      }
      int num;          // measure number
      int seg;          // measure segment
      int start;        // starting line of segment
      int stop;         // ending line of segment
      int tracks;       // number of primary tracks in file.
      HumdrumFile* file;

      // musical settings at start of measure
      Array<Coord> sclef;     // starting clef of segment
      Array<Coord> skeysig;   // starting keysig of segment
      Array<Coord> skey;      // starting key of segment
      Array<Coord> stimesig;  // starting timesig of segment
      Array<Coord> smet;      // starting met of segment
      Array<Coord> stempo;    // starting tempo of segment

      // musical settings at start of measure
      Array<Coord> eclef;     // ending clef    of segment
      Array<Coord> ekeysig;   // ending keysig  of segment
      Array<Coord> ekey;      // ending key     of segment
      Array<Coord> etimesig;  // ending timesig of segment
      Array<Coord> emet;      // ending met     of segment
      Array<Coord> etempo;    // ending tempo   of segment
};


ostream& operator<<(ostream& out, MeasureInfo& info) {
   if (info.file == NULL) {
      return out;
   }
   HumdrumFile& infile = *(info.file);
   out << "================================== " << endl;
   out << "NUMBER         = " << info.num << endl;
   out << "SEGMENT        = " << info.seg << endl;
   out << "START          = " << info.start << endl;
   out << "STOP           = " << info.stop << endl;

   int i;
   for (i=1; i<info.sclef.getSize(); i++) {
      out << "TRACK " << i << ":" << endl;
      if (info.sclef[i].isValid()) {
         out << "   START CLEF    = " << infile[info.sclef[i].x][info.sclef[i].y]       << endl;
      }
      if (info.skeysig[i].isValid()) {
         out << "   START KEYSIG  = " << infile[info.skeysig[i].x][info.skeysig[i].y]   << endl;
      }
      if (info.skey[i].isValid()) {
         out << "   START KEY     = " << infile[info.skey[i].x][info.skey[i].y]         << endl;
      }
      if (info.stimesig[i].isValid()) {
         out << "   START TIMESIG = " << infile[info.stimesig[i].x][info.stimesig[i].y] << endl;
      }
      if (info.smet[i].isValid()) {
         out << "   START MET     = " << infile[info.smet[i].x][info.smet[i].y]         << endl;
      }
      if (info.stempo[i].isValid()) {
         out << "   START TEMPO   = " << infile[info.stempo[i].x][info.stempo[i].y]     << endl;
      }

      if (info.eclef[i].isValid()) {
         out << "   END CLEF    = " << infile[info.eclef[i].x][info.eclef[i].y]       << endl;
      }
      if (info.ekeysig[i].isValid()) {
         out << "   END KEYSIG  = " << infile[info.ekeysig[i].x][info.ekeysig[i].y]   << endl;
      }
      if (info.ekey[i].isValid()) {
         out << "   END KEY     = " << infile[info.ekey[i].x][info.ekey[i].y]         << endl;
      }
      if (info.etimesig[i].isValid()) {
         out << "   END TIMESIG = " << infile[info.etimesig[i].x][info.etimesig[i].y] << endl;
      }
      if (info.emet[i].isValid()) {
         out << "   END MET     = " << infile[info.emet[i].x][info.emet[i].y]         << endl;
      }
      if (info.etempo[i].isValid()) {
         out << "   END TEMPO   = " << infile[info.etempo[i].x][info.etempo[i].y]     << endl;
      }
   }

   return out;
}




//////////////////////////////////////////////////////////////////////////

// function declarations:
void      checkOptions         (Options& opts, int argc, char** argv);
void      example              (void);
void      usage                (const char* command);
void      myank                (HumdrumFile& infile,
                                Array<MeasureInfo>& outmeasure);
void      removeDollarsFromString(Array<char>& buffer, int maxx);
void      processFieldEntry    (Array<MeasureInfo>& field, const char* string,
                                HumdrumFile& infile, int maxmeasure,
                                Array<MeasureInfo>& inmeasures,
                                Array<int>& inmap);
void      expandMeasureOutList   (Array<MeasureInfo>& measureout,
                                Array<MeasureInfo>& measurein,
                                HumdrumFile& infile, const char* optionstring);
void      getMeasureStartStop  (Array<MeasureInfo>& measurelist,
                                HumdrumFile& infile);
void      printEnding          (HumdrumFile& infile, int lastline, int adjlin);
void      printStarting        (HumdrumFile& infile);
void      reconcileSpineBoundary(HumdrumFile& infile, int index1, int index2);
void      reconcileStartingPosition(HumdrumFile& infile, int index2);
void      printJoinLine        (Array<int>& splits, int index, int count);
void      printInvisibleMeasure(HumdrumFile& infile, int line);
void      fillGlobalDefaults   (HumdrumFile& infile,
                                Array<MeasureInfo>& measurein,
                                Array<int>& inmap);
void      adjustGlobalInterpretations(HumdrumFile& infile, int ii,
                                Array<MeasureInfo>& outmeasures, int index);
void      adjustGlobalInterpretationsStart(HumdrumFile& infile, int ii,
                                Array<MeasureInfo>& outmeasures, int index);
void      getMarkString        (ostream& out, HumdrumFile& infile);
void      printDoubleBarline   (HumdrumFile& infile, int line);
void      insertZerothMeasure  (Array<MeasureInfo>& measurelist,
                                HumdrumFile& infile);
void      getMetStates         (Array<Array<Coord> >& metstates,
                                HumdrumFile& infile);
Coord     getLocalMetInfo      (HumdrumFile& infile, int row, int track);
int       atEndOfFile          (HumdrumFile& infile, int line);
void      processFile          (HumdrumFile& infile, int segmentCount);
int       getSectionCount      (HumdrumFile& infile);
void      getSectionString     (string& sstring, HumdrumFile& infile, int sec);


// User interface variables:
Options options;
int    debugQ      = 0;            // used with --debug option
int    inputlist   = 0;            // used with --inlist option
int    inlistQ     = 0;            // used with --inlist option
int    outlistQ    = 0;            // used with --outlist option
int    verboseQ    = 0;            // used with -v option
int    invisibleQ  = 1;            // used with --visible option
int    maxQ        = 0;            // used with --max option
int    minQ        = 0;            // used with --min option
int    instrumentQ = 0;            // used with -I option
int    nolastbarQ  = 0;            // used with -B option
int    markQ       = 0;            // used with --mark option
int    doubleQ     = 0;            // used with --mdsep option
int    barnumtextQ = 0;            // used with -T option
int    Section     = 0;            // used with --section option
int    sectionCountQ = 0;          // used with --section-count option
Array<MeasureInfo> MeasureOutList; // used with -m option
Array<MeasureInfo> MeasureInList;  // used with -m option

Array<Array<Coord> > metstates;

//////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
   HumdrumFileSet infiles;

   // initial processing of the command-line options
   checkOptions(options, argc, argv);
   infiles.read(options);

   int i;
   for (i=0; i<infiles.getCount(); i++) {
      processFile(infiles[i], infiles.getCount());
   }

   return 0;
}



////////////////////////
//
// processFile --
//

void processFile(HumdrumFile& infile, int segmentCount) {
   if (sectionCountQ) {
      int sections = getSectionCount(infile);
      cout << sections << endl;
      return;
   }

   getMetStates(metstates, infile);
   getMeasureStartStop(MeasureInList, infile);

   Array<char> measurestring;
   measurestring.setSize(strlen(options.getString("measure").c_str())+1);
   strcpy(measurestring.getBase(), options.getString("measure").c_str());
   if (markQ) {
      stringstream mstring;
      getMarkString(mstring, infile);
      mstring << ends;
      measurestring.setSize(strlen(mstring.str().c_str())+1);
      strcpy(measurestring.getBase(), mstring.str().c_str());
      if (debugQ) {
         cout << "MARK STRING: " << mstring.str().c_str() << endl;
      }
   } else if (Section) {
      string sstring;
      getSectionString(sstring, infile, Section);
      measurestring.setSize(sstring.size()+1);
      strcpy(measurestring.getBase(), sstring.c_str());
   }
   if (debugQ) {
      cout << "MARK MEASURES: " << measurestring << endl;
   }

   // expand to multiple measures later.
   expandMeasureOutList(MeasureOutList, MeasureInList, infile,
         measurestring.getBase());

   if (inlistQ) {
      cout << "INPUT MEASURE MAP: " << endl;
      for (int i=0; i<MeasureInList.getSize(); i++) {
         cout << MeasureInList[i];
      }
   }
   if (outlistQ) {
      cout << "OUTPUT MEASURE MAP: " << endl;
      for (int i=0; i<MeasureOutList.getSize(); i++) {
         cout << MeasureOutList[i];
      }
   }

   if (MeasureOutList.getSize() == 0) {
      // disallow processing files with no barlines
      return;
   }

   if (segmentCount > 1) {
      infile.printNonemptySegmentLabel(cout);
   }
   myank(infile, MeasureOutList);
}


//////////////////////////////////////////////////////////////////////////


////////////////////////
//
// getMetStates --  Store the current *met for every token
// in the score, keeping track of meter without metric symbols.
//

void getMetStates(Array<Array<Coord> >& metstates, HumdrumFile& infile) {
   Array<Coord> current;
   current.setSize(infile.getMaxTracks()+1);
   metstates.setSize(infile.getNumLines());
   PerlRegularExpression pre;

   int i, j, track;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isInterpretation()) {
         for (j=0; j<infile[i].getFieldCount(); j++) {
            track = infile[i].getPrimaryTrack(j);
            if (pre.search(infile[i][j], "^\\*met\\([^\\)]+\\)", "")) {
               current[track].x = i;
               current[track].y = j;
            } else if (pre.search(infile[i][j], "^\\*M\\d+\\d+", "")) {
               current[track] = getLocalMetInfo(infile, i, track);
            }
         }
      }

      // metstates[i].setSize(infile[i].getFieldCount());
      // for (j=0; j<infile[i].getFieldCount(); j++) {
      //    track = infile[i].getPrimaryTrack(j);
      //    metstates[i][j] = current[track];
      // }
      metstates[i].setSize(infile.getMaxTracks()+1);
      for (j=1; j<=infile.getMaxTracks(); j++) {
         metstates[i][j] = current[j];
      }
   }

   if (debugQ) {
      for (i=0; i<infile.getNumLines(); i++) {
         for (j=1; j<metstates[i].getSize(); j++) {
            if (metstates[i][j].x < 0) {
               cout << ".";
            } else {
               cout << infile[metstates[i][j].x][metstates[i][j].y];
            }
            cout << "\t";
         }
         cout << infile[i] << endl;
      }

   }
}



//////////////////////////////
//
// getLocalMetInfo -- search in the non-data region indicated by the
// input row for a *met entry in the input track.  Return empty
// value if none found.
//

Coord getLocalMetInfo(HumdrumFile& infile, int row, int track) {
   Coord output;
   int startline = -1;
   int stopline = -1;
   int i = row;
   int j;
   int xtrac;
   PerlRegularExpression pre;

   while (i>=0) {
      if (infile[i].isData()) {
         startline = i+1;
         break;
      }
      i--;
   }
   if (startline < 0) {
      startline = 0;
   }
   i = row;
   while (i<infile.getNumLines()){
      if (infile[i].isData()) {
         stopline = i-1;
         break;
      }
      i++;
   }
   if (stopline >= infile.getNumLines()) {
      stopline = infile.getNumLines()-1;
   }
   for (i=startline; i<=stopline; i++) {
      if (!infile[i].isInterpretation()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         xtrac = infile[i].getPrimaryTrack(j);
         if (track != xtrac) {
            continue;
         }
         if (pre.search(infile[i][j], "^\\*met\\([^\\)]+\\)", "")) {
            output.x = i;
            output.x = j;
         }
      }

   }

   return output;
}



//////////////////////////////
//
// getMarkString -- return a list of measures which contain marked
//    notes (primarily from search matches).
// This function scans for reference records in this form:
// !!!RDF**kern: @= matched note
// or
// !!!RDF**kern: i= marked note
// If it finds any lines like that, it will extract the character before
// the equals sign, and scan for it in the **kern data in the file.
// any measure which contains such a mark will be stored in the output
// string.
//

void getMarkString(ostream& out, HumdrumFile& infile)  {
   Array<char> mchar; // list of characters which are marks
   mchar.setSize(0);
   int i, j, k, m;
   char target;
   PerlRegularExpression pre;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isBibliographic()) {
         continue;
      }
      if (pre.search(infile[i][0],
            "!!!RDF\\*\\*kern\\s*:\\s*([^=])\\s*=\\s*match", "i")) {
         target = pre.getSubmatch(1)[0];
         mchar.append(target);
      } else if (pre.search(infile[i][0],
            "!!!RDF\\*\\*kern\\s*:\\s*([^=])\\s*=\\s*mark", "i")) {
         target = pre.getSubmatch(1)[0];
         mchar.append(target);
      }
   }

   if (debugQ) {
      for (i=0; i<mchar.getSize(); i++) {
         cout << "\tMARK CHARCTER: " << mchar[i] << endl;
      }
   }

   if (mchar.getSize() == 0) {
      return;
   }

   // now search for measures which contains any of those character
   // in **kern data:
   int curmeasure = 0;
   int inserted = 0;
   int hasmark = 0;
   const char* str;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isMeasure()) {
         if (pre.search(infile[i][0], "^=.*?(\\d+)", "")) {
            curmeasure = atoi(pre.getSubmatch(1));
            hasmark = 0;
         }
      }
      if (hasmark) {
         continue;
      }
      if (!infile[i].isData()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (infile[i].isExInterp(j, "**kern")) {
            k=0;
            str = infile[i][j];
            while (str[k] != '\0') {
               for (m=0; m<mchar.getSize(); m++) {
                  if (str[k] == mchar[m]) {
                     if (inserted) {
                        out << ',';
                     } else {
                        inserted++;
                     }
                     out << curmeasure;
                     hasmark = 1;
                     goto outerforloop;
                  }
               }
               k++;
            }
         }
      }
outerforloop: ;
   }
}



//////////////////////////////
//
// myank -- yank the specified measures.
//

void myank(HumdrumFile& infile, Array<MeasureInfo>& outmeasures) {
   if (outmeasures.getSize() > 0) {
      printStarting(infile);
   }

   int lastline = -1;
   int h, i, j;
   int counter;
   int printed = 0;
   int mcount = 0;
   int measurestart = 1;
   int datastart = 0;
   int bartextcount = 0;
   for (h=0; h<outmeasures.getSize(); h++) {
      measurestart = 1;
      printed = 0;
      counter = 0;
      if (debugQ) {
         cout << "!! =====================================\n";
         cout << "!! processing " << outmeasures[h].num << endl;
      }
      if (h > 0) {
         reconcileSpineBoundary(infile, outmeasures[h-1].stop,
            outmeasures[h].start);
      } else {
         reconcileStartingPosition(infile, outmeasures[0].start);
      }
      for (i=outmeasures[h].start; i<outmeasures[h].stop; i++) {
         counter++;
         if ((!printed) && ((mcount == 0) || (counter == 2))) {
            if ((datastart == 0) && outmeasures[h].num == 0) {
               // not ideal setup...
               datastart = 1;
            } else{
               adjustGlobalInterpretations(infile, i, outmeasures, h);
               printed = 1;
            }
         }
         if (infile[i].isData() && (mcount == 0)) {
            mcount++;
         }
         if (infile[i].isMeasure()) {
            mcount++;
         }
         if ((mcount == 1) && invisibleQ && infile[i].isMeasure()) {
            printInvisibleMeasure(infile, i);
            measurestart = 0;
            if ((bartextcount++ == 0) && infile[i].isMeasure()) {
               int barline = 0;
               sscanf(infile[i][0], "=%d", &barline);
               if (barnumtextQ && (barline > 0)) {
                  cout << "!!LO:TX:Z=20:X=-90:t=" << barline << endl;
               }
            }
         } else if (doubleQ && measurestart) {
            printDoubleBarline(infile, i);
            measurestart = 0;
         } else {
            cout << infile[i] << "\n";
            if (barnumtextQ && (bartextcount++ == 0) && infile[i].isMeasure()) {
               int barline = 0;
               sscanf(infile[i][0], "=%d", &barline);
               if (barline > 0) {
                  cout << "!!LO:TX:Z=20:X=-25:t=" << barline << endl;
               }
            }
         }
         lastline = i;
      }
   }

   PerlRegularExpression pre;
   Array<char> token;
   int lasti;
   if (outmeasures.getSize() > 0) {
      lasti = outmeasures.last().stop;
   } else {
      lasti = -1;
   }
   if ((!nolastbarQ) &&  (lasti >= 0) && infile[lasti].isMeasure()) {
      for (j=0; j<infile[lasti].getFieldCount(); j++) {
         token.setSize(strlen(infile[lasti][j])+1);
         strcpy(token.getBase(), infile[lasti][j]);
         pre.sar(token, "\\d+", "", "");
         if (doubleQ) {
            if (pre.search(token, "=(.+)")) {
               // don't add double barline, there is already
               // some style on the barline
            } else {
               // add a double barline
               pre.sar(token, "$", "||", "");
            }
         }
         cout << token;
         if (j < infile[lasti].getFieldCount() - 1) {
            cout << '\t';
         }
      }
      cout << '\n';
   }

   if (debugQ) {
      cout << "PROCESSING ENDING" << endl;
   }

   if (lastline >= 0) {
      //printEnding(infile, lastline);
      printEnding(infile, outmeasures.last().stop, lasti);
   }

}



//////////////////////////////
//
// adjustGlobalInterpretations --
//

void adjustGlobalInterpretations(HumdrumFile& infile, int ii,
      Array<MeasureInfo>& outmeasures, int index) {

   if (index <= 0) {
      adjustGlobalInterpretationsStart(infile, ii, outmeasures, index);
      return;
   }

   // the following lines will not work when non-contiguous measures are
   // elided.
   //   if (!infile[ii].isInterpretation()) {
   //      return;
   //   }

   int i;

   int clefQ    = 0;
   int keysigQ  = 0;
   int keyQ     = 0;
   int timesigQ = 0;
   int metQ     = 0;
   int tempoQ   = 0;

   int x, y;
   int xo, yo;

   int tracks = infile.getMaxTracks();

   // these lines may cause bugs, but they get rid of zeroth measure
   // problem.
// ggg
//   if ((outmeasures.getSize() > 1) && (outmeasures[index-1].num == 0)) {
//      return;
//   }
//   if ((outmeasures.getSize() > 0) && (outmeasures[index].num == 0)) {
//      return;
//   }

   for (i=1; i<=tracks; i++) {

      if (!clefQ && (outmeasures[index].sclef.getSize() > 0)) {
         x  = outmeasures[index].sclef[i].x;
         y  = outmeasures[index].sclef[i].y;
         xo = outmeasures[index-1].eclef[i].x;
         yo = outmeasures[index-1].eclef[i].y;
         if ((x>=0)&&(y>=0)&&(xo>=0)&&(yo>=0)) {
            if (strcmp(infile[x][y], infile[xo][yo]) != 0) {
               clefQ = 1;
            }
         }
      }

      if (!keysigQ && (outmeasures[index].skeysig.getSize() > 0)) {
         x  = outmeasures[index].skeysig[i].x;
         y  = outmeasures[index].skeysig[i].y;
         xo = outmeasures[index-1].ekeysig[i].x;
         yo = outmeasures[index-1].ekeysig[i].y;
         if ((x>=0)&&(y>=0)&&(xo>=0)&&(yo>=0)) {
            if (strcmp(infile[x][y], infile[xo][yo]) != 0) {
               keysigQ = 1;
            }
         }
      }

      if (!keyQ && (outmeasures[index].skey.getSize() > 0)) {
         x  = outmeasures[index].skey[i].x;
         y  = outmeasures[index].skey[i].y;
         xo = outmeasures[index-1].ekey[i].x;
         yo = outmeasures[index-1].ekey[i].y;
         if ((x>=0)&&(y>=0)&&(xo>=0)&&(yo>=0)) {
            if (strcmp(infile[x][y], infile[xo][yo]) != 0) {
               keyQ = 1;
            }
         }
      }

      if (!timesigQ && (outmeasures[index].stimesig.getSize() > 0)) {
         x  = outmeasures[index].stimesig[i].x;
         y  = outmeasures[index].stimesig[i].y;
         xo = outmeasures[index-1].etimesig[i].x;
         yo = outmeasures[index-1].etimesig[i].y;
         if ((x>=0)&&(y>=0)&&(xo>=0)&&(yo>=0)) {
            if (strcmp(infile[x][y], infile[xo][yo]) != 0) {
               timesigQ = 1;
            }
         }
      }

      if (!metQ && (outmeasures[index].smet.getSize() > 0)) {
         x  = outmeasures[index].smet[i].x;
         y  = outmeasures[index].smet[i].y;
         xo = outmeasures[index-1].emet[i].x;
         yo = outmeasures[index-1].emet[i].y;
         if ((x>=0)&&(y>=0)&&(xo>=0)&&(yo>=0)) {
            if (strcmp(infile[x][y], infile[xo][yo]) != 0) {
               metQ = 1;
            }
         }
      }

      if (!tempoQ && (outmeasures[index].stempo.getSize() > 0)) {
         x  = outmeasures[index].stempo[i].x;
         y  = outmeasures[index].stempo[i].y;
         xo = outmeasures[index-1].etempo[i].x;
         yo = outmeasures[index-1].etempo[i].y;
         if ((x>=0)&&(y>=0)&&(xo>=0)&&(yo>=0)) {
            if (strcmp(infile[x][y], infile[xo][yo]) != 0) {
               tempoQ = 1;
            }
         }
      }
   }

   int track;

   if (clefQ) {
      for (i=0; i<infile[ii].getFieldCount(); i++) {
         track = infile[ii].getPrimaryTrack(i);
         x  = outmeasures[index].sclef[track].x;
         y  = outmeasures[index].sclef[track].y;
         xo = outmeasures[index-1].eclef[track].x;
         yo = outmeasures[index-1].eclef[track].y;
         if ((x>=0)&&(y>=0)&&(xo>=0)&&(yo>=0)) {
            if (strcmp(infile[x][y], infile[xo][yo]) != 0) {
               cout << infile[x][y];
            } else {
               cout << "*";
            }
         } else {
            cout << "*";
         }
         if (i < infile[ii].getFieldCount()-1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }

   if (keysigQ) {
      for (i=0; i<infile[ii].getFieldCount(); i++) {
         track = infile[ii].getPrimaryTrack(i);
         x  = outmeasures[index].skeysig[track].x;
         y  = outmeasures[index].skeysig[track].y;
         xo = outmeasures[index-1].ekeysig[track].x;
         yo = outmeasures[index-1].ekeysig[track].y;
         if ((x>=0)&&(y>=0)&&(xo>=0)&&(yo>=0)) {
            if (strcmp(infile[x][y], infile[xo][yo]) != 0) {
               cout << infile[x][y];
            } else {
               cout << "*";
            }
         } else {
            cout << "*";
         }
         if (i < infile[ii].getFieldCount()-1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }

   if (keyQ) {
      for (i=0; i<infile[ii].getFieldCount(); i++) {
         track = infile[ii].getPrimaryTrack(i);
         x  = outmeasures[index].skey[track].x;
         y  = outmeasures[index].skey[track].y;
         xo = outmeasures[index-1].ekey[track].x;
         yo = outmeasures[index-1].ekey[track].y;
         if ((x>=0)&&(y>=0)&&(xo>=0)&&(yo>=0)) {
            if (strcmp(infile[x][y], infile[xo][yo]) != 0) {
               cout << infile[x][y];
            } else {
               cout << "*";
            }
         } else {
            cout << "*";
         }
         if (i < infile[ii].getFieldCount()-1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }

   if (timesigQ) {
      for (i=0; i<infile[ii].getFieldCount(); i++) {
         track = infile[ii].getPrimaryTrack(i);
         x  = outmeasures[index].stimesig[track].x;
         y  = outmeasures[index].stimesig[track].y;
         xo = outmeasures[index-1].etimesig[track].x;
         yo = outmeasures[index-1].etimesig[track].y;
         if ((x>=0)&&(y>=0)&&(xo>=0)&&(yo>=0)) {
            if (strcmp(infile[x][y], infile[xo][yo]) != 0) {
               cout << infile[x][y];
            } else {
               cout << "*";
            }
         } else {
            cout << "*";
         }
         if (i < infile[ii].getFieldCount()-1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }

   if (metQ) {
      for (i=0; i<infile[ii].getFieldCount(); i++) {
         track = infile[ii].getPrimaryTrack(i);
         x  = outmeasures[index].smet[track].x;
         y  = outmeasures[index].smet[track].y;
         xo = outmeasures[index-1].emet[track].x;
         yo = outmeasures[index-1].emet[track].y;
         if ((x>=0)&&(y>=0)&&(xo>=0)&&(yo>=0)) {
            if (strcmp(infile[x][y], infile[xo][yo]) != 0) {
               cout << infile[x][y];
            } else {
               cout << "*";
            }
         } else {
            cout << "*";
         }
         if (i < infile[ii].getFieldCount()-1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }

   if (tempoQ) {
      for (i=0; i<infile[ii].getFieldCount(); i++) {
         track = infile[ii].getPrimaryTrack(i);
         x  = outmeasures[index].stempo[track].x;
         y  = outmeasures[index].stempo[track].y;
         xo = outmeasures[index-1].etempo[track].x;
         yo = outmeasures[index-1].etempo[track].y;
         if ((x>=0)&&(y>=0)&&(xo>=0)&&(yo>=0)) {
            if (strcmp(infile[x][y], infile[xo][yo]) != 0) {
               cout << infile[x][y];
            } else {
               cout << "*";
            }
         } else {
            cout << "*";
         }
         if (i < infile[ii].getFieldCount()-1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }

}



//////////////////////////////
//
// adjustGlobalInterpretationsStart --
//

void adjustGlobalInterpretationsStart(HumdrumFile& infile, int ii,
      Array<MeasureInfo>& outmeasures, int index) {
   if (index != 0) {
      cerr << "Error in adjustGlobalInterpetationsStart" << endl;
      exit(1);
   }

   int i;

   int clefQ    = 0;
   int keysigQ  = 0;
   int keyQ     = 0;
   int timesigQ = 0;
   int metQ     = 0;
   int tempoQ   = 0;

   int x, y;

   // ignore the zeroth measure
   // (may not be proper).
// ggg
   if (outmeasures[index].num == 0) {
      return;
   }

   int tracks = infile.getMaxTracks();

   for (i=1; i<=tracks; i++) {

      if (!clefQ) {
         x  = outmeasures[index].sclef[i].x;
         y  = outmeasures[index].sclef[i].y;

         if ((x>=0)&&(y>=0)) {
            clefQ = 1;
         }
      }

      if (!keysigQ) {
         x  = outmeasures[index].skeysig[i].x;
         y  = outmeasures[index].skeysig[i].y;
         if ((x>=0)&&(y>=0)) {
            keysigQ = 1;
         }
      }

      if (!keyQ) {
         x  = outmeasures[index].skey[i].x;
         y  = outmeasures[index].skey[i].y;
         if ((x>=0)&&(y>=0)) {
            keyQ = 1;
         }
      }

      if (!timesigQ) {
         x  = outmeasures[index].stimesig[i].x;
         y  = outmeasures[index].stimesig[i].y;
         if ((x>=0)&&(y>=0)) {
            timesigQ = 1;
         }
      }

      if (!metQ) {
         x  = outmeasures[index].smet[i].x;
         y  = outmeasures[index].smet[i].y;
         if ((x>=0)&&(y>=0)) {
            metQ = 1;
         }
      }

      if (!tempoQ) {
         x  = outmeasures[index].stempo[i].x;
         y  = outmeasures[index].stempo[i].y;
         if ((x>=0)&&(y>=0)) {
            tempoQ = 1;
         }
      }
   }

   int ptrack;

   if (clefQ) {
      for (i=0; i<infile[ii].getFieldCount(); i++) {
         ptrack = infile[ii].getPrimaryTrack(i);
         x  = outmeasures[index].sclef[ptrack].x;
         y  = outmeasures[index].sclef[ptrack].y;
         if ((x>=0)&&(y>=0)) {
            cout << infile[x][y];
         } else {
            cout << "*";
         }
         if (i < infile[ii].getFieldCount()-1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }

   if (keysigQ) {
      for (i=0; i<infile[ii].getFieldCount(); i++) {
         ptrack = infile[ii].getPrimaryTrack(i);
         x  = outmeasures[index].skeysig[ptrack].x;
         y  = outmeasures[index].skeysig[ptrack].y;
         if ((x>=0)&&(y>=0)) {
            cout << infile[x][y];
         } else {
            cout << "*";
         }
         if (i < infile[ii].getFieldCount()-1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }

   if (keyQ) {
      for (i=0; i<infile[ii].getFieldCount(); i++) {
         ptrack = infile[ii].getPrimaryTrack(i);
         x  = outmeasures[index].skey[ptrack].x;
         y  = outmeasures[index].skey[ptrack].y;
         if ((x>=0)&&(y>=0)) {
            cout << infile[x][y];
         } else {
            cout << "*";
         }
         if (i < infile[ii].getFieldCount()-1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }

   if (timesigQ) {
      for (i=0; i<infile[ii].getFieldCount(); i++) {
         ptrack = infile[ii].getPrimaryTrack(i);
         x  = outmeasures[index].stimesig[ptrack].x;
         y  = outmeasures[index].stimesig[ptrack].y;
         if ((x>=0)&&(y>=0)) {
            cout << infile[x][y];
         } else {
            cout << "*";
         }
         if (i < infile[ii].getFieldCount()-1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }

   if (metQ) {
      for (i=0; i<infile[ii].getFieldCount(); i++) {
         ptrack = infile[ii].getPrimaryTrack(i);
         x  = outmeasures[index].smet[ptrack].x;
         y  = outmeasures[index].smet[ptrack].y;
         if ((x>=0)&&(y>=0)) {
            cout << infile[x][y];
         } else {
            cout << "*";
         }
         if (i < infile[ii].getFieldCount()-1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }

   if (tempoQ) {
      for (i=0; i<infile[ii].getFieldCount(); i++) {
         ptrack = infile[ii].getPrimaryTrack(i);
         x  = outmeasures[index].stempo[ptrack].x;
         y  = outmeasures[index].stempo[ptrack].y;
         if ((x>=0)&&(y>=0)) {
            cout << infile[x][y];
         } else {
            cout << "*";
         }
         if (i < infile[ii].getFieldCount()-1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }
}



//////////////////////////////
//
// printDoubleBarline --
//

void printDoubleBarline(HumdrumFile& infile, int line) {


   if (!infile[line].isMeasure()) {
      cout << infile[line] << "\n";
      return;
   }

   PerlRegularExpression pre;
   int j;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (pre.search(infile[line][j], "(=\\d*)(.*)", "")) {
         cout << pre.getSubmatch(1);
         cout << "||";
      } else {
         cout << "=||";
      }
      if (j < infile[line].getFieldCount()-1) {
         cout << "\t";
      }
   }
   cout << "\n";

   if (barnumtextQ) {
      int barline = 0;
      sscanf(infile[line][0], "=%d", &barline);
      if (barline > 0) {
         cout << "!!LO:TX:Z=20:X=-25:t=" << barline << endl;
      }
   }

}



//////////////////////////////
//
// printInvisibleMeasure --
//

void printInvisibleMeasure(HumdrumFile& infile, int line) {
   if (!infile[line].isMeasure()) {
      cout << infile[line] << "\n";
      return;
   }

   PerlRegularExpression pre;
   int j;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (strchr(infile[line][j], '-') != NULL) {
         cout << infile[line][j];
         if (j < infile[line].getFieldCount()-1) {
            cout << "\t";
         }
         continue;
      }
      if (pre.search(infile[line][j], "(=\\d*)(.*)", "")) {
         cout << pre.getSubmatch(1);
         cout << "-";
         cout << pre.getSubmatch(2);
      } else {
         cout << infile[line][j];
      }
      if (j < infile[line].getFieldCount()-1) {
         cout << "\t";
      }
   }
   cout << "\n";
}



//////////////////////////////
//
// reconcileSpineBoundary -- merge spines correctly between segments.
//    will not be able to handle all permutations of spine manipulators.
//    So don't expect exotic manipulators to work...
//

void reconcileSpineBoundary(HumdrumFile& infile, int index1, int index2) {

   if (debugQ) {
      cout << "RECONCILING LINES " << index1+1 << " and " << index2+1 << endl;
      cout << "FIELD COUNT OF " << index1+1 << " is "
           << infile[index1].getFieldCount() << endl;
      cout << "FIELD COUNT OF " << index2+1 << " is "
           << infile[index2].getFieldCount() << endl;
   }

   // check to see if any changes need reconciling; otherwise, exit function
   int i, j;
   if (infile[index1].getFieldCount() == infile[index2].getFieldCount()) {
      int same = 1;
      for (i=0; i<infile[index1].getFieldCount(); i++) {
         if (infile[index1].getSpineInfo(i) != infile[index2].getSpineInfo(i)) {
            same = 0;
         }
      }
      if (same != 0) {
         return;
      }
   }

   // handle splits all at once
   char buff1[1024] = {0};
   char buff2[1024] = {0};

   Array<int> splits(infile[index1].getFieldCount());
   splits.setAll(0);

   int hassplit = 0;
   for (i=0; i<infile[index1].getFieldCount(); i++) {
      strcpy(buff1, "(");
      strcat(buff1, infile[index1].getSpineInfo(i).c_str());
      strcat(buff1, ")");
      strcpy(buff2, buff1);
      strcat(buff1, "a");
      strcat(buff2, "b");
      for (j=0; j<infile[index2].getFieldCount()-1; j++) {
         if ((strcmp(buff1, infile[index2].getSpineInfo(j).c_str()) == 0)
               && (strcmp(buff2, infile[index2].getSpineInfo(j+1).c_str()) == 0)) {
            splits[i] = 1;
            hassplit++;
         }
      }
   }

   if (hassplit) {
      for (i=0; i<splits.getSize(); i++) {
         if (splits[i]) {
            cout << "*^";
         } else {
            cout << '*';
         }
         if (i < splits.getSize()-1) {
            cout << '\t';
         }
      }
      cout << '\n';
   }

   // make splits cumulative;
   //for (i=1; i<splits.getSize(); i++) {
   //   splits[i] += splits[i-1];
   //}

   PerlRegularExpression pre1;
   PerlRegularExpression pre2;
   // handle joins one at a time, only look for binary joins at the moment.
   // assuming that no *x has been used to mix the voices up.
   for (i=0; i<infile[index1].getFieldCount()-1; i++) {
      if (!pre1.search(infile[index1].getSpineInfo(i), "\\((.*)\\)a")) {
         continue;
      }
      if (!pre2.search(infile[index1].getSpineInfo(i+1), "\\((.*)\\)b")) {
         continue;
      }
      if (strcmp(pre1.getSubmatch(1), pre2.getSubmatch(1)) != 0) {
         // spines are not split from same source
         continue;
      }

      // found an "a" and "b" portion of a spine split, now search
      // through the target line for a joint of those two sub-spines
      for (j=0; j<infile[index2].getFieldCount(); j++) {
         if (strcmp(infile[index2].getSpineInfo(j).c_str(), pre1.getSubmatch()) != 0) {
            continue;
         }
         // found a simple binary spine join: emmit a spine manipulator line
         printJoinLine(splits, i, 2);
      }
   }

   // handle *x switches, not perfect since ordering might need to be
   // handled between manipulators...

}



//////////////////////////////
//
// printJoinLine -- count is currently ignored, but may in the future
//    allow for more than two spines to join at the same time.
//

void printJoinLine(Array<int>& splits, int index, int count) {
   int i;
   for (i=0; i<splits.getSize(); i++) {
      if (i == index) {
         cout << "*v\t*v";
         i+=count-1;
      } else {
         cout << "*";
      }
      if (i<splits.getSize()-1) {
         cout << "\t";
      }
   }
   cout << "\n";

   // merge splits by one element
   for (i=index+1; i<splits.getSize()-1; i++) {
      splits[i] = splits[i+1];
   }
   splits.setSize(splits.getSize()-1);
}



//////////////////////////////
//
// reconcileStartingPosition -- merge spines from start of data and
//    first measure in output.
//

void reconcileStartingPosition(HumdrumFile& infile, int index2) {
   int i;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isInterpretation()) {
         reconcileSpineBoundary(infile, i, index2);
         break;
      }
   }
}



//////////////////////////////
//
// printStarting -- print header information before start of data.
//

void printStarting(HumdrumFile& infile) {
   int i, j;
   int exi = -1;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isInterpretation()) {
         // the first interpretation is the exclusive one
         cout << infile[i] << "\n";
         exi = i;
         break;
      }
      cout << infile[i] << "\n";
   }

   int hasI = 0;

   if (instrumentQ) {
      // print any tandem interpretations which start with *I found
      // at the start of the data before measures, notes, or any
      // spine manipulator lines
      for (i=exi+1; i<infile.getNumLines(); i++) {
         if (infile[i].isData()) {
            break;
         }
         if (infile[i].isMeasure()) {
            break;
         }
         if (!infile[i].isInterpretation()) {
            continue;
         }
         if (infile[i].isSpineManipulator()) {
            break;
         }
         hasI = 0;
         for (j=0; j<infile[i].getFieldCount(); j++) {
            if (strncmp(infile[i][j], "*I", 2) == 0) {
               hasI = 1;
               break;
            }
         }
         if (hasI) {
            for (j=0; j<infile[i].getFieldCount(); j++) {
               if (strncmp(infile[i][j], "*I", 2) == 0) {
                  cout << infile[i][j];
               } else {
                  cout << "*";
               }
               if (j < infile[i].getFieldCount() - 1) {
                  cout << "\t";
               }
            }
            cout << "\n";
         }
      }
   }

}



//////////////////////////////
//
// printEnding -- print the measure
//

void printEnding(HumdrumFile& infile, int lastline, int adjlin) {
   if (debugQ) {
      cout << "IN printEnding" << endl;
   }
   int ending = -1;
   int marker = -1;
   int i;
   for (i=infile.getNumLines()-1; i>=0; i--) {
      if (infile[i].isInterpretation() && (ending <0)
            && (strcmp(infile[i][0], "*-") == 0)) {
         ending = i;
      }
      if (infile[i].isData()) {
         marker = i+1;
         break;
      }
      if (infile[i].isMeasure()) {
         marker = i+1;
         break;
      }
   }

   if (ending >= 0) {
      reconcileSpineBoundary(infile, adjlin, ending);
   }

   int startline  = ending;
   if (marker >= 0) {
      // capture any comment which occur after the last measure
      // line in the data.
      startline = marker;
   }

   // reconcileSpineBoundary(infile, lastline, startline);

   if (startline >= 0) {
      for (i=startline; i<infile.getNumLines(); i++) {
         cout << infile[i] << "\n";
      }
   }

}



//////////////////////////////
//
// getMeasureStartStop --  Get a list of the (numbered) measures in the
//    input file, and store the start/stop lines for those measures.
//    All data before the first numbered measure is in measure 0.
//    although, if the first measure is not labeled, then ...
//

void getMeasureStartStop(Array<MeasureInfo>& measurelist, HumdrumFile& infile) {
   measurelist.setSize(infile.getNumLines());
   measurelist.setSize(0);

   MeasureInfo current;
   int i, ii;
   int lastend = -1;
   int dataend = -1;
   int barnum1 = -1;
   int barnum2 = -1;
   PerlRegularExpression pre;

   insertZerothMeasure(measurelist, infile);

   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isInterpretation()) {
         if (strcmp(infile[i][0], "*-") == 0) {
            dataend = i;
            break;
         }
      }
      if (!infile[i].isMeasure()) {
         continue;
      }
      //if (!pre.search(infile[i][0], "^=.*(\\d+)")) {
      //   continue;
      //}
      //barnum1 = atoi(pre.getSubmatch(1));
      if (!sscanf(infile[i][0], "=%d", &barnum1)) {
         continue;
      }
      current.clear();
      current.start = i;
      current.num   = barnum1;
      for (ii=i+1; ii<infile.getNumLines(); ii++) {
         if (!infile[ii].isMeasure()) {
            continue;
         }
         //if (pre.search(infile[ii][0], "^=.*(\\d+)")) {
         //   barnum2 = atoi(pre.getSubmatch(1));
         //   current.stop = ii;
         //   lastend = ii;
         //   i = ii - 1;
         //   measurelist.append(current);
         //   break;
         //}
         if (pre.search(infile[ii][0], "=[^\\d]*(\\d+)")) {
         // if (sscanf(infile[ii][0], "=%d", &barnum2)) {
            barnum2 = atoi(pre.getSubmatch(1));
            current.stop = ii;
            lastend = ii;
            i = ii - 1;
            current.file = &infile;
            measurelist.append(current);
            break;
         } else {
            if (atEndOfFile(infile, ii)) {
               break;
            }
         }
      }
   }

   int lastdata    = -1;   // last line in file with data
   int lastmeasure = -1;   // last line in file with measure

   for (i=infile.getNumLines()-1; i>=0; i--) {
      if ((lastdata < 0) && infile[i].isData()) {
         lastdata = i;
      }
      if ((lastmeasure < 0) && infile[i].isMeasure()) {
         lastmeasure = i;
      }
      if ((lastmeasure >= 0) && (lastdata >= 0)) {
         break;
      }
   }

   if (lastmeasure < lastdata) {
      // no final barline, so set to ignore
      lastmeasure = -1;
      lastdata    = -1;
   }

   if ((barnum2 >= 0) && (lastend >= 0) && (dataend >= 0)) {
      current.clear();
      current.num = barnum2;
      current.start = lastend;
      current.stop = dataend;
      if (lastmeasure > lastdata) {
         current.stop = lastmeasure;
      }
      current.file = &infile;
      measurelist.append(current);
   }


}



//////////////////////////////
//
// getSectionCount -- Count the number of sections in a file according to
//     JRP rules: sections are defined by double barlines. There may be some
//     corner cases to consider.
//

int getSectionCount(HumdrumFile& infile) {
   int i;
   int count = 0;
   int dataQ = 0;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!dataQ && infile[i].isData()) {
         dataQ = 1;
         count++;
         continue;
      }
      if (infile[i].isBarline()) {
         if (strstr(infile[i][0], "||")) {
            dataQ = 0;
         }
      }
   }
   return count;
}



//////////////////////////////
//
// getSectionString -- return the measure range of a section.
//

void getSectionString(string& sstring, HumdrumFile& infile, int sec) {
   int i;
   int first = -1;
   int second = -1;
   int barnum = 0;
   int count = 0;
   int dataQ = 0;
   PerlRegularExpression pre;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!dataQ && infile[i].isData()) {
         dataQ = 1;
         count++;
         if (count == sec) {
            first = barnum;
         } else if (count == sec+1) {
            second = barnum - 1;
         }
         continue;
      }
      if (infile[i].isBarline()) {
         if (strstr(infile[i][0], "||")) {
            dataQ = 0;
         }
         if (pre.search(infile[i][0], "(\\d+)")) {
            barnum = atoi(pre.getSubmatch(1));
         }
      }
   }
   if (second < 0) {
      second = barnum;
   }
   char buffer1[32] = {0};
   char buffer2[32] = {0};
   snprintf(buffer1, 32, "%d", first);
   snprintf(buffer2, 32, "%d", second);
   sstring = buffer1;
   sstring += "-";
   sstring += buffer2;
}



//////////////////////////////
//
// atEndOfFile --
//

int atEndOfFile(HumdrumFile& infile, int line) {
   int i;
   for (i=line+1; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         return 1;
      }
   }

   return 0;
}



//////////////////////////////
//
// insertZerothMeasure --
//

void insertZerothMeasure(Array<MeasureInfo>& measurelist,
      HumdrumFile& infile) {

   PerlRegularExpression pre;
   int exinterpline = -1;
   int startline = -1;
   int stopline = -1;
   int i;
   for (i=9; i<infile.getNumLines(); i++) {
      if ((exinterpline < 0) && infile[i].isInterpretation()) {
         exinterpline = i;
      }
      if ((startline < 0) && (infile[i].isData())) {
         startline = i;
      }
      if (infile[i].isMeasure() && pre.search(infile[i][0], "^=.*\\d+", "")) {
         stopline = i;
         break;
      }
   }

   if (exinterpline < 0) {
      // somethind weird happend, just return
      return;
   }
   if (startline < 0) {
      // no zeroth measure;
      return;
   }
   if (stopline < 0) {
      // strange situation, no measure numbers
      // consider what to do later...
      return;
   }

   MeasureInfo current;
   current.clear();
   current.num = 0;
   // current.start = startline;
   current.start = exinterpline+1;
   current.stop = stopline;
   measurelist.append(current);
}



//////////////////////////////
//
// expandMeasureOutList -- read the measure list for the sequence of measures
//     to extract.
//

void expandMeasureOutList(Array<MeasureInfo>& measureout,
      Array<MeasureInfo>& measurein, HumdrumFile& infile,
      const char* optionstring) {

   PerlRegularExpression pre;
   // find the largest measure number in the score
   int maxmeasure = -1;
   int minmeasure = -1;
   int i;
   for (i=0; i<measurein.getSize(); i++) {
      if (maxmeasure < measurein[i].num) {
         maxmeasure = measurein[i].num;
      }
      if ((minmeasure == -1) || (minmeasure > measurein[i].num)) {
         minmeasure = measurein[i].num;
      }
   }
   if (maxmeasure <= 0) {
      cerr << "Error: There are no measure numbers present in the data" << endl;
      exit(1);
   }
   if (maxmeasure > 1123123) {
      cerr << "Error: ridiculusly large measure number: " << maxmeasure << endl;
      exit(1);
   }
   if (maxQ) {
      if (measurein.getSize() == 0) {
         cout << 0 << endl;
      } else {
         cout << maxmeasure << endl;
      }
      exit(0);
   } else if (minQ) {
      int ii;
      for (ii=0; ii<infile.getNumLines(); ii++) {
         if (infile[ii].isMeasure()) {
            if (pre.search(infile[ii][0], "=\\d", "")) {
               break;
            } else {
               cout << 0 << endl;
               exit(0);
            }
         }
         if (infile[ii].isData()) {
            cout << 0 << endl;
            exit(0);
         }
      }
      if (measurein.getSize() == 0) {
         cout << 0 << endl;
      } else {
         cout << minmeasure << endl;
      }
      exit(0);
   }

   // create reverse-lookup list
   Array<int> inmap(maxmeasure+1);
   inmap.setAll(-1);
   for (i=0; i<measurein.getSize(); i++) {
		int number = measurein[i].num;
		if (number >=0) {
      	inmap[number] = i;
		}
   }

   fillGlobalDefaults(infile, measurein, inmap);

   Array <char> ostring;
   ostring.setSize(strlen(optionstring)+1);
   strcpy(ostring.getBase(), optionstring);

   removeDollarsFromString(ostring, maxmeasure);

   if (debugQ) {
      cout << "Option string expanded: " << ostring << endl;
   }

   pre.sar(ostring, "\\s+", "", "g");  // remove any spaces between items.
   pre.sar(ostring, "--+", "-", "g");  // remove extra dashes
   int value = 0;
   int start = 0;
   Array<MeasureInfo>& range = measureout;
   range.setSize(10000);
   range.setSize(0);
   range.setGrowth(5123123);
   value = pre.search(ostring.getBase(), "^([^,]+,?)");
   while (value != 0) {
      start += value - 1;
      start += strlen(pre.getSubmatch(1));
      processFieldEntry(range, pre.getSubmatch(), infile, maxmeasure,
          measurein, inmap);
      value = pre.search(ostring.getBase() + start, "^([^,]+,?)");
   }
}



//////////////////////////////
//
// fillGlobalDefaults -- keep track of the clef, key signature, key, etc.
//

void fillGlobalDefaults(HumdrumFile& infile, Array<MeasureInfo>& measurein,
      Array<int>& inmap) {
   int i, j;
   PerlRegularExpression pre;

   int tracks = infile.getMaxTracks();

   Array<Coord> currclef(tracks+1);
   Array<Coord> currkeysig(tracks+1);
   Array<Coord> currkey(tracks+1);
   Array<Coord> currtimesig(tracks+1);
   Array<Coord> currmet(tracks+1);
   Array<Coord> currtempo(tracks+1);

   Coord undefCoord;
   undefCoord.clear();

   currclef.setAll(undefCoord);
   currkeysig.setAll(undefCoord);
   currkey.setAll(undefCoord);
   currtimesig.setAll(undefCoord);
   currmet.setAll(undefCoord);
   currtempo.setAll(undefCoord);

   int          currmeasure = -1;
   int          lastmeasure = -1;
   int          datafound   = 0;
   int track;
   int thingy = 0;

   for (i=0; i<infile.getNumLines(); i++) {
      if ((currmeasure == -1) && (thingy == 0) && infile[i].isData()) {
         currmeasure = 0;
      }
		if (currmeasure < 0) {
			currmeasure = 0;
		}
      if (infile[i].isMeasure()) {
         if (!pre.search(infile[i][0], "(\\d+)", "")) {
            continue;
         }
         thingy = 1;
			int mnum = inmap[currmeasure];
			if (mnum < 0) {
				mnum = 0;
			}

         // store state of global music values at end of measure
         if (currmeasure >= 0) {
            measurein[mnum].eclef    = currclef;
            measurein[mnum].ekeysig  = currkeysig;
            measurein[mnum].ekey     = currkey;
            measurein[mnum].etimesig = currtimesig;
            measurein[mnum].emet     = currmet;
            measurein[mnum].etempo   = currtempo;
         }

         lastmeasure = currmeasure;
         currmeasure = atoi(pre.getSubmatch(1));

         if ((currmeasure >= 0) && (currmeasure < inmap.getSize())) {
            // [20120818] Had to compensate for last measure being single
            // and un-numbered.
            if (inmap[currmeasure] < 0) {
               // [20111008] Had to compensate for "==85" barline
               datafound = 0;
               break;
            }
            measurein[inmap[currmeasure]].sclef    = currclef;
            measurein[inmap[currmeasure]].skeysig  = currkeysig;
            measurein[inmap[currmeasure]].skey     = currkey;
            measurein[inmap[currmeasure]].stimesig = currtimesig;
            measurein[inmap[currmeasure]].smet     = metstates[i];
            measurein[inmap[currmeasure]].stempo   = currtempo;
         }

         datafound   = 0;
         continue;
      }
      if (infile[i].isInterpretation()) {
         for (j=0; j<infile[i].getFieldCount(); j++) {
            if (!infile[i].isExInterp(j, "**kern")) {
               continue;
            }
            track = infile[i].getPrimaryTrack(j);

            if ((datafound == 0) && (lastmeasure >= 0)) {
               if (strncmp(infile[i][j], "*clef", 5) == 0) {
                  measurein[inmap[currmeasure]].sclef[track].x = -1;
                  measurein[inmap[currmeasure]].sclef[track].y = -1;
               } else if (pre.search(infile[i][j], "^\\*k\\[.*\\]", "")) {
                  measurein[inmap[currmeasure]].skeysig[track].x = -1;
                  measurein[inmap[currmeasure]].skeysig[track].y = -1;
               } else if (pre.search(infile[i][j], "^\\*[A-G][#-]?:", "i")) {
                  measurein[inmap[currmeasure]].skey[track].x = -1;
                  measurein[inmap[currmeasure]].skey[track].y = -1;
               } else if (pre.search(infile[i][j], "^\\*M\\d+/\\d+", "i")) {
                  measurein[inmap[currmeasure]].stimesig[track].x = -1;
                  measurein[inmap[currmeasure]].stimesig[track].y = -1;
                  measurein[inmap[currmeasure]].smet[track].x = -1;
                  measurein[inmap[currmeasure]].smet[track].y = -1;
               } else if (pre.search(infile[i][j], "^\\*MM\\d+", "i")) {
                  measurein[inmap[currmeasure]].stempo[track].x = -1;
                  measurein[inmap[currmeasure]].stempo[track].y = -1;
               }
            }

            if (strncmp(infile[i][j], "*clef", 5) == 0) {
               currclef[track].x = i;
               currclef[track].y = j;
               continue;
            }
            if (pre.search(infile[i][j], "^\\*k\\[.*\\]", "")) {
               currkeysig[track].x = i;
               currkeysig[track].y = j;
               continue;
            }
            if (pre.search(infile[i][j], "^\\*[A-G][#-]?:", "i")) {
               currkey[track].x = i;
               currkey[track].y = j;
               continue;
            }
            if (pre.search(infile[i][j], "^\\*M\\d+/\\d+", "i")) {
               currtimesig[track].x = i;
               currtimesig[track].y = j;
               continue;
            }
            if (pre.search(infile[i][j], "^\\*MM[\\d.]+", "i")) {
               currtempo[track].x = i;
               currtempo[track].y = j;
               continue;
            }

         }
      }
      if (infile[i].isData()) {
         datafound = 1;
      }
   }

   // store state of global music values at end of music
   if ((currmeasure >= 0) && (currmeasure < inmap.getSize())
         && (inmap[currmeasure] >= 0)) {
      measurein[inmap[currmeasure]].eclef    = currclef;
      measurein[inmap[currmeasure]].ekeysig  = currkeysig;
      measurein[inmap[currmeasure]].ekey     = currkey;
      measurein[inmap[currmeasure]].etimesig = currtimesig;
      measurein[inmap[currmeasure]].emet     = currmet;
      measurein[inmap[currmeasure]].etempo   = currtempo;
   }


   // go through the measure list and clean up start/end states
   for (i=0; i<measurein.getSize()-2; i++) {

      if (measurein[i].sclef.getSize() == 0) {
         measurein[i].sclef.setSize(tracks+1);
         measurein[i].sclef.setAll(undefCoord);
      }
      if (measurein[i].eclef.getSize() == 0) {
         measurein[i].eclef.setSize(tracks+1);
         measurein[i].eclef.setAll(undefCoord);
      }
      if (measurein[i+1].sclef.getSize() == 0) {
         measurein[i+1].sclef.setSize(tracks+1);
         measurein[i+1].sclef.setAll(undefCoord);
      }
      if (measurein[i+1].eclef.getSize() == 0) {
         measurein[i+1].eclef.setSize(tracks+1);
         measurein[i+1].eclef.setAll(undefCoord);
      }
      for (j=1; j<measurein[i].sclef.getSize(); j++) {
         if (!measurein[i].eclef[j].isValid()) {
            if (measurein[i].sclef[j].isValid()) {
               measurein[i].eclef[j] = measurein[i].sclef[j];
            }
         }
         if (!measurein[i+1].sclef[j].isValid()) {
            if (measurein[i].eclef[j].isValid()) {
               measurein[i+1].sclef[j] = measurein[i].eclef[j];
            }
         }
      }

      if (measurein[i].skeysig.getSize() == 0) {
         measurein[i].skeysig.setSize(tracks+1);
         measurein[i].skeysig.setAll(undefCoord);
      }
      if (measurein[i].ekeysig.getSize() == 0) {
         measurein[i].ekeysig.setSize(tracks+1);
         measurein[i].ekeysig.setAll(undefCoord);
      }
      if (measurein[i+1].skeysig.getSize() == 0) {
         measurein[i+1].skeysig.setSize(tracks+1);
         measurein[i+1].skeysig.setAll(undefCoord);
      }
      if (measurein[i+1].ekeysig.getSize() == 0) {
         measurein[i+1].ekeysig.setSize(tracks+1);
         measurein[i+1].ekeysig.setAll(undefCoord);
      }
      for (j=1; j<measurein[i].skeysig.getSize(); j++) {
         if (!measurein[i].ekeysig[j].isValid()) {
            if (measurein[i].skeysig[j].isValid()) {
               measurein[i].ekeysig[j] = measurein[i].skeysig[j];
            }
         }
         if (!measurein[i+1].skeysig[j].isValid()) {
            if (measurein[i].ekeysig[j].isValid()) {
               measurein[i+1].skeysig[j] = measurein[i].ekeysig[j];
            }
         }
      }

      if (measurein[i].skey.getSize() == 0) {
         measurein[i].skey.setSize(tracks+1);
         measurein[i].skey.setAll(undefCoord);
      }
      if (measurein[i].ekey.getSize() == 0) {
         measurein[i].ekey.setSize(tracks+1);
         measurein[i].ekey.setAll(undefCoord);
      }
      if (measurein[i+1].skey.getSize() == 0) {
         measurein[i+1].skey.setSize(tracks+1);
         measurein[i+1].skey.setAll(undefCoord);
      }
      if (measurein[i+1].ekey.getSize() == 0) {
         measurein[i+1].ekey.setSize(tracks+1);
         measurein[i+1].ekey.setAll(undefCoord);
      }
      for (j=1; j<measurein[i].skey.getSize(); j++) {
         if (!measurein[i].ekey[j].isValid()) {
            if (measurein[i].skey[j].isValid()) {
               measurein[i].ekey[j] = measurein[i].skey[j];
            }
         }
         if (!measurein[i+1].skey[j].isValid()) {
            if (measurein[i].ekey[j].isValid()) {
               measurein[i+1].skey[j] = measurein[i].ekey[j];
            }
         }
      }

      if (measurein[i].stimesig.getSize() == 0) {
         measurein[i].stimesig.setSize(tracks+1);
         measurein[i].stimesig.setAll(undefCoord);
      }
      if (measurein[i].etimesig.getSize() == 0) {
         measurein[i].etimesig.setSize(tracks+1);
         measurein[i].etimesig.setAll(undefCoord);
      }
      if (measurein[i+1].stimesig.getSize() == 0) {
         measurein[i+1].stimesig.setSize(tracks+1);
         measurein[i+1].stimesig.setAll(undefCoord);
      }
      if (measurein[i+1].etimesig.getSize() == 0) {
         measurein[i+1].etimesig.setSize(tracks+1);
         measurein[i+1].etimesig.setAll(undefCoord);
      }
      for (j=1; j<measurein[i].stimesig.getSize(); j++) {
         if (!measurein[i].etimesig[j].isValid()) {
            if (measurein[i].stimesig[j].isValid()) {
               measurein[i].etimesig[j] = measurein[i].stimesig[j];
            }
         }
         if (!measurein[i+1].stimesig[j].isValid()) {
            if (measurein[i].etimesig[j].isValid()) {
               measurein[i+1].stimesig[j] = measurein[i].etimesig[j];
            }
         }
      }

      if (measurein[i].smet.getSize() == 0) {
         measurein[i].smet.setSize(tracks+1);
         measurein[i].smet.setAll(undefCoord);
      }
      if (measurein[i].emet.getSize() == 0) {
         measurein[i].emet.setSize(tracks+1);
         measurein[i].emet.setAll(undefCoord);
      }
      if (measurein[i+1].smet.getSize() == 0) {
         measurein[i+1].smet.setSize(tracks+1);
         measurein[i+1].smet.setAll(undefCoord);
      }
      if (measurein[i+1].emet.getSize() == 0) {
         measurein[i+1].emet.setSize(tracks+1);
         measurein[i+1].emet.setAll(undefCoord);
      }
      for (j=1; j<measurein[i].smet.getSize(); j++) {
         if (!measurein[i].emet[j].isValid()) {
            if (measurein[i].smet[j].isValid()) {
               measurein[i].emet[j] = measurein[i].smet[j];
            }
         }
         if (!measurein[i+1].smet[j].isValid()) {
            if (measurein[i].emet[j].isValid()) {
               measurein[i+1].smet[j] = measurein[i].emet[j];
            }
         }
      }

      if (measurein[i].stempo.getSize() == 0) {
         measurein[i].stempo.setSize(tracks+1);
         measurein[i].stempo.setAll(undefCoord);
      }
      if (measurein[i].etempo.getSize() == 0) {
         measurein[i].etempo.setSize(tracks+1);
         measurein[i].etempo.setAll(undefCoord);
      }
      if (measurein[i+1].stempo.getSize() == 0) {
         measurein[i+1].stempo.setSize(tracks+1);
         measurein[i+1].stempo.setAll(undefCoord);
      }
      if (measurein[i+1].etempo.getSize() == 0) {
         measurein[i+1].etempo.setSize(tracks+1);
         measurein[i+1].etempo.setAll(undefCoord);
      }
      for (j=1; j<measurein[i].stempo.getSize(); j++) {
         if (!measurein[i].etempo[j].isValid()) {
            if (measurein[i].stempo[j].isValid()) {
               measurein[i].etempo[j] = measurein[i].stempo[j];
            }
         }
         if (!measurein[i+1].stempo[j].isValid()) {
            if (measurein[i].etempo[j].isValid()) {
               measurein[i+1].stempo[j] = measurein[i].etempo[j];
            }
         }
      }


   }

}



//////////////////////////////
//
// processFieldEntry --
//   3-6 expands to 3 4 5 6
//   $   expands to maximum spine track
//   $0  expands to maximum spine track
//   $1  expands to maximum spine track minus 1, etc.
//   2-$1 expands to 2 through the maximum minus one.
//   6-3 expands to 6 5 4 3
//   $2-5 expands to the maximum minus 2 down through 5.
//   Ignore negative values and values which exceed the maximum value.
//

void processFieldEntry(Array<MeasureInfo>& field, const char* string,
     HumdrumFile& infile, int maxmeasure, Array<MeasureInfo>& inmeasures,
     Array<int>& inmap) {

   MeasureInfo current;

   PerlRegularExpression pre;
   Array<char> buffer;
   buffer.setSize(strlen(string)+1);
   strcpy(buffer.getBase(), string);

   // remove any comma left at end of input string (or anywhere else)
   pre.sar(buffer, ",", "", "g");

   if (pre.search(buffer.getBase(), "^(\\d+)[a-z]?-(\\d+)[a-z]?$")) {
      int firstone = strtol(pre.getSubmatch(1), NULL, 10);
      int lastone  = strtol(pre.getSubmatch(2), NULL, 10);

      // limit the range to 0 to maxmeasure
      if (firstone > maxmeasure) { firstone = maxmeasure; }
      if (lastone  > maxmeasure) { lastone  = maxmeasure; }
      if (firstone < 0         ) { firstone = 0         ; }
      if (lastone  < 0         ) { lastone  = 0         ; }

      if ((firstone < 1) && (firstone != 0)) {
         cerr << "Error: range token: \"" << string << "\""
              << " contains too small a number at start: " << firstone << endl;
         cerr << "Minimum number allowed is " << 1 << endl;
         exit(1);
      }
      if ((lastone < 1) && (lastone != 0)) {
         cerr << "Error: range token: \"" << string << "\""
              << " contains too small a number at end: " << lastone << endl;
         cerr << "Minimum number allowed is " << 1 << endl;
         exit(1);
      }

      int i;
      if (firstone > lastone) {
         for (i=firstone; i>=lastone; i--) {
            if (inmap[i] >= 0) {
               if ((field.getSize() > 0) &&
                     (field.last().stop == inmeasures[inmap[i]].start)) {
                  field.last().stop = inmeasures[inmap[i]].stop;
               } else {
                  current.clear();
                  current.file = &infile;
                  current.num = i;
                  current.start = inmeasures[inmap[i]].start;
                  current.stop = inmeasures[inmap[i]].stop;

                  current.sclef    = inmeasures[inmap[i]].sclef;
                  current.skeysig  = inmeasures[inmap[i]].skeysig;
                  current.skey     = inmeasures[inmap[i]].skey;
                  current.stimesig = inmeasures[inmap[i]].stimesig;
                  current.smet     = inmeasures[inmap[i]].smet;
                  current.stempo   = inmeasures[inmap[i]].stempo;

                  current.eclef    = inmeasures[inmap[i]].eclef;
                  current.ekeysig  = inmeasures[inmap[i]].ekeysig;
                  current.ekey     = inmeasures[inmap[i]].ekey;
                  current.etimesig = inmeasures[inmap[i]].etimesig;
                  current.emet     = inmeasures[inmap[i]].emet;
                  current.etempo   = inmeasures[inmap[i]].etempo;

                  field.append(current);
               }
            }
         }
      } else {
         for (i=firstone; i<=lastone; i++) {
            if (inmap[i] >= 0) {
               if ((field.getSize() > 0) &&
                     (field.last().stop == inmeasures[inmap[i]].start)) {
                  field.last().stop = inmeasures[inmap[i]].stop;
               } else {
                  current.clear();
                  current.file = &infile;
                  current.num = i;
                  current.start = inmeasures[inmap[i]].start;
                  current.stop = inmeasures[inmap[i]].stop;

                  current.sclef    = inmeasures[inmap[i]].sclef;
                  current.skeysig  = inmeasures[inmap[i]].skeysig;
                  current.skey     = inmeasures[inmap[i]].skey;
                  current.stimesig = inmeasures[inmap[i]].stimesig;
                  current.smet     = inmeasures[inmap[i]].smet;
                  current.stempo   = inmeasures[inmap[i]].stempo;

                  current.eclef    = inmeasures[inmap[i]].eclef;
                  current.ekeysig  = inmeasures[inmap[i]].ekeysig;
                  current.ekey     = inmeasures[inmap[i]].ekey;
                  current.etimesig = inmeasures[inmap[i]].etimesig;
                  current.emet     = inmeasures[inmap[i]].emet;
                  current.etempo   = inmeasures[inmap[i]].etempo;

                  field.append(current);
               }
            }
         }
      }
   } else if (pre.search(buffer.getBase(), "^(\\d+)([a-z]*)")) {
      int value = strtol(pre.getSubmatch(1), NULL, 10);
      // do something with letter later...

      if ((value < 1) && (value != 0)) {
         cerr << "Error: range token: \"" << string << "\""
              << " contains too small a number at end: " << value << endl;
         cerr << "Minimum number allowed is " << 1 << endl;
         exit(1);
      }
      if (inmap[value] >= 0) {
         if ((field.getSize() > 0) &&
               (field.last().stop == inmeasures[inmap[value]].start)) {
            field.last().stop = inmeasures[inmap[value]].stop;
         } else {
            current.clear();
            current.file = &infile;
            current.num = value;
            current.start = inmeasures[inmap[value]].start;
            current.stop = inmeasures[inmap[value]].stop;

            current.sclef    = inmeasures[inmap[value]].sclef;
            current.skeysig  = inmeasures[inmap[value]].skeysig;
            current.skey     = inmeasures[inmap[value]].skey;
            current.stimesig = inmeasures[inmap[value]].stimesig;
            current.smet     = inmeasures[inmap[value]].smet;
            current.stempo   = inmeasures[inmap[value]].stempo;

            current.eclef    = inmeasures[inmap[value]].eclef;
            current.ekeysig  = inmeasures[inmap[value]].ekeysig;
            current.ekey     = inmeasures[inmap[value]].ekey;
            current.etimesig = inmeasures[inmap[value]].etimesig;
            current.emet     = inmeasures[inmap[value]].emet;
            current.etempo   = inmeasures[inmap[value]].etempo;

            field.append(current);
         }
      }
   }
}



//////////////////////////////
//
// removeDollarsFromString -- substitute $ sign for maximum track count.
//

void removeDollarsFromString(Array<char>& buffer, int maxx) {
   PerlRegularExpression pre;
   PerlRegularExpression pre2;
   char tbuf[1024] = {0};
   char obuf[1024] = {0};
   int outval;
   int value;

   if (debugQ) {
      cout << "MEASURE STRING BEFORE DOLLAR REMOVAL: " << buffer << endl;
   }

   while (pre.search(buffer, "(\\$\\d*)", "")) {
      strcpy(tbuf, pre.getSubmatch(1));
      if (pre2.search(tbuf, "(\\$\\d+)")) {
        sscanf(pre2.getSubmatch(1), "$%d", &value);
        outval = maxx - value;
      } else {
         outval = maxx;
      }

      if (outval < 0) {
         outval = 0;
      }

      snprintf(tbuf, 1024, "%d", outval);
      strcpy(obuf, "\\");
      strcat(obuf, pre.getSubmatch());
      pre.sar(buffer, obuf, tbuf, "");
   }
   if (debugQ) {
      cout << "DOLLAR EXPAND: " << buffer << endl;
   }
}



//////////////////////////////
//
// checkOptions --
//

void checkOptions(Options& opts, int argc, char** argv) {
   opts.define("v|verbose=b",  "Verbose output of data");
   opts.define("debug=b",    "Debugging information");
   opts.define("inlist=b",   "Show input measure list");
   opts.define("outlist=b",  "Show output measure list");
   opts.define("mark|marks=b",    "Yank measure with marked notes");
   opts.define("T|M|bar-number-text=b", "print barnum with LO text above system ");
   opts.define("d|double|dm|md|mdsep|mdseparator=b", "Put double barline between non-consecutive measure segments");
   opts.define("m|b|measures|bars|measure|bar=s", "Measures to yank");
   opts.define("I|i|instrument=b", "Include instrument codes from start of data");
   opts.define("visible|not-invisible=b", "Do not make initial measure invisible");
   opts.define("B|noendbar=b", "Do not print barline at end of data");
   opts.define("max=b",  "print maximum measure number");
   opts.define("min=b",  "print minimum measure number");
   opts.define("section-count=b", "count the number of sections, JRP style");
   opts.define("section=i:0", "extract given section number (indexed from 1");

   opts.define("author=b",    "Program author");
   opts.define("version=b",   "Program version");
   opts.define("example=b",   "Program examples");
   opts.define("h|help=b",    "Short description");
   opts.process(argc, argv);

   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, December 2010" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 26 December 2010" << endl;
      cout << "compiled: " << __DATE__ << endl;
      cout << MUSEINFO_VERSION << endl;
      exit(0);
   } else if (opts.getBoolean("help")) {
      usage(opts.getCommand().c_str());
      exit(0);
   } else if (opts.getBoolean("example")) {
      example();
      exit(0);
   }

   debugQ   = opts.getBoolean("debug");
   inlistQ  = opts.getBoolean("inlist");
   outlistQ = opts.getBoolean("outlist");
   verboseQ = opts.getBoolean("verbose");
   maxQ     = opts.getBoolean("max");
   minQ     = opts.getBoolean("min");

   invisibleQ    = !opts.getBoolean("not-invisible");
   instrumentQ   =  opts.getBoolean("instrument");
   nolastbarQ    =  opts.getBoolean("noendbar");
   markQ         =  opts.getBoolean("mark");
   doubleQ       =  opts.getBoolean("mdsep");
   barnumtextQ   =  opts.getBoolean("bar-number-text");
   sectionCountQ =  opts.getBoolean("section-count");
   Section       =  opts.getInteger("section");

   if (!Section) {
      if (!(opts.getBoolean("measures") || markQ)) {
         // if -m option is not given, then --mark option presumed
         markQ = 1;
         // cerr << "Error: the -m option is required" << endl;
         // exit(1);
      }
   }

}



//////////////////////////////
//
// example -- example function calls to the program.
//

void example(void) {


}



//////////////////////////////
//
// usage -- command-line usage description and brief summary
//

void usage(const char* command) {

}



