//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Nov 23 05:24:14 PST 2009
// Last Modified: Tue Nov 23 09:54:40 PST 2010 added more features
// Last Modified: Wed Jan 12 07:00:38 PST 2011 added ending note marking
// Last Modified: Tue Jan 18 11:10:38 PST 2011 added --mstart option
// Last Modified: Sun Feb 20 18:38:07 PST 2011 added --percent option
// Last Modified: Thu Feb 24 17:10:34 PST 2011 added --file option
// Last Modified: Thu Feb 24 17:10:34 PST 2011 C strings to C++ strings.
// Filename:      ...sig/examples/all/theloc.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/theloc.cpp
// Syntax:        C++; museinfo
//
// Description:   Identify the location of an index note in a files as 
//                output from "themax --location".
//
// Todo: If a mark is added with -m or --mchar, the program should
// complain and exit if the mark already exists.  But probably allow
// the -m to have the !!!RDF be overwritten with the same marker.

#include "humdrum.h"
#include "PerlRegularExpression.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;


// Function declarations:
void      checkOptions          (Options& opts, int argc, char** argv);
void      example               (void);
void      usage                 (const string& command);
void      processData           (istream& input);
void      extractDataFromInputLine(string& filename, 
                                  string& voicename, int& track, 
                                  int& subtrack, vector<int>& starts, 
                                  vector<int>& endings, char* inputline);
void      prepareSearchPaths    (vector<string>& paths, 
                                 const string& pathlist);
int       fileexists            (string& jointname, string& filename,
                                 string& path);
void      getFileAndPath        (string& fileandpath, 
                                 string& filename, 
                                 vector<string>& paths);
int       findNote              (int nth, HumdrumFile& infile, int& cur, 
                                 int& row, int& col, int track, int subtrack,
                                 int& measure);
void      fillMeterInfo         (HumdrumFile& infile, 
                                 vector<RationalNumber>& meterbot, int track);
void      markNotes             (HumdrumFile& infile, int row, int col, 
                                 int track, int subtrack, int matchlen, 
                                 const string& marker);
void      processDataLine       (HumdrumFile& infile, const string& inputline, 
                                 string& filename, 
                                 string& lastfilename, 
                                 string& voicename, int track, 
                                 int subtrack, vector<int>& starts, 
                                 vector<int>& endings);
void      printDash             (void);
void      displayNoteLocationInfo(HumdrumFile& infile, int num, int row, 
                                int col, int measure,
                                vector<RationalNumber>& meterbot);

// User interface variables:
Options   options;
vector<string> paths;
int         debugQ       = 0;     // used with --debug option
int         percentQ     = 0;     // used with -P option
int         dirdropQ     = 0;     // used with -D option
int         dispLineQ    = 0;     // used with -l option
int         dispColumnQ  = 0;     // used with -c option
int         dispNoteQ    = 1;     // used with -N option
int         dispAbsBeatQ = 0;     // used with -a option
int         dispMeasureQ = 1;     // used with -M option
int         dispQBeatQ   = 0;     // used with -q option
int         dispBeatQ    = 1;     // used with -B option
int         rationalQ    = 0;     // used with -r option
int         dispFracQ    = 0;     // used with -f option
int         markQ        = 0;     // used with --mark option
int         matchlistQ   = 0;     // used with --matchlist option
int         mark2Q       = 0;     // used with --mark2 option
int         tieQ         = 0;     // used with --tie option
int         graceQ       = 1;     // used with -G option
int         doubleQ      = 0;     // used with --mstart option
int         fileQ        = 0;     // used with --file option
string      Filename     = "";    // used with --file option
int         matchlen     = 1;     // used with --mark option
string      marker       = "@";   // used with --marker option


