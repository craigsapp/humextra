//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Jul 18 11:23:42 PDT 2005
// Last Modified: Tue Sep  1 13:54:42 PDT 2009 Added -f, and -p options
// Last Modified: Wed Sep  2 13:26:58 PDT 2009 Added -t option
// Last Modified: Sat Sep  5 15:21:29 PDT 2009 Added sub-spine features
// Last Modified: Tue Sep  8 11:43:46 PDT 2009 Added co-spine extraction
// Last Modified: Sat Apr  6 00:48:21 PDT 2013 Enabled multiple segment input
// Last Modified: Thu Oct 24 12:32:47 PDT 2013 Switch to HumdrumStream class
// Last Modified: Sat Mar 12 18:34:05 PST 2016 Switch to STL
// Filename:      ...sig/examples/all/extractx.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/extractx.cpp
// Syntax:        C++; museinfo
//
// Description:   C++ implementation of the Humdrum Toolkit extract command.
//
// To do:         Allow *x records to be echoed when using -s 0 insertion
//                Currently spines with *x are unwrapped and the *x is changed
//                to *.

#include "humdrum.h"
#include "PerlRegularExpression.h"

#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;


// function declarations
void    checkOptions            (Options& opts, int argc, char* argv[]);
void    processFile             (HumdrumFile& infile);
void    excludeFields           (HumdrumFile& infile, vector<int>& field,
                                 vector<int>& subfield, vector<int>& model);
void    extractFields           (HumdrumFile& infile, vector<int>& field,
                                 vector<int>& subfield, vector<int>& model);
void    extractTrace            (HumdrumFile& infile, const string& tracefile);
void    getInterpretationFields (vector<int>& field, vector<int>& subfield,
                                 vector<int>& model, HumdrumFile& infile,
                                 string& interps, int state);
//void    extractInterpretations  (HumdrumFile& infile, string& interps);
void    example                 (void);
void    usage                   (const char* command);
void    fillFieldData           (vector<int>& field, vector<int>& subfield,
                                 vector<int>& model, string& fieldstring,
                                 HumdrumFile& infile);
void    processFieldEntry       (vector<int>& field, vector<int>& subfield,
                                 vector<int>& model, const char* astring,
                                 HumdrumFile& infile);
void    removeDollarsFromString (string& buffer, int maxtrack);
int     isInList                (int number, vector<int>& listofnum);
void    getTraceData            (vector<int>& startline,
                                 vector<vector<int> >& fields,
                                 const string& tracefile, HumdrumFile& infile);
void    printTraceLine          (HumdrumFile& infile, int line,
                                 vector<int>& field);
void    dealWithSpineManipulators(HumdrumFile& infile, int line,
                                 vector<int>& field, vector<int>& subfield,
                                 vector<int>& model);
void    storeToken              (vector<string>& storage,
                                 const char* string);
void    storeToken              (vector<string>& storage, int index,
                                 const char* string);
void    printMultiLines         (vector<int>& vsplit, vector<int>& vserial,
                                 vector<string>& tempout);
void    reverseSpines           (vector<int>& field, vector<int>& subfield,
                                 vector<int>& model, HumdrumFile& infile,
                                 const string& exinterp);
void    getSearchPat            (string& spat, int target,
                                 const char* modifier);
void    expandSpines            (vector<int>& field, vector<int>& subfield,
                                 vector<int>& model, HumdrumFile& infile,
                                 string& interp);
void    dealWithSecondarySubspine(vector<int>& field, vector<int>& subfield,
                                 vector<int>& model, int targetindex,
                                 HumdrumFile& infile, int line, int spine,
                                 int submodel);
void    dealWithCospine         (vector<int>& field, vector<int>& subfield,
                                 vector<int>& model, int targetindex,
                                 HumdrumFile& infile, int line, int cospine,
                                 int comodel, int submodel,
                                 const string& cointerp);
void    printCotokenInfo        (int& start, HumdrumFile& infile, int line,
                                 int spine, vector<string>& cotokens,
                                 vector<int>& spineindex,
                                 vector<int>& subspineindex);
void    fillFieldDataByGrep     (vector<int>& field, vector<int>& subfield,
                                 vector<int>& model, const string& grepString,
                                 HumdrumFile& infile, int state);

// global variables
Options      options;            // database for command-line arguments
int          excludeQ = 0;       // used with -x option
int          expandQ  = 0;       // used with -e option
string       expandInterp = "";  // used with -E option
int          interpQ  = 0;       // used with -i option
string       interps  = "";      // used with -i option
int          debugQ   = 0;       // used with --debug option
int          fieldQ   = 0;       // used with -f or -p option
string       fieldstring = "";   // used with -f or -p option
vector<int>   field;              // used with -f or -p option
vector<int>   subfield;           // used with -f or -p option
vector<int>   model;              // used with -p, or -e options and similar
int          countQ   = 0;       // used with -C option
int          traceQ   = 0;       // used with -t option
string       tracefile = "";     // used with -t option
int          reverseQ = 0;       // used with -r option
string       reverseInterp = "**kern"; // used with -r and -R options.
// sub-spine "b" expansion model: how to generate data for a secondary
// spine if the primary spine is not divided.  Models are:
//    'd': duplicate primary spine (or "a" subspine) data (default)
//    'n': null = use a null token
//    'r': rest = use a rest instead of a primary spine note (in **kern)
//         data.  'n' will be used for non-kern spines when 'r' is used.
int          submodel = 'd';     // used with -m option
const char* editorialInterpretation = "yy";
string      cointerp = "**kern";   // used with -c option
int         comodel  = 0;          // used with -M option
const char* subtokenseparator = " "; // used with a future option
int         interpstate = 0;       // used -I or with -i
int         grepQ       = 0;       // used with -g option
string      grepString  = "";      // used with -g option