//////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
   // process the command-line options
   checkOptions(options, argc, argv);
 
   ifstream input;
   int numinputs = options.getArgumentCount();
   for (int i=0; i<numinputs || i==0; i++) {
      // if no command-line arguments read data file from standard input
      if (numinputs < 1) {
         processData(cin);
      } else {
         input.open(options.getArg(i+1).c_str());
         processData(input);
         input.close();
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// processData --
//

void processData(istream& input) {
   #define LINESIZE 100000
   string filename;
   string lastfilename;
   vector<int> starts;
   vector<int> endings;
   string voicename;
   int track;
   int subtrack;
   HumdrumFile infile;
   char inputline[LINESIZE] = {0};

   do {
      input.getline(inputline, LINESIZE);
      if (input.eof()) {
         break;
      }

      if (strncmp(inputline, "#NOGRACE", strlen("#NOGRACE")) == 0) {
         graceQ = 0;
         continue;
      } else if (strncmp(inputline, "#GRACE", strlen("#GRACE")) == 0) {
         graceQ = 1;
         continue;
      } else if (strncmp(inputline, "#", strlen("#")) == 0) {
         // unknown control message, just ignore and don't try to process.
         continue;
      }

      extractDataFromInputLine(filename, voicename, track, subtrack, starts, 
            endings, inputline);
      if (track > 0) {
         processDataLine(infile, inputline, filename, lastfilename, voicename,
               track, subtrack, starts, endings);
         lastfilename = filename;
      } else {
         // echo input lines which are not understood (comments?)
         cout << inputline << endl; 
      }

   } while (!input.eof());


   while (!input.eof()) {

   }

   // flush any data needing to be printed (such as for markQ):
	filename = "";
   processDataLine(infile, "", filename, lastfilename, voicename, track, 
         subtrack, starts, endings);
}



//////////////////////////////
//
// processDataLine --
//

void processDataLine(HumdrumFile& infile, const string& inputline, 
      string& filename, string& lastfilename, 
      string& voicename, int track, int subtrack, vector<int>& starts, 
      vector<int>& endings) {

   PerlRegularExpression pre;
   PerlRegularExpression pre2;
   PerlRegularExpression pre3;

   string fileandpath;
   if (fileQ) {
		filename = Filename;
      pre.sar(filename, "&colon;", ":", "g");
   }
   if (filename != "") {
      getFileAndPath(fileandpath, filename, paths);
      if (debugQ) {
         cout << "FOUND FILE: " << fileandpath << endl;
      }
      if (fileandpath == "") {
         // print original line without processing content since file not found
         cout << inputline << endl;
         return;
      }
   }

   string tempstr;
	
   if (filename != lastfilename) {
      if (lastfilename != "") {
         if (markQ) {
            cout << infile;
            if (pre.search(marker, "^\\s*([^\\s])\\s+", "") ||
                pre.search(marker, "^\\s*([^\\s])$", "")
                     ) {
               cout << "!!!RDF**kern: " << pre.getSubmatch(1);
               cout << "= matched note";
               if (pre.search(marker, 
                     "color\\s*=\\s*\"?([^\\s\"\\)\\(,;]+)\"?", "")) {

						tempstr = marker;
                  pre3.sar(tempstr, "^\\s*[^\\s]\\s+", "", "");
                  pre3.sar(tempstr, "color\\s*=\\s*\"?[^\\s\"\\)\\(,;]+\"?", "", "g");
                  pre3.sar(tempstr, "^[\\s:,;-=]+", "", "");
                  pre3.sar(tempstr, "[\\s,:;-=]+$", "", "");

                  if (!pre2.search(pre.getSubmatch(1), "#", "")) {
                     if (pre2.search(pre.getSubmatch(1), "^[0-9a-f]+$", "i")) {
                        if (strlen(pre.getSubmatch(1)) == 6) {
                           cout << ", color=\"#" << pre.getSubmatch(1) << "\"";
                        } else {
                           cout << ", color=\"" << pre.getSubmatch(1) << "\"";
                        }
                     } else {
                        cout << ", color=\"" << pre.getSubmatch(1) << "\"";
                     }
                  } else {
                     cout << ", color=\"" << pre.getSubmatch(1) << "\"";
                  }
               }
               if (tempstr.size() > 0) {
                  cout << " " << tempstr;
               }
               cout << endl;
            }
            if (!mark2Q) {
               cout << "!!!MATCHLEN:\t" << matchlen << endl;
            }
            if (filename == "") {
               // empty filename is a dummy to force last line of 
               // input to be processed correctly if markQ or similar is used.
               return;
            }
         }
      }
      if (filename != "") {
         infile.clear();
         infile.read(fileandpath.c_str());
         infile.analyzeRhythm("4"); // only by quarter-note beats for now
      }
   }

   if (filename =="") {
      // empty filename is a dummy, but shouldn't ever get here.
      return;
   }

   if (matchlistQ && markQ) {
      cout << "!!MATCHES:\t";
   }

   if (fileQ) {
		fileandpath = Filename;
   }

   if (matchlistQ) {
      cout << fileandpath;
      cout << ":";
      cout << voicename;
      cout << ":";
      cout << track;
      cout << "\t";
   }
   
   int row = 0;
   int col = 0;
   int cur = 0;   // current nth numbered note in the given track

   int erow = 0;
   int ecol = 0;
   int ecur = 0;   // current nth numbered note in the given track

   int measure = 1;
   int emeasure = 1;
   if (infile.getPickupDuration() > 0.0) {
      measure = 0;
      emeasure = 0;
   }

   vector<RationalNumber> meterbot;
   if (dispBeatQ) {
      fillMeterInfo(infile, meterbot, track);
   }

   int i;
   int state;
   int estate;
   for (i=0; i<(int)starts.size(); i++) {
      state = findNote(starts[i], infile, cur, row, col, track, 
            subtrack, measure);
      if (state == 0) {
         continue;
      }
      if (((int)endings.size() >0) && (endings[i] >= 0)) {
         estate = findNote(endings[i], infile, ecur, erow, ecol, 
         track, subtrack, emeasure);
      } else {
         estate = 0;
      }
      if (markQ) {
         if (((int)endings.size() > 0) && (endings[i] >= 0)) {
            markNotes(infile, row, col, track, subtrack, 
                  endings[i]-starts[i]+1, marker);
         } else {
            markNotes(infile, row, col, track, subtrack, matchlen, marker);
         }
      }
      if (matchlistQ) {
         displayNoteLocationInfo(infile, starts[i], row, col, measure, meterbot);
         if (((int)endings.size() > 0) && (endings[i] >= 0) && estate) {
            printDash();
            displayNoteLocationInfo(infile, endings[i], erow, ecol, emeasure, 
                  meterbot);
         }
      }

      if (matchlistQ) {
         if (i < (int)starts.size()-1) {
            cout << " ";
         }
      }
   }
   if (matchlistQ) {
      cout << endl;
   }

}



//////////////////////////////
//
// printDash -- have to do a complicated system, since markQ would not
// involve a dash being printed.
//

void printDash(void) {
   if (dispNoteQ || dispLineQ || dispColumnQ || dispAbsBeatQ || dispMeasureQ ||
         dispBeatQ || dispFracQ || percentQ || dispQBeatQ) {
      cout << "-";  
   }
}



//////////////////////////////
//
// displayNoteLocationInfo --
//

void displayNoteLocationInfo(HumdrumFile& infile, int num, int row, int col,
      int measure, vector<RationalNumber>& meterbot) {

   RationalNumber four(4, 1);

   if (dispNoteQ) {
      cout << num;
   }
   if (dispLineQ) {
      cout << "L" << row+1;
   }
   if (dispColumnQ) {
      cout << "C" << col+1;
   }
   if (dispAbsBeatQ) {
      if (rationalQ) {
         cout << "A" << infile[row].getAbsBeatR();
      } else {
         cout << "A" << infile[row].getAbsBeat();
      }
   }
   if (percentQ) {
      double percent = infile.getTotalDuration();
      if (percent > 0.0) {
         percent = int(infile[row].getAbsBeat() / percent * 1000.0 
               + 0.5) / 10.0;
      }
      cout << "P" << percent;
   }
   if (dispMeasureQ) {
      cout << "=" << measure;
   }
   if (dispBeatQ) {
      if (rationalQ) {
         RationalNumber tval = (infile[row].getBeatR()-1) * 
                                  (meterbot[row] / four) + 1;
         cout << "B";
         tval.printTwoPart(cout);
      } else {
         cout << "B" << (infile[row].getBeat()-1) * 
                         (meterbot[row].getFloat() / 4.0) + 1;
      }
   }
   if (dispQBeatQ) {
      if (rationalQ) {
         cout << "Q" << infile[row].getBeatR();
      } else {
         cout << "Q" << infile[row].getBeat();
      }
   }
   if (dispFracQ) {
      cout << "F";
   }
}



//////////////////////////////
//
// markNotes -- mark notes in match sequence (for possible later output as HTML
//     with colors).
//

void markNotes(HumdrumFile& infile, int row, int col, int track, int subtrack,
      int matchlen, const string& marker) {

   PerlRegularExpression pre;
   char markchar[2] = {0};
   if (pre.search(marker, "^\\s*([^\\s])\\s+", "") ||
         pre.search(marker, "^\\s*([^\\s])$", "")) {
      markchar[0] = pre.getSubmatch(1)[0];
   }

   int tiestate = 0;
   string newdata;
   newdata.reserve(1000);
   int foundcount = 0;
   int i, j;
   int scount = 0;
   for (i=row; i<infile.getNumLines(); i++) {
      if (!infile[i].isData()) {
         continue;
      }
      if (foundcount >= matchlen) {
         break;
      }
      scount = 0;
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (track != infile[i].getPrimaryTrack(j)) {
            // ignore the current spine if not the correct one
            continue;
         }
         scount++;
         if (subtrack != scount) {
            continue;
         }
         if (strcmp(infile[i][j], ".") == 0) {
            // don't count null tokens
            break;
         }
         if (strchr(infile[i][j], 'r') != NULL) {
            // don't count rests
            break;
         }
         if ((!graceQ) && ((strchr(infile[i][j], 'q') != NULL) ||
                              (strchr(infile[i][j], 'Q') != NULL))
               ) {
            // ignore grace notes if not supposed to count
            break;
         }

         if (strchr(infile[i][j], '[') != NULL) {
            if (tieQ) {
               foundcount--;  // suppress from count later on
               tiestate = 1;
            } 
         } else if (strchr(infile[i][j], ']') != NULL) {
            if (tieQ) {
               // don't subtract one from foundcount (this is the last note)
               tiestate = 1;
            } else {
               // don't color tied note unless asked to
               break;
            }
         } else if (strchr(infile[i][j], '_') != NULL) {
            if (tieQ) {
               foundcount--;  // suppress from count later on
               if (tiestate == 0) {
                  // a weird case where the tie starts with a medial
                  // tie.  This is can legally occur when there is a
                  // multiple ending which a slur crosses.
                  foundcount++;
               }
               tiestate = 1;
            } else {
               // don't color tied note unless asked to
               continue;
            }
         } else {
            tiestate = 0;
         }

         foundcount++;

         if (strstr(infile[i][j], markchar) != NULL) {
            // already marked (perhaps overlapping match)
            break;
         }
			newdata = infile[i][j];
			newdata += markchar;
         if (doubleQ && (foundcount == 1)) {
				newdata += markchar;
         }
         infile[i].changeField(j, newdata.c_str());
         break;

         if (foundcount >= matchlen) {
            goto veryend;
            // if (tieQ && (strchr(infile[i][j], '[') != NULL)) {
            //    foundcount--;
            // } else if (tieQ && (strchr(infile[i][j], '_') != NULL)) {
            //    foundcount--;
            // }
         } 
      }
   }
veryend: ;
}



//////////////////////////////
//
// fillMeterInfo --
//

void fillMeterInfo(HumdrumFile& infile, vector<RationalNumber>& meterbot, 
      int track) {

   int top;
   int bot;

   meterbot.resize(infile.getNumLines());

   RationalNumber current(4, 1);
   RationalNumber compound(3, 2);
   int i, j;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isInterpretation()) {
         meterbot[i] = current;         
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (track != infile[i].getPrimaryTrack(j)) {
            continue;
         }
         if (sscanf(infile[i][j], "*M%d/%d", &top, &bot) == 2) {
            current = bot;
            if ((top != 3) && ((top % 3) == 0)) {
               current *= compound;
            }
         }
         meterbot[i] = current;
         break;
      }
   }
}