///////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[]) {
   checkOptions(options, argc, argv);
   HumdrumStream streamer(options);
   HumdrumFile infile;

   while (streamer.read(infile)) {
      if (!streamer.eof()) {
         cout << "!!!!SEGMENT: " << infile.getFileName() << endl;
      }
      processFile(infile);
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// processFile --
//

void processFile(HumdrumFile& infile) {
   if (countQ) {
      cout << infile.getMaxTracks() << endl;
      return;
   }
   if (expandQ) {
      expandSpines(field, subfield, model, infile, expandInterp);
   } else if (interpQ) {
      getInterpretationFields(field, subfield, model, infile, interps,
            interpstate);
   } else if (reverseQ) {
      reverseSpines(field, subfield, model, infile, reverseInterp);
   } else if (fieldQ || excludeQ) {
      fillFieldData(field, subfield, model, fieldstring, infile);
   } else if (grepQ) {
      fillFieldDataByGrep(field, subfield, model, grepString, infile,
         interpstate);
   }

   int j;
   if (debugQ && !traceQ) {
      cout << "!! Field Expansion List:";
      for (j=0; j<(int)field.size(); j++) {
         cout << " " << field[j];
  if (subfield[j]) {
            cout << (char)subfield[j];
         }
  if (model[j]) {
            cout << (char)model[j];
         }
      }
      cout << endl;
   }

   // preserve SEGMENT filename if present (now printed in main())
   // infile.printNonemptySegmentLabel(cout);

   // analyze the input file according to command-line options
   if (fieldQ || grepQ) {
      extractFields(infile, field, subfield, model);
   } else if (excludeQ) {
      excludeFields(infile, field, subfield, model);
   } else if (traceQ) {
      extractTrace(infile, tracefile);
   } else {
      cout << infile;
   }
}



//////////////////////////////
//
// fillFieldDataByGrep --
//

void fillFieldDataByGrep(vector<int>& field, vector<int>& subfield,
      vector<int>& model, const string& searchstring, HumdrumFile& infile,
      int state) {

   field.reserve(infile.getMaxTracks()+1);
   subfield.reserve(infile.getMaxTracks()+1);
   model.reserve(infile.getMaxTracks()+1);
   field.resize(0);
   subfield.resize(0);
   model.resize(0);

   vector<int> tracks;
   tracks.resize(infile.getMaxTracks()+1);
   fill(tracks.begin(), tracks.end(), 0);
   PerlRegularExpression pre;
   int track;

   int i, j;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isSpineLine()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (pre.search(infile[i][j], searchstring.data(), "")) {
            track = infile[i].getPrimaryTrack(j);
            tracks[track] = 1;
         }
      }
   }

   int zero = 0;
   for (i=1; i<(int)tracks.size(); i++) {
      if (state != 0) {
         tracks[i] = !tracks[i];
      }
      if (tracks[i]) {
         field.push_back(i);
         subfield.push_back(zero);
         model.push_back(zero);
      }
   }
}



//////////////////////////////
//
// getInterpretationFields --
//

void getInterpretationFields(vector<int>& field, vector<int>& subfield,
      vector<int>& model, HumdrumFile& infile, string& interps, int state) {
   vector<string> sstrings; // search strings
   sstrings.reserve(100);
   sstrings.resize(0);

   int i, j, k;
   string buffer;
   buffer = interps;

   PerlRegularExpression pre;
   pre.sar(buffer, "\\s+", "", "g");  // remove spaces from the search string.

   while (pre.search(buffer, "^([^,]+)")) {
      sstrings.push_back(pre.getSubmatch());
      pre.sar(buffer, "^[^,]+,?", "", "");
   }

   if (debugQ) {
      cout << "!! Interpretation strings to search for: " << endl;
      for (i=0; i<(int)sstrings.size(); i++) {
         cout << "!!\t" << sstrings[i] << endl;
      }
   }

   vector<int> tracks;
   tracks.resize(infile.getMaxTracks()+1);
   fill(tracks.begin(), tracks.end(), 0);

   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isInterpretation()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         for (k=0; k<(int)sstrings.size(); k++) {
            if (sstrings[k] == infile[i][j]) {
               tracks[infile[i].getPrimaryTrack(j)] = 1;
            }
         }
      }
   }

   field.reserve(tracks.size());
   subfield.reserve(tracks.size());
   model.reserve(tracks.size());

   field.resize(0);
   subfield.resize(0);
   model.resize(0);


   int zero = 0;
   for (i=1; i<(int)tracks.size(); i++) {
      if (state == 0) {
         tracks[i] = !tracks[i];
      }
      if (tracks[i]) {
         field.push_back(i);
         subfield.push_back(zero);
         model.push_back(zero);
      }
   }

}



//////////////////////////////
//
// expandSpines --
//

void expandSpines(vector<int>& field, vector<int>& subfield, vector<int>& model,
      HumdrumFile& infile, string& interp) {

   vector<int> splits;
   splits.resize(infile.getMaxTracks()+1);
   fill(splits.begin(), splits.end(), 0);

   int i, j;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isSpineManipulator()) {
         continue;
      }

      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (strchr(infile[i].getSpineInfo(j), '(') != NULL) {
            splits[infile[i].getPrimaryTrack(j)] = 1;
         }
      }
   }

   field.reserve(infile.getMaxTracks()*2);
   field.resize(0);

   subfield.reserve(infile.getMaxTracks()*2);
   subfield.resize(0);

   model.reserve(infile.getMaxTracks()*2);
   model.resize(0);

   int allQ = 0;
   if (interp.size() == 0) {
      allQ = 1;
   }

   // ggg
   vector<int> dummyfield;
   vector<int> dummysubfield;
   vector<int> dummymodel;
   getInterpretationFields(dummyfield, dummysubfield, model, infile, interp, 1);

   vector<int> interptracks;

   interptracks.resize(infile.getMaxTracks()+1);
   fill(interptracks.begin(), interptracks.end(), 0);

   for (i=0; i<(int)dummyfield.size(); i++) {
      interptracks[dummyfield[i]] = 1;
   }

   int aval = 'a';
   int bval = 'b';
   int zero = 0;
   for (i=1; i<(int)splits.size(); i++) {
      if (splits[i] && (allQ || interptracks[i])) {
         field.push_back(i);
         subfield.push_back(aval);
         model.push_back(zero);
         field.push_back(i);
         subfield.push_back(bval);
         model.push_back(zero);
      } else {
         field.push_back(i);
         subfield.push_back(zero);
         model.push_back(zero);
      }
   }

   if (debugQ) {
      cout << "!!expand: ";
      for (i=0; i<(int)field.size(); i++) {
         cout << field[i];
         if (subfield[i]) {
            cout << (char)subfield[i];
         }
         if (i < (int)field.size()-1) {
            cout << ",";
         }
      }
      cout << endl;
   }
}



//////////////////////////////
//
// reverseSpines -- reverse the order of spines, grouped by the
//   given exclusive interpretation.
//

void reverseSpines(vector<int>& field, vector<int>& subfield, vector<int>& model,
      HumdrumFile& infile, const string& exinterp) {

   vector<int> target;
   target.resize(infile.getMaxTracks()+1);
   fill(target.begin(), target.end(), 0);

   int t;

   for (t=1; t<=infile.getMaxTracks(); t++) {
      if (strcmp(infile.getTrackExInterp(t), exinterp.data()) == 0) {
         target[t] = 1;
      }
   }

   field.reserve(infile.getMaxTracks()*2);
   field.resize(0);

   int i, j;
   int lasti = target.size();
   for (i=(int)target.size()-1; i>0; i--) {
      if (target[i]) {
         lasti = i;
         field.push_back(i);
         for (j=i+1; j<(int)target.size(); j++) {
            if (!target[j]) {
               field.push_back(j);
            } else {
               break;
            }
         }
      }
   }

   // if the grouping spine is not first, then preserve the
   // locations of the pre-spines.
   int extras = 0;
   if (lasti != 1) {
      extras = lasti - 1;
      field.resize(field.size()+extras);
      for (i=0; i<(int)field.size()-extras; i++) {
         field[(int)field.size()-1-i] = field[(int)field.size()-1-extras-i];
      }
      for (i=0; i<extras; i++) {
         field[i] = i+1;
      }
   }

   if (debugQ) {
      cout << "!!reverse: ";
      for (i=0; i<(int)field.size(); i++) {
         cout << field[i] << " ";
      }
      cout << endl;
   }

   subfield.resize(field.size());
   fill(subfield.begin(), subfield.end(), 0);

   model.resize(field.size());
   fill(model.begin(), model.end(), 0);

}



//////////////////////////////
//
// fillFieldData --
//

void fillFieldData(vector<int>& field, vector<int>& subfield,
      vector<int>& model, string& fieldstring, HumdrumFile& infile) {

   int maxtrack = infile.getMaxTracks();

   field.reserve(maxtrack);
   field.resize(0);

   subfield.reserve(maxtrack);
   subfield.resize(0);

   model.reserve(maxtrack);
   model.resize(0);

   PerlRegularExpression pre;
   string buffer = fieldstring;
   pre.sar(buffer, "\\s", "", "gs");
   int start = 0;
   int value = 0;
   value = pre.search(buffer, "^([^,]+,?)");
   string tempstr;
   while (value != 0) {
      start += value - 1;
      start += strlen(pre.getSubmatch(1));
      processFieldEntry(field, subfield, model, pre.getSubmatch(), infile);
      tempstr = buffer.c_str() + start;
      value = pre.search(tempstr, "^([^,]+,?)");
   }
}



//////////////////////////////
//
// processFieldEntry --
//   3-6 expands to 3 4 5 6
//   $   expands to maximum spine track
//   $-1 expands to maximum spine track minus 1, etc.
//

void processFieldEntry(vector<int>& field, vector<int>& subfield,
      vector<int>& model, const char* astring, HumdrumFile& infile) {

   int maxtrack = infile.getMaxTracks();
   int modletter;
   int subletter;

   PerlRegularExpression pre;
   string buffer = astring;

   // remove any comma left at end of input astring (or anywhere else)
   pre.sar(buffer, ",", "", "g");

   // first remove $ symbols and replace with the correct values
   removeDollarsFromString(buffer, infile.getMaxTracks());

   int zero = 0;
   if (pre.search(buffer, "^(\\d+)-(\\d+)$")) {
      int firstone = strtol(pre.getSubmatch(1), NULL, 10);
      int lastone  = strtol(pre.getSubmatch(2), NULL, 10);

      if ((firstone < 1) && (firstone != 0)) {
         cerr << "Error: range token: \"" << astring << "\""
              << " contains too small a number at start: " << firstone << endl;
         cerr << "Minimum number allowed is " << 1 << endl;
         exit(1);
      }
      if ((lastone < 1) && (lastone != 0)) {
         cerr << "Error: range token: \"" << astring << "\""
              << " contains too small a number at end: " << lastone << endl;
         cerr << "Minimum number allowed is " << 1 << endl;
         exit(1);
      }
      if (firstone > maxtrack) {
         cerr << "Error: range token: \"" << astring << "\""
              << " contains number too large at start: " << firstone << endl;
         cerr << "Maximum number allowed is " << maxtrack << endl;
         exit(1);
      }
      if (lastone > maxtrack) {
         cerr << "Error: range token: \"" << astring << "\""
              << " contains number too large at end: " << lastone << endl;
         cerr << "Maximum number allowed is " << maxtrack << endl;
         exit(1);
      }

      int i;
      if (firstone > lastone) {
         for (i=firstone; i>=lastone; i--) {
            field.push_back(i);
            subfield.push_back(zero);
            model.push_back(zero);
         }
      } else {
         for (i=firstone; i<=lastone; i++) {
            field.push_back(i);
            subfield.push_back(zero);
            model.push_back(zero);
         }
      }
   } else if (pre.search(buffer, "^(\\d+)([a-z]*)")) {
      int value = strtol(pre.getSubmatch(1), NULL, 10);
      modletter = 0;
      subletter = 0;
      if (strchr(pre.getSubmatch(2), 'a') != NULL) {
         subletter = 'a';
      }
      if (strchr(pre.getSubmatch(), 'b') != NULL) {
         subletter = 'b';
      }
      if (strchr(pre.getSubmatch(), 'c') != NULL) {
         subletter = 'c';
      }
      if (strchr(pre.getSubmatch(), 'd') != NULL) {
         modletter = 'd';
      }
      if (strchr(pre.getSubmatch(), 'n') != NULL) {
         modletter = 'n';
      }
      if (strchr(pre.getSubmatch(), 'r') != NULL) {
         modletter = 'r';
      }

      if ((value < 1) && (value != 0)) {
         cerr << "Error: range token: \"" << astring << "\""
              << " contains too small a number at end: " << value << endl;
         cerr << "Minimum number allowed is " << 1 << endl;
         exit(1);
      }
      if (value > maxtrack) {
         cerr << "Error: range token: \"" << astring << "\""
              << " contains number too large at start: " << value << endl;
         cerr << "Maximum number allowed is " << maxtrack << endl;
         exit(1);
      }
      field.push_back(value);
      if (value == 0) {
         subfield.push_back(zero);
         model.push_back(zero);
      } else {
         subfield.push_back(subletter);
         model.push_back(modletter);
      }
   }
}



//////////////////////////////
//
// removeDollarsFromString -- substitute $ sign for maximum track count.
//

void removeDollarsFromString(string& buffer, int maxtrack) {
   PerlRegularExpression pre;
   char buf2[128] = {0};
   int value2;

   if (pre.search(buffer, "\\$$")) {
      sprintf(buf2, "%d", maxtrack);
      pre.sar(buffer, "\\$$", buf2);
   }

   if (pre.search(buffer, "\\$(?![\\d-])")) {
      // don't know how this case could happen, however...
      sprintf(buf2, "%d", maxtrack);
      pre.sar(buffer, "\\$(?![\\d-])", buf2, "g");
   }

   if (pre.search(buffer, "\\$0")) {
      // replace $0 with maxtrack (used for reverse orderings)
      sprintf(buf2, "%d", maxtrack);
      pre.sar(buffer, "\\$0", buf2, "g");
   }

   while (pre.search(buffer, "\\$(-?\\d+)")) {
      value2 = maxtrack - (int)fabs(strtol(pre.getSubmatch(1), NULL, 10));
      sprintf(buf2, "%d", value2);
      pre.sar(buffer, "\\$-?\\d+", buf2);
   }

}