//////////////////////////////
//
// findNote -- Search for the row and column of the nth note in the
//     track.  The current position of row and col is on the cur'th
//     note in the track (if all are 0, then haven't started looking).
//     returns 0 if nth note cannot be found in track.
//

int findNote(int nth, HumdrumFile& infile, int& cur, int& row, int& col, 
      int track, int subtrack, int& measure) {
   int direction = 1;

   if (nth > cur) {
      direction = 1;
   } else if (nth < cur) {
      direction = -1;
   } else {
      // Already have the match (for some strange reason) so just return.
      return 1;
   }

   int scount;
   int mval;
   int i, j;
   for (i=row+direction; (i<infile.getNumLines()) && (i>=0); i+=direction) {
      if (infile[i].isMeasure()) {
         if (sscanf(infile[i][0], "=%d", &mval)) {
            measure = mval;
            if (direction < 0) {
               measure--;
            }
         }
      }
      if (!infile[i].isData()) {
         continue;
      }
      scount = 0;
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (track != infile[i].getPrimaryTrack(j)) {
            continue;
         }
         scount++;
         if (subtrack == scount) {
            if (strcmp(infile[i][j], ".") == 0) { 
               // skip null tokens (could make search faster
               // if null token references were utilized).
               break;
            }
            // currently only considering tracks to be **kern data,
            // but should be generalized later (so don't exit from "r"
            // or "]" or "_" for non **kern data.
            if (strchr(infile[i][j], 'r') != NULL) { 
               // skip null tokens (could make search faster
               // if null token references were utilized).
               break;
            }
            if ((!graceQ) && ((strchr(infile[i][j], 'q') != NULL) ||
                              (strchr(infile[i][j], 'Q') != NULL))
                  ) {
               // ignore grace notes if requested
               break;
            }
            // the following statements are not quite right (consider
            // chords with only some notes being tied?)
            // but this will be dependent on tindex's behavior.
            if (strchr(infile[i][j], ']') != NULL) { 
               // skip endings of ties.
               break;
            }
            if (strchr(infile[i][j], '_') != NULL) { 
               // skip continuation ties.
               break;
            }
            // now have a note which is to be counted:
            cur += direction;
            if (cur == nth) {
               row = i;
               col = j;
               return 1;
            } 
            break;
         }
      }
   }

   // if get here, then note not found:
   row = 0;
   col = 0;
   cur = 0;
   return 0;
}