//////////////////////////////
//
// excludeFields -- print all spines except the ones in the list of fields.
//

void excludeFields(HumdrumFile& infile, vector<int>& field,
      vector<int>& subfield, vector<int>& model) {
   int i;
   int j;
   int start = 0;

   for (i=0; i<infile.getNumLines(); i++) {
      switch (infile[i].getType()) {
         case E_humrec_none:
         case E_humrec_empty:
         case E_humrec_global_comment:
         case E_humrec_bibliography:
            cout << infile[i] << '\n';
            break;
         case E_humrec_data_comment:
         case E_humrec_data_kern_measure:
         case E_humrec_interpretation:
         case E_humrec_data:
            start = 0;
            for (j=0; j<infile[i].getFieldCount(); j++) {
               if (isInList(infile[i].getPrimaryTrack(j), field)) {
                  continue;
               }
               if (start != 0) {
                  cout << '\t';
               }
               start = 1;
               cout << infile[i][j];
            }
            if (start != 0) {
               cout << endl;
            }
            break;
         default:
            cout << "!!Line = UNKNOWN:" << infile[i] << endl;
            break;
      }
   }
}



//////////////////////////////
//
// extractFields -- print all spines in the list of fields.
//

void extractFields(HumdrumFile& infile, vector<int>& field,
      vector<int>& subfield, vector<int>& model) {

   PerlRegularExpression pre;
   int i;
   int j;
   int t;
   int start = 0;
   int target;
   int subtarget;
   int modeltarget;
   string spat;

   for (i=0; i<infile.getNumLines(); i++) {
      switch (infile[i].getType()) {
         case E_humrec_none:
         case E_humrec_empty:
         case E_humrec_global_comment:
         case E_humrec_bibliography:
            cout << infile[i] << '\n';
            break;
         case E_humrec_data_comment:
         case E_humrec_data_kern_measure:
         case E_humrec_interpretation:
         case E_humrec_data:
            if (infile[i].isSpineManipulator()) {
               dealWithSpineManipulators(infile, i, field, subfield, model);
               break;
            }
            start = 0;
            for (t=0; t<(int)field.size(); t++) {
               target = field[t];
               subtarget = subfield[t];
               modeltarget = model[t];
               if (modeltarget == 0) {
                  switch (subtarget) {
                     case 'a':
                     case 'b':
                        modeltarget = submodel;
                        break;
                     case 'c':
                        modeltarget = comodel;
                  }
               }
               if (target == 0) {
                  if (start != 0) {
                     cout << '\t';
                  }
                  start = 1;
                  if (!infile[i].hasSpineManip()) {
                     switch (infile[i].getType()) {
                        case E_humrec_data_comment:
                           cout << "!"; break;
                        case E_humrec_data_kern_measure:
                           cout << infile[i][0]; break;
                        case E_humrec_data:
                           cout << "."; break;
                        // interpretations handled in dealWithSpineManipulators()
                        // [obviously not, so adding a blank one here
                        case E_humrec_interpretation:
                           cout << "*"; break;
                     }
                  }
               } else {
                  for (j=0; j<infile[i].getFieldCount(); j++) {
                     if (infile[i].getPrimaryTrack(j) != target) {
                        continue;
                     }
                     switch (subtarget) {
                     case 'a':

                        getSearchPat(spat, target, "a");
                        if (pre.search(infile[i].getSpineInfo(j), spat) ||
                              !pre.search(infile[i].getSpineInfo(j), "\\(")) {
                           if (start != 0) {
                              cout << '\t';
                           }
                           start = 1;
                           cout << infile[i][j];
                        }

                        break;
                     case 'b':

                        getSearchPat(spat, target, "b");
                        if (pre.search(infile[i].getSpineInfo(j), spat)) {
                           if (start != 0) {
                              cout << '\t';
                           }
                           start = 1;
                           cout << infile[i][j];
                        } else if (!pre.search(infile[i].getSpineInfo(j),
                              "\\(")) {
                           if (start != 0) {
                              cout << '\t';
                           }
                           start = 1;
                           dealWithSecondarySubspine(field, subfield, model, t,
                                 infile, i, j, modeltarget);
                        }

                        break;
                     case 'c':
                        if (start != 0) {
                           cout << '\t';
                        }
                        start = 1;
                        dealWithCospine(field, subfield, model, t, infile, i, j,
                           modeltarget, modeltarget, cointerp);
                        break;
                     default:
                        if (start != 0) {
                           cout << '\t';
                        }
                        start = 1;
                        cout << infile[i][j];
                     }
                  }
               }
            }
            if (start != 0) {
               cout << endl;
            }
            break;
         default:
            cout << "!!Line = UNKNOWN:" << infile[i] << endl;
            break;
      }
   }
}



//////////////////////////////
//
// dealWithCospine -- extract the required token(s) from a co-spine.
//

void dealWithCospine(vector<int>& field, vector<int>& subfield, vector<int>& model,
      int targetindex, HumdrumFile& infile, int line, int cospine,
      int comodel, int submodel, const string& cointerp) {

   vector<string> cotokens;
   cotokens.reserve(50);

   char buffer[1024];
   int i, j, k;
   int index;

   if (infile[line].isInterpretation()) {
      cout << infile[line][cospine];
      return;
   }

   if (infile[line].isMeasure()) {
      cout << infile[line][cospine];
      return;
   }

   if (infile[line].isLocalComment()) {
      cout << infile[line][cospine];
      return;
   }

   int count = infile[line].getTokenCount(cospine);
   for (k=0; k<count; k++) {
      infile[line].getToken(buffer, cospine, k, 1000);
      cotokens.resize(cotokens.size()+1);
      index = (int)cotokens.size()-1;
      cotokens[index] = buffer;
   }

   vector<int> spineindex;
   vector<int> subspineindex;

   spineindex.reserve(infile.getMaxTracks()*2);
   spineindex.resize(0);

   subspineindex.reserve(infile.getMaxTracks()*2);
   subspineindex.resize(0);

   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (strcmp(infile[line].getExInterp(j), cointerp.data()) != 0) {
         continue;
      }
      if (strcmp(infile[line][j], ".") == 0) {
         continue;
      }
      count = infile[line].getTokenCount(j);
      for (k=0; k<count; k++) {
         infile[line].getToken(buffer, j, k, 1000);
         if (comodel == 'r') {
            if (strchr(buffer, 'r') != NULL) {
               continue;
            }
         }
         spineindex.push_back(j);
         subspineindex.push_back(k);
      }
   }

   if (debugQ) {
      cout << "\n!!codata:\n";
      for (i=0; i<(int)cotokens.size(); i++) {
         cout << "!!\t" << i << "\t" << cotokens[i];
         if (i < (int)spineindex.size()) {
            cout << "\tspine=" << spineindex[i];
            cout << "\tsubspine=" << subspineindex[i];
         } else {
            cout << "\tspine=.";
            cout << "\tsubspine=.";
         }
         cout << endl;
      }
   }

   string buff;

   int start = 0;
   for (i=0; i<(int)field.size(); i++) {
      if (strcmp(infile.getTrackExInterp(field[i]), cointerp.data()) != 0) {
         continue;
      }

      for (j=0; j<infile[line].getFieldCount(); j++) {
         if (infile[line].getPrimaryTrack(j) != field[i]) {
            continue;
         }
         if (subfield[i] == 'a') {
            getSearchPat(buff, field[i], "a");
            if ((strchr(infile[line].getSpineInfo(j), '(') == NULL) ||
               (strstr(infile[line].getSpineInfo(j), buff.c_str()) != NULL)) {
               printCotokenInfo(start, infile, line, j, cotokens, spineindex,
                     subspineindex);
            }
         } else if (subfield[i] == 'b') {
            // this section may need more work...
            getSearchPat(buff, field[i], "b");
            if ((strchr(infile[line].getSpineInfo(j), '(') == NULL) ||
               (strstr(infile[line].getSpineInfo(j), buff.c_str()) != NULL)) {
               printCotokenInfo(start, infile, line, j, cotokens, spineindex,
                     subspineindex);
            }
         } else {
            printCotokenInfo(start, infile, line, j, cotokens, spineindex,
               subspineindex);
         }
      }
   }
}



//////////////////////////////
//
// printCotokenInfo --
//

void printCotokenInfo(int& start, HumdrumFile& infile, int line, int spine,
      vector<string>& cotokens, vector<int>& spineindex,
      vector<int>& subspineindex) {
   int i;
   int found = 0;
   for (i=0; i<(int)spineindex.size(); i++) {
      if (spineindex[i] == spine) {
         if (start == 0) {
            start++;
         } else {
            cout << subtokenseparator;
         }
         if (i<(int)cotokens.size()) {
            cout << cotokens[i];
         } else {
            cout << ".";
         }
      found = 1;
      }
   }
   if (!found) {
      if (start == 0) {
         start++;
      } else {
         cout << subtokenseparator;
      }
      cout << ".";
   }
}



//////////////////////////////
//
// dealWithSecondarySubspine -- what to print if a secondary spine
//     does not exist on a line.
//

void dealWithSecondarySubspine(vector<int>& field, vector<int>& subfield,
      vector<int>& model, int targetindex, HumdrumFile& infile, int line,
      int spine, int submodel) {

   int& i = line;
   int& j = spine;

   PerlRegularExpression pre;
   string buffer;

   switch (infile[line].getType()) {
      case E_humrec_data_comment:
         if ((submodel == 'n') || (submodel == 'r')) {
            cout << "!";
         } else {
            cout << infile[i][j];
         }
         break;

      case E_humrec_data_kern_measure:
         cout << infile[i][j];
         break;

      case E_humrec_interpretation:
         if ((submodel == 'n') || (submodel == 'r')) {
            cout << "*";
         } else {
            cout << infile[i][j];
         }
         break;

      case E_humrec_data:
         if (submodel == 'n') {
            cout << ".";
         } else if (submodel == 'r') {
            if (strcmp(infile[i][j], ".") == 0) {
               cout << ".";
            } else if (strchr(infile[i][j], 'q') != NULL) {
               cout << ".";
            } else if (strchr(infile[i][j], 'Q') != NULL) {
               cout << ".";
            } else {
               buffer = infile[i][j];
               if (pre.search(buffer, "{")) {
                  cout << "{";
               }
               // remove secondary chord notes:
               pre.sar(buffer, " .*", "", "");
               // remove unnecessary characters (such as stem direction):
               pre.sar(buffer, "[^}pPqQA-Ga-g0-9.;%#nr-]", "", "g");
               // change pitch to rest:
               pre.sar(buffer, "[A-Ga-g#n-]+", "r", "");
               // add editorial marking unless -Y option is given:
               if (strcmp(editorialInterpretation, "") != 0) {
                  if (pre.search(buffer, "rr")) {
                     pre.sar(buffer, "(?<=rr)", editorialInterpretation, "");
                     pre.sar(buffer, "rr", "r");
                  } else {
                     pre.sar(buffer, "(?<=r)", editorialInterpretation, "");
                  }
               }
               cout << buffer;
            }
         } else {
            cout << infile[i][j];
         }
         break;

      default:
         cerr << "Should not get to this line of code" << endl;
         exit(1);
   }
}




//////////////////////////////
//
// getSearchPat --
//

void getSearchPat(string& spat, int target, const char* modifier) {
   if (strlen(modifier) > 20) {
      cerr << "Error in GetSearchPat" << endl;
      exit(1);
   }
   spat.reserve(32);
   spat = "\\(";
   char buffer[32] = {0};
   sprintf(buffer, "%d", target);
   spat += buffer;
   spat += "\\)";
   spat += modifier;
}



//////////////////////////////
//
// dealWithSpineManipulators -- check for proper Humdrum syntax of
//     spine manipulators (**, *-, *x, *v, *^) when creating the output.
//