//////////////////////////////
//
// getFileAndPath -- given a particular filename and a list of directory
//    paths to search, return the first file which is found which matches
//    the filename in the list of directory paths.  First search using
//    the complete filename.  Then if the filename with any attached 
//    directory information is not found, then remove the directory
//    information and search again.  
//

void getFileAndPath(string& fileandpath, string& filename, 
   vector<string>& paths) {
   PerlRegularExpression pre;

   pre.sar(filename, "&colon;", ":", "g");
   if (pre.search(filename, "://")) {
      // either a URL or a URI, so no path.
      fileandpath = filename;
      paths.resize(1);
      paths[0] = "";
      return;
   }

   int i;
   for (i=0; i<(int)paths.size(); i++) {
      if (fileexists(fileandpath, filename, paths[i])) {
         return;
      }
   }

   if (!pre.search(filename, "/")) {
		fileandpath = "";
   }

   // check to see if removing the directory name already attached 
   // to the filename helps:

   string tempfilename = filename;
   pre.sar(tempfilename, ".*/", "", "");

   for (i=0; i<(int)paths.size(); i++) {
      if (fileexists(fileandpath, tempfilename, paths[i])) {
         return;
      }
   }

	fileandpath = "";
}



//////////////////////////////
//
// JoinDirToPath -- The temporary buffer used to
//    create the filename is given back to the calling function.
//