void dealWithSpineManipulators(HumdrumFile& infile, int line,
      vector<int>& field, vector<int>& subfield, vector<int>& model) {

   vector<int> vmanip;  // counter for *v records on line
   vmanip.resize(infile[line].getFieldCount());
   fill(vmanip.begin(), vmanip.end(), 0);

   vector<int> xmanip; // counter for *x record on line
   xmanip.resize(infile[line].getFieldCount());
   fill(xmanip.begin(), xmanip.end(), 0);

   int i = 0;
   int j;
   for (j=0; j<(int)vmanip.size(); j++) {
      if (strcmp(infile[line][j], "*v") == 0) {
         vmanip[j] = 1;
      }
      if (strcmp(infile[line][j], "*x") == 0) {
         xmanip[j] = 1;
      }
   }

   int counter = 1;
   for (i=1; i<(int)xmanip.size(); i++) {
      if ((xmanip[i] == 1) && (xmanip[i-1] == 1)) {
         xmanip[i] = counter;
         xmanip[i-1] = counter;
         counter++;
      }
   }

   counter = 1;
   i = 0;
   while (i < (int)vmanip.size()) {
      if (vmanip[i] == 1) {
         while ((i < (int)vmanip.size()) && (vmanip[i] == 1)) {
            vmanip[i] = counter;
            i++;
         }
         counter++;
      }
      i++;
   }

   vector<int> fieldoccur;  // nth occurance of an input spine in the output
   fieldoccur.resize(field.size());
   fill(fieldoccur.begin(), fieldoccur.end(), 0);

   vector<int> trackcounter; // counter of input spines occurances in output
   trackcounter.resize(infile.getMaxTracks()+1);
   fill(trackcounter.begin(), trackcounter.end(), 0);

   for (i=0; i<(int)field.size(); i++) {
      if (field[i] != 0) {
         trackcounter[field[i]]++;
         fieldoccur[i] = trackcounter[field[i]];
      }
   }

   vector<string> tempout;
   vector<int> vserial;
   vector<int> xserial;
   vector<int> fpos;     // input column of output spine

   tempout.reserve(1000);
   tempout.resize(0);

   vserial.resize(1000);
   vserial.resize(0);

   xserial.resize(1000);
   xserial.resize(0);

   fpos.resize(1000);
   fpos.resize(0);

   string spat;
   string spinepat;
   PerlRegularExpression pre;
   int subtarget;
   int modeltarget;
   int xdebug = 0;
   int vdebug = 0;
   int suppress = 0;
   int t;
   int target;
   int tval;
   for (t=0; t<(int)field.size(); t++) {
      target = field[t];
      subtarget = subfield[t];
      modeltarget = model[t];
      if (modeltarget == 0) {
         switch (subtarget) {
            case 'a':
            case 'b':
               modeltarget = submodel;
               break;
            case 'c':
               modeltarget = comodel;
         }
      }
      suppress = 0;
      if (target == 0) {
         if (strncmp(infile[line][0], "**", 2) == 0) {
            storeToken(tempout, "**blank");
            tval = 0;
            vserial.push_back(tval);
            xserial.push_back(tval);
            fpos.push_back(tval);
         } else if (strcmp(infile[line][0], "*-") == 0) {
            storeToken(tempout, "*-");
            tval = 0;
            vserial.push_back(tval);
            xserial.push_back(tval);
            fpos.push_back(tval);
         } else {
            storeToken(tempout, "*");
            tval = 0;
            vserial.push_back(tval);
            xserial.push_back(tval);
            fpos.push_back(tval);
         }
      } else {
         for (j=0; j<infile[line].getFieldCount(); j++) {
            if (infile[line].getPrimaryTrack(j) != target) {
               continue;
            }
       // filter by subfield
       if (subtarget == 'a') {
               getSearchPat(spat, target, "b");
          if (pre.search(infile[line].getSpineInfo(j), spat)) {
                  continue;
          }
       } else if (subtarget == 'b') {
               getSearchPat(spat, target, "a");
          if (pre.search(infile[line].getSpineInfo(j), spat)) {
                  continue;
               }
            }

            switch (subtarget) {
            case 'a':

               if (!pre.search(infile[line].getSpineInfo(j), "\\(")) {
                  if (strcmp(infile[line][j], "*^") == 0) {
                     storeToken(tempout, "*");
                  } else {
                     storeToken(tempout, infile[line][j]);
                  }
               } else {
                  getSearchPat(spat, target, "a");
                  spinepat =  infile[line].getSpineInfo(j);
                  pre.sar(spinepat, "\\(", "\\(", "g");
                  pre.sar(spinepat, "\\)", "\\)", "g");

                  if ((strcmp(infile[line][j], "*v") == 0) &&
                        (spinepat == spat)) {
                     storeToken(tempout, "*");
                  } else {
                     getSearchPat(spat, target, "b");
                     if ((spinepat == spat) &&
                           (strcmp(infile[line][j], "*v") == 0)) {
                        // do nothing
                        suppress = 1;
                     } else {
                        storeToken(tempout, infile[line][j]);
                     }
                  }
               }

               break;
            case 'b':

               if (!pre.search(infile[line].getSpineInfo(j), "\\(")) {
                  if (strcmp(infile[line][j], "*^") == 0) {
                     storeToken(tempout, "*");
                  } else {
                     storeToken(tempout, infile[line][j]);
                  }
               } else {
                  getSearchPat(spat, target, "b");
                  spinepat = infile[line].getSpineInfo(j);
                  pre.sar(spinepat, "\\(", "\\(", "g");
                  pre.sar(spinepat, "\\)", "\\)", "g");

                  if ((strcmp(infile[line][j], "*v") == 0) &&
                        (spinepat == spat)) {
                     storeToken(tempout, "*");
                  } else {
                     getSearchPat(spat, target, "a");
                     if ((spinepat == spat) &&
                           (strcmp(infile[line][j], "*v") == 0)) {
                        // do nothing
                        suppress = 1;
                     } else {
                        storeToken(tempout, infile[line][j]);
                     }
                  }
               }

               break;
            case 'c':
               // work on later
               storeToken(tempout, infile[line][j]);
               break;
            default:
               storeToken(tempout, infile[line][j]);
            }

            if (suppress) {
               continue;
            }

            if (tempout[(int)tempout.size()-1] == "*x") {
               tval = fieldoccur[t] * 1000 + xmanip[j];
               xserial.push_back(tval);
               xdebug = 1;
            } else {
               tval = 0;
               xserial.push_back(tval);
            }

            if (tempout[(int)tempout.size()-1] == "*v") {
               tval = fieldoccur[t] * 1000 + vmanip[j];
               vserial.push_back(tval);
               vdebug = 1;
            } else {
               tval = 0;
               vserial.push_back(tval);
            }

            fpos.push_back(j);

         }
      }
   }

   if (debugQ && xdebug) {
      cout << "!! *x serials = ";
      for (int ii=0; ii<(int)xserial.size(); ii++) {
         cout << xserial[ii] << " ";
      }
      cout << "\n";
   }

   if (debugQ && vdebug) {
      cout << "!!LINE: " << infile[line].getLine() << endl;
      cout << "!! *v serials = ";
      for (int ii=0; ii<(int)vserial.size(); ii++) {
         cout << vserial[ii] << " ";
      }
      cout << "\n";
   }

   // check for proper *x syntax /////////////////////////////////
   for (i=0; i<(int)xserial.size()-1; i++) {
      if (!xserial[i]) {
         continue;
      }
      if (xserial[i] != xserial[i+1]) {
         if (tempout[i] == "*x") {
            xserial[i] = 0;
            tempout[i] = "*";
         }
      } else {
         i++;
      }
   }

   if ((tempout.size() == 1) || (xserial.size() == 1)) {
      // get rid of *x if there is only one spine in output
      if (xserial[0]) {
         xserial[0] = 0;
         tempout[0] = "*";
      }
   } else if ((int)xserial.size() > 1) {
      // check the last item in the list
      int index = (int)xserial.size()-1;
      if (tempout[index] == "*x") {
         if (xserial[index] != xserial[index-1]) {
            xserial[index] = 0;
            tempout[index] = "*";
         }
      }
   }

   // check for proper *v syntax /////////////////////////////////
   vector<int> vsplit;
   vsplit.resize((int)vserial.size());
   fill(vsplit.begin(), vsplit.end(), 0);

   // identify necessary line splits
   for (i=0; i<(int)vserial.size()-1; i++) {
      if (!vserial[i]) {
         continue;
      }
      while ((i<(int)vserial.size()-1) && (vserial[i]==vserial[i+1])) {
         i++;
      }
      if ((i<(int)vserial.size()-1) && vserial[i]) {
         if (vserial.size() > 1) {
            if (vserial[i+1]) {
               vsplit[i+1] = 1;
            }
         }
      }
   }

   // remove single *v spines:

   for (i=0; i<(int)vsplit.size()-1; i++) {
      if (vsplit[i] && vsplit[i+1]) {
         if (tempout[i] == "*v") {
            tempout[i] = "*";
            vsplit[i] = 0;
         }
      }
   }

   if (debugQ) {
      cout << "!!vsplit array: ";
      for (i=0; i<(int)vsplit.size(); i++) {
         cout << " " << vsplit[i];
      }
      cout << endl;
   }

   if (vsplit.size() > 0) {
      if (vsplit[(int)vsplit.size()-1]) {
         if (tempout[(int)tempout.size()-1] == "*v") {
            tempout[(int)tempout.size()-1] = "*";
            vsplit[(int)vsplit.size()-1] = 0;
         }
      }
   }

   int vcount = 0;
   for (i=0; i<(int)vsplit.size(); i++) {
      vcount += vsplit[i];
   }

   if (vcount) {
      printMultiLines(vsplit, vserial, tempout);
   }

   int start = 0;
   for (i=0; i<(int)tempout.size(); i++) {
      if (tempout[i] != "") {
         if (start != 0) {
            cout << "\t";
         }
         cout << tempout[i];
         start++;
      }
   }
   if (start) {
      cout << '\n';
   }
}



//////////////////////////////
//
// printMultiLines -- print separate *v lines.
//

void printMultiLines(vector<int>& vsplit, vector<int>& vserial,
      vector<string>& tempout) {
   int i;

   int splitpoint = -1;
   for (i=0; i<(int)vsplit.size(); i++) {
      if (vsplit[i]) {
         splitpoint = i;
         break;
      }
   }

   if (debugQ) {
      cout << "!!tempout: ";
      for (i=0; i<(int)tempout.size(); i++) {
         cout << tempout[i] << " ";
      }
      cout << endl;
   }

   if (splitpoint == -1) {
      return;
   }

   int start = 0;
   int printv = 0;
   for (i=0; i<splitpoint; i++) {
      if (tempout[i] != "") {
         if (start) {
            cout << "\t";
         }
         cout << tempout[i];
         start = 1;
         if (tempout[i] == "*v") {
            if (printv) {
               tempout[i] = "";
            } else {
               tempout[i] = "*";
               printv = 1;
            }
         } else {
            tempout[i] = "*";
         }
      }
   }

   for (i=splitpoint; i<(int)vsplit.size(); i++) {
      if (tempout[i] != "") {
         if (start) {
            cout << "\t";
         }
         cout << "*";
      }
   }

   if (start) {
      cout << "\n";
   }

   vsplit[splitpoint] = 0;

   printMultiLines(vsplit, vserial, tempout);
}



//////////////////////////////
//
// storeToken --
//

void storeToken(vector<string>& storage, const char* string) {
   storage.push_back(string);
}

void storeToken(vector<string>& storage, int index, const char* string) {
   storage[index] = string;
}



//////////////////////////////
//
// isInList -- returns true if first number found in list of numbers.
//     returns the matching index plus one.
//

int isInList(int number, vector<int>& listofnum) {
   int i;
   for (i=0; i<(int)listofnum.size(); i++) {
      if (listofnum[i] == number) {
         return i+1;
      }
   }
   return 0;

}



//////////////////////////////
//
// getTraceData --
//

void getTraceData(vector<int>& startline, vector<vector<int> >& fields,
      const string& tracefile, HumdrumFile& infile) {
   char buffer[1024] = {0};
   PerlRegularExpression pre;
   int linenum;
   startline.reserve(10000);
   startline.resize(0);
   fields.reserve(10000);
   fields.resize(0);

   ifstream input;
   input.open(tracefile.data());
   if (!input.is_open()) {
      cerr << "Error: cannot open file for reading: " << tracefile << endl;
      exit(1);
   }

   string temps;
   vector<int> field;
   vector<int> subfield;
   vector<int> model;

   input.getline(buffer, 1024);
   while (!input.eof()) {
      if (pre.search(buffer, "^\\s*$")) {
         continue;
      }
      if (!pre.search(buffer, "(\\d+)")) {
         continue;
      }
      linenum = strtol(pre.getSubmatch(1), NULL, 10);
      linenum--;  // adjust so that line 0 is the first line in the file
      temps = buffer;
      pre.sar(temps, "\\d+", "", "");
      pre.sar(temps, "[^,\\s\\d\\$\\-].*", "");  // remove any possible comments
      pre.sar(temps, "\\s", "", "g");
      if (pre.search(temps, "^\\s*$")) {
         // no field data to process online
         continue;
      }
      startline.push_back(linenum);
      string ttemp = temps;
      fillFieldData(field, subfield, model, ttemp, infile);
      fields.push_back(field);
      input.getline(buffer, 1024);
   }

}



//////////////////////////////
//
// extractTrace --
//

void extractTrace(HumdrumFile& infile, const string& tracefile) {
   vector<int> startline;
   vector<vector<int> > fields;
   getTraceData(startline, fields, tracefile, infile);
   int i, j;

   if (debugQ) {
      for (i=0; i<(int)startline.size(); i++) {
         cout << "!!TRACE " << startline[i]+1 << ":\t";
         for (j=0; j<(int)fields[i].size(); j++) {
            cout << fields[i][j] << " ";
         }
         cout << "\n";
      }
   }


   if (startline.size() == 0) {
      for (i=0; i<infile.getNumLines(); i++) {
         switch (infile[i].getType()) {
            case E_humrec_none:
            case E_humrec_empty:
            case E_humrec_global_comment:
            case E_humrec_bibliography:
               cout << infile[i] << '\n';
         }
      }
      return;
   }

   for (i=0; i<startline[0]; i++) {
      switch (infile[i].getType()) {
         case E_humrec_none:
         case E_humrec_empty:
         case E_humrec_global_comment:
         case E_humrec_bibliography:
            cout << infile[i] << '\n';
      }
   }

   int endline;
   for (j=0; j<(int)startline.size(); j++) {
      if (j == (int)startline.size()-1) {
         endline = infile.getNumLines()-1;
      } else {
         endline = startline[j+1]-1;
      }
      for (i=startline[j]; i<endline; i++) {
         switch (infile[i].getType()) {
            case E_humrec_none:
            case E_humrec_empty:
            case E_humrec_global_comment:
            case E_humrec_bibliography:
               cout << infile[i] << '\n';
            default:
               printTraceLine(infile, i, fields[j]);
         }
      }
   }
}