void JoinDirToPath(string& jointname, string& path, 
      string& filename) {

   PerlRegularExpression pre;
   if (pre.search(path, "^\\./*$")) {
      // if searching in the current directory, then don't
      // add the current directory marker.
		jointname = filename;
   } else {
      // append "/" to directory name and then filename
      if (pre.search(path, "/$", "")) {
         // don't need to add "/" to separate dir and file names.
			jointname = path;
			jointname += filename;
      } else {
         // need to add "/" to separate dir and file names.
         jointname = path;
			jointname += "/";
			jointname += filename;
      }
   }
}



//////////////////////////////
//
// fileexists --
//

int fileexists(string& jointname, string& filename, 
      string& path) {
   JoinDirToPath(jointname, path, filename);
   if (access(jointname.c_str(), F_OK) != -1) {
      return 1;
   } else {
      return 0;
   }

   // or another way:
   //    struct stat buffer;
   //    return stat(filename, &buffer) == 0;
}



//////////////////////////////
//
// extractDataFromInputLine --
//

void extractDataFromInputLine(string& filename, 
      string& voicename, int& track, int& subtrack, vector<int>& starts, 
      vector<int>& endings, char* inputline) {
	filename = "";
   track = 0;
   subtrack = 1;

   starts.resize(0);
   starts.reserve(1000);

   endings.resize(0);
   endings.reserve(1000);

   int negone = -1;

   char* ptr = inputline;
   int value;
   PerlRegularExpression pre;
   PerlRegularExpression pre2;
   PerlRegularExpression prefilename;

   if (pre.search(ptr, "^([^\\t:]+)[^\\t]*:(\\d+)\\.?(\\d+)?\\t(\\d+)([^\\s]*\\s*)")) {
      if (fileQ) {
			filename = Filename;
      } else {
			filename = pre.getSubmatch(1);
      }
      track = atoi(pre.getSubmatch(2));
      if (strlen(pre.getSubmatch(3)) > 0) {
         subtrack = atoi(pre.getSubmatch(3));
      } else {
         subtrack = 1;
      }
      value = atoi(pre.getSubmatch(4));
      starts.push_back(value);
      if (pre2.search(pre.getSubmatch(5), "-(\\d+)", "")) {
         value = atoi(pre2.getSubmatch(1));
         endings.push_back(value);
      } else {
         endings.push_back(negone);
      }
      ptr = ptr + pre.getSubmatchEnd(5);
      while (pre.search(ptr, "^(\\d+)([^\\s]*\\s*)")) { 
         value = atoi(pre.getSubmatch(1));
         starts.push_back(value);
         if (pre2.search(pre.getSubmatch(2), "-(\\d+)", "")) {
            value = atoi(pre2.getSubmatch(1));
            endings.push_back(value);
         } else {
            endings.push_back(negone);
         }
         ptr = ptr + pre.getSubmatchEnd(2);
      }
      if (pre.search(inputline, "^[^\\t:]*:([^\\t:]*):")) {
         voicename = pre.getSubmatch(1);
      }
   } else if (pre.search(ptr, "^([^\\t:]+)\\t(\\d+)([^\\s]*\\s*)")) {
      // monophonic label (no spine information available).
      // search only on the first **kern column in the file.
      if (fileQ) {
			filename = Filename;
      } else {
			filename = pre.getSubmatch(1);
      }
      track = 1;
      value = atoi(pre.getSubmatch(2));
      starts.push_back(value);
      if (pre2.search(pre.getSubmatch(3), "-(\\d+)", "")) {
         value = atoi(pre2.getSubmatch(1));
         endings.push_back(value);
      } else {
         endings.push_back(negone);
      }
      ptr = ptr + pre.getSubmatchEnd(3);
      while (pre.search(ptr, "^(\\d+)([^\\s]*\\s*)")) { 
         value = atoi(pre.getSubmatch(1));
         starts.push_back(value);
         if (pre2.search(pre.getSubmatch(2), "-(\\d+)", "")) {
            value = atoi(pre2.getSubmatch(1));
            endings.push_back(value);
         } else {
            endings.push_back(negone);
         }
         ptr = ptr + pre.getSubmatchEnd(2);
      }
      if (pre.search(inputline, "^[^\\t:]*:([^\\t:]*):")) {
         // won't occur in this case
         voicename = pre.getSubmatch(1);
      }
   }

   if (dirdropQ) {
      // remove directory names from filename if the -D option was used.
      pre.sar(filename, ".*/", "", "");
   }

   prefilename.sar(filename, "&colon;", ":", "g");

}