//////////////////////////////
//
// printTraceLine --
//

void printTraceLine(HumdrumFile& infile, int line, vector<int>& field) {
   int j;
   int t;
   int start = 0;
   int target;

   start = 0;
   for (t=0; t<(int)field.size(); t++) {
      target = field[t];
      for (j=0; j<infile[line].getFieldCount(); j++) {
         if (infile[line].getPrimaryTrack(j) != target) {
            continue;
         }
         if (start != 0) {
            cout << '\t';
         }
         start = 1;
         cout << infile[line][j];
      }
   }
   if (start != 0) {
      cout << endl;
   }
}


/*

//////////////////////////////
//
// extractInterpretation -- extract interpretations which match the
//    given list.  Not used any more: generalized to allow for
//    other interpretation matching other than exclusive interpretations.
//

void extractInterpretations(HumdrumFile& infile, string& interps) {
   int i;
   int j;
   int column = 0;
   PerlRegularExpression pre;
   string buffer;
   buffer.reserve(1024);
   buffer = "";

   for (i=0; i<infile.getNumLines(); i++) {
      if (debugQ) {
         cout << "!!Processing line " << i+1 << ": " << infile[i] << endl;
      }
      switch (infile[i].getType()) {
         case E_humrec_none:
         case E_humrec_empty:
         case E_humrec_global_comment:
         case E_humrec_bibliography:
            cout << infile[i] << '\n';
            break;
         case E_humrec_data_comment:
         case E_humrec_data_kern_measure:
         case E_humrec_interpretation:
         case E_humrec_data:
            column = 0;
            for (j=0; j<infile[i].getFieldCount(); j++) {
               buffer = infile[i].getExInterp(j);
               buffer += "\\b";  // word boundary marker
               pre.sar(buffer, "\\*", "\\\\*", "g");
               if (pre.search(interps.data(), buffer.getBase()) == 0) {
               // if (strstr(interps.data(), infile[i].getExInterp(j)) == NULL) {
                  continue;
               }
               if (column != 0) {
                  cout << '\t';
               }
               column++;
               cout << infile[i][j];
            }
            if (column != 0) {
               cout << endl;
            }
            break;
         default:
            cout << "!!Line is UNKNOWN:" << infile[i] << endl;
            break;
      }
   }
}

*/



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("P|F|S|x|exclude=s:", "Remove listed spines from output");
   opts.define("i=s:", "Exclusive interpretation list to extract from input");
   opts.define("I=s:", "Exclusive interpretation exclusion list");
   opts.define("f|p|s|field|path|spine=s:",
                     "for extraction of particular spines");
   opts.define("C|count=b", "print a count of the number of spines in file");
   opts.define("c|cointerp=s:**kern", "Exclusive interpretation for cospines");
   opts.define("g|grep=s:", "Extract spines which match a given regex.");
   opts.define("r|reverse=b", "reverse order of spines by **kern group");
   opts.define("R=s:**kern", "reverse order of spine by exinterp group");
   opts.define("t|trace=s:", "use a trace file to extract data");
   opts.define("e|expand=b", "expand spines with subspines");
   opts.define("E|expand-interp=s:", "expand subspines limited to exinterp");
   opts.define("m|model|method=s:d", "method for extracting secondary spines");
   opts.define("M|cospine-model=s:d", "method for extracting cospines");
   opts.define("Y|no-editoral-rests=b",
                     "do not display yy marks on interpreted rests");

   opts.define("debug=b", "print debugging information");
   opts.define("author=b");                     // author of program
   opts.define("version=b");                    // compilation info
   opts.define("example=b");                    // example usages
   opts.define("h|help=b");                     // short description
   opts.process(argc, argv);

   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Feb 2008" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: Feb 2008" << endl;
      cout << "compiled: " << __DATE__ << endl;
      cout << MUSEINFO_VERSION << endl;
      exit(0);
   } else if (opts.getBoolean("help")) {
      usage(opts.getCommand().data());
      exit(0);
   } else if (opts.getBoolean("example")) {
      example();
      exit(0);
   }

   excludeQ    = opts.getBoolean("x");
   interpQ     = opts.getBoolean("i");
   interps     = opts.getString("i");

   interpstate = 1;
   if (!interpQ) {
      interpQ = opts.getBoolean("I");
      interpstate = 0;
      interps = opts.getString("I");
   }

   fieldQ      = opts.getBoolean("f");
   debugQ      = opts.getBoolean("debug");
   countQ      = opts.getBoolean("count");
   traceQ      = opts.getBoolean("trace");
   tracefile   = opts.getString("trace");
   reverseQ    = opts.getBoolean("reverse");
   expandQ     = opts.getBoolean("expand") || opts.getBoolean("E");
   submodel    = opts.getString("model").data()[0];
   cointerp    = opts.getString("cointerp");
   comodel     = opts.getString("cospine-model").data()[0];

   if (opts.getBoolean("no-editoral-rests")) {
      editorialInterpretation = "";
   }

   if (interpQ) {
      fieldQ = 1;
   }

   if (expandQ) {
      fieldQ = 1;
      expandInterp = opts.getString("expand-interp");
   }

   if (!reverseQ) {
      reverseQ = opts.getBoolean("R");
      if (reverseQ) {
         reverseInterp = opts.getString("R");
      }
   }

   if (reverseQ) {
      fieldQ = 1;
   }

   if (excludeQ) {
      fieldstring = opts.getString("x");
   } else if (fieldQ) {
      fieldstring = opts.getString("f");
   }

   grepQ = opts.getBoolean("grep");
   grepString = opts.getString("grep");

}



//////////////////////////////
//
// example -- example usage of the sonority program
//

void example(void) {
   cout <<
   "                                                                         \n"
   << endl;
}





//////////////////////////////
//
// usage -- gives the usage statement for the sonority program
//

void usage(const char* command) {
   cout <<
   "                                                                         \n"
   << endl;
}



// md5sum: 21ed6709fd2f31e68ee16f8b5a0d1d52 extractx.cpp [20151120]