//////////////////////////////
//
// checkOptions -- 
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("p|path=s:.", "colon-separated search patch for data files");
   opts.define("D|nodir=b", "remove directory information from input data");
   opts.define("l|L|line=b", "display line in file on which match found");
   opts.define("c|col=b", "display column in file on which match found");
   opts.define("N|nonth=b", "don't display nth note number in file");
   opts.define("P|percent=b", "display percentage into file");
   opts.define("M|nomeasure=b", "don't display measure number in file");
   opts.define("a|abs=b", "display absolute beat number of note in file");
   opts.define("q|qbeat=b", "display quarter note duration for start of bar");
   opts.define("B|nobeat=b", "don't display beat of start of match");
   opts.define("r|rational=b", "display metric info as rational numbers");
   opts.define("f|fraction=b", "display fractional position of match");
   opts.define("m|mark=b", "mark matches in first input file");
   opts.define("mstart|double=b", "double-mark first note in match");
   opts.define("G|no-grace|nograce=b", "do not count grace notes");
   opts.define("fixedmark=i:1", "mark matches in first input file");
   opts.define("matchlist=b", "list matches in output Humdurm file");
   opts.define("file=s:", "filename to use as basis for search information");
   opts.define("mchar|markchar=s:@", "character to mark matches with");
   opts.define("all=b", "display all location formats");
   opts.define("tie|ties=b", "display search markers on tie middle/end notes");

   opts.define("debug=b",  "author of program"); 
   opts.define("author=b",  "author of program"); 
   opts.define("version=b", "compilation info");
   opts.define("example=b", "example usages");   
   opts.define("help=b",  "short description");
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Nov 2010" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 10 Nov 2010" << endl;
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

   dispLineQ   = opts.getBoolean("line");
   dispAbsBeatQ= opts.getBoolean("abs");
   dispColumnQ = opts.getBoolean("col");
   dispNoteQ   =!opts.getBoolean("nonth");
   dispMeasureQ=!opts.getBoolean("nomeasure");
   dispQBeatQ  = opts.getBoolean("qbeat");
   dispBeatQ   =!opts.getBoolean("nobeat");
   dispFracQ   = opts.getBoolean("fraction");
   rationalQ   = opts.getBoolean("rational");
   dirdropQ    = opts.getBoolean("nodir");
   debugQ      = opts.getBoolean("debug");
   percentQ    = opts.getBoolean("percent");
   tieQ        = opts.getBoolean("tie");
   doubleQ     = opts.getBoolean("mstart");
   fileQ       = opts.getBoolean("file");
   if (fileQ) {
      Filename = opts.getString("file").c_str();
   }
   markQ       = opts.getBoolean("fixedmark");
   matchlen    = opts.getInteger("fixedmark");
   matchlistQ  = opts.getBoolean("matchlist");
   if (opts.getBoolean("mark")) {
      markQ = 1;
      matchlen = 1;
      mark2Q = 1;
   } else {
      mark2Q = 0;
   }

   if (!markQ) {
      matchlistQ = 1;
   }
   marker =  opts.getString("markchar").c_str();
   graceQ = !opts.getString("no-grace").c_str();

   if (opts.getBoolean("all")) {
      dispLineQ    = 1;  // used with -l option
      dispColumnQ  = 1;  // used with -c option
      dispNoteQ    = 1;  // used with -N option
      dispAbsBeatQ = 1;  // used with -a option
      percentQ     = 1;  // used with -P option
      dispMeasureQ = 1;  // used with -M option
      dispQBeatQ   = 1;  // used with -q option
      dispBeatQ    = 1;  // used with -B option
   }
   
   prepareSearchPaths(paths, opts.getString("path").c_str());
}



//////////////////////////////
//
// prepareSearchPaths -- get a list of search paths for looking for data files.
//     The list of paths is colon separated.
//

void prepareSearchPaths(vector<string>& paths, const string& pathlist) {
   paths.resize(0);
   paths.reserve(100);
   PerlRegularExpression pre;
   const char* ptr = pathlist.c_str();
   while (pre.search(ptr, "([^:]+):?")) {
		paths.push_back(pre.getSubmatch(1));
      ptr = ptr + pre.getSubmatchEnd(1);
   }

   if (debugQ) {
      cout << "Search Paths:" << endl;
      for (int i=0; i<(int)paths.size(); i++) {
         cout << "search path " << i + 1 << ":\t" << paths[i] << endl;
      }
   }


}



//////////////////////////////
//
// example --
//

void example(void) {


}



//////////////////////////////
//
// usage --
//

void usage(const string& command) {

}



// md5sum: 7b18e2c430085328dca17fcb5d9de87a theloc.cpp [20170605]
