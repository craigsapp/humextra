//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Nov  2 18:28:33 PDT 2010
// Last Modified: Sun Nov  7 07:52:55 PST 2010
// Last Modified: Mon Apr  1 00:21:49 PDT 2013 Enabled multiple segment input
// Filename:      ...sig/examples/all/serialize.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/serialize.cpp
// Syntax:        C++; museinfo
//
// Description: Extract all spines from a multi-spine file into
//      single-column output.
// 

#include "humdrum.h"

#include <algorithm>

using namespace std;

// function declarations
void      checkOptions       (Options& opts, int argc, char* argv[]);
void      example            (void);
void      usage              (const char* command);
int       printHeader        (HumdrumFile& infile);
void      printFooter        (HumdrumFile& infile);
void      printTrack         (HumdrumFile& infile, int track, int start);
void      printOutput        (HumdrumFile& infile);
int       interpmatch        (HumdrumFile& infile, int track, int start, 
                              string& interp);
void      printChordSeparator(HumdrumFile& infile, int row, int col, 
                              char separator, int nth);
void      analyzeMaxSubSpines(HumdrumFile& infile, vector<int>& parmax);
void      printSubTracks     (HumdrumFile& infile, int track, int start, 
                              int submax);
void      printSubSpine      (HumdrumFile& infile, int track, int start, 
                              int subspine);
void      printSubToken      (HumdrumFile& infile, int row, int col, 
                              char separator, int nth);


// global variables
Options     options;          // database for command-line arguments
int          mergeQ  = 0;     // used with -m option
string       interp;          // used with -i option
int          chordQ  = 0;     // extract chord notes serially as well
int          Exstart = 0;     // used for -m -i option interaction
int          Separator = ' '; // used with -c option and -s option
int          subspineQ = 0;   // used with -S option
string       marker = ".";    // used with -e option
int          markerQ = 0;     // used with -e option
int          parsubQ = 0;     // used with -p option
vector<int>  parmax;          // used with -p option
int          nthQ    = 0;     // used with -n option
int          nth     = 0;     // used with -n option
int          leftQ   = 0;     // used with -L option


///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
   HumdrumFileSet infiles;

   // process the command-line options
   checkOptions(options, argc, argv);

   // figure out the number of input files to process
   infiles.read(options);

   int i;
   for (i=0; i<infiles.getCount(); i++) {
      infiles[i].analyzeRhythm();

      if (parsubQ) {
         analyzeMaxSubSpines(infiles[i], parmax);
      } else {
         parmax.resize(infiles[i].getMaxTracks()+1); // [0] is not used.
         fill(parmax.begin(), parmax.end(), 0);
      }
      Exstart = 0;
      if ((infiles.getCount() > 1) && 
          (infiles[i].getFilename().size() > 0)) {
         cout << "!!!!SEGMENT: " << infiles[i].getFilename() << endl;
      }
      printOutput(infiles[i]);
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// analyzeMaxSubSpines -- find the largest number of sub-spines in each 
//     spine.
//

void analyzeMaxSubSpines(HumdrumFile& infile, vector<int>& parmax) {
   parmax.resize(infile.getMaxTracks()+1);  // [0] is not used.
   fill(parmax.begin(), parmax.end(), 0);

   vector<int> linemax;
   linemax.resize(infile.getMaxTracks()+1);  // [0] is not used.
   fill(linemax.begin(), linemax.end(), 0);
   
   int i, j;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isData()) {
         continue;
      }
      fill(linemax.begin(), linemax.end(), 0);
      for (j=0; j<infile[i].getFieldCount(); j++) {
         linemax[infile[i].getPrimaryTrack(j)]++;
      }
      for (j=0; j<(int)linemax.size(); j++) {
         if (parmax[j] < linemax[j]) {
            parmax[j] = linemax[j];
         }
      }
   }
}



//////////////////////////////
//
// printOutput --
//

void printOutput(HumdrumFile& infile) {
   int start = printHeader(infile);
   int maxtrack = infile.getMaxTracks();
   int i;
   for (i=1; i<=maxtrack; i++) {
      printTrack(infile, i, start);
      if (parsubQ) {
         printSubTracks(infile, i, start, parmax[i]);
      }
      if (parsubQ && !mergeQ) {
         cout << "*-" << endl;
      }
   }
   if (mergeQ) {
      cout << "*-" << endl;
   }
   printFooter(infile);
}



//////////////////////////////
//
// printSubTracks --
//

void printSubTracks(HumdrumFile& infile, int track, int start, int submax) {
   if (submax <= 1) {
      // nothing to do...
      return;
   }

   int i;
   for (i=2; i<=submax; i++) {
      printSubSpine(infile, track, start, i);
   }
}



//////////////////////////////
//
// printSubSpine -- print a specific sub-spine sequence.  Similar to
//     printTrack but too messy to merge functionality.
//

void printSubSpine(HumdrumFile& infile, int track, int start, int subspine) {
   int i, j;
   int taber;
   int counter = 0;

   if (interp.size() > 1) {
      if (!interpmatch(infile, track, start, interp)) {
         return;
      }
   }

   vector<int> currsub;
   currsub.resize(infile.getMaxTracks()+1);

   vector<int> lastsub;
   lastsub.resize(infile.getMaxTracks()+1);
   fill(lastsub.begin(), lastsub.end(), 1241324);
   int markerCounter = 0;

   for (i=start; i<infile.getNumLines(); i++) {
      if ((lastsub[track] >= subspine) && (currsub[track] < subspine)) {
         if (markerQ && markerCounter) {
            cout << marker << endl;
         }
         markerCounter++;
      }
      lastsub = currsub;
      fill(currsub.begin(), currsub.end(), 0);
      switch (infile[i].getType()) {
         case E_humrec_data_comment:
         case E_humrec_data_kern_measure:
            taber = 0;
            for (j=0; j<infile[i].getFieldCount(); j++) {
               currsub[infile[i].getPrimaryTrack(j)]++;
               if (currsub[infile[i].getPrimaryTrack(j)] != subspine) {
                  break;
               }
               if (track == infile[i].getPrimaryTrack(j)) {
                  if (taber > 0) {
                     cout << "\t";
                  }
                  cout << infile[i][j];
                  counter++;
                  if (subspineQ) {
                     cout << "\n";
                  } else {
                     taber++;
                  }
               }
            }
            if (taber > 0) {
               cout << endl;
            }
            break;

         case E_humrec_interpretation:
            taber = 0;
            for (j=0; j<infile[i].getFieldCount(); j++) {
               currsub[infile[i].getPrimaryTrack(j)]++;
               if (currsub[infile[i].getPrimaryTrack(j)] != subspine) {
                  break;
               }
               if (markerQ && (strcmp(infile[i][j], "*-") == 0)) {
                  // put marker at the end of the data for the spine so that
                  // the context command will not mix data from 
                  // different spines. (grep -v out after context is run).
                  if (track == infile[i].getPrimaryTrack(j)) {
                     cout << marker << "\n";                  
                  }
               }
               if ((strcmp(infile[i][0], "*-") == 0) && mergeQ) {
                  // suppress all spine terminators
                  // before the last one if merging
                  if (track != infile.getMaxTracks()) {
                     continue;
                  }
               }
               if (track == infile[i].getPrimaryTrack(j)) {
                  if (taber > 0) {
                     cout << "\t";
                  }
                  if (subspineQ || parsubQ) {
                     if (infile[i].isSpineManipulator(j)) {
                        // suppress spine manipulator characters
                        // so that output Humdrum file remains
                        // valid.
                        cout << "*" << "\n";
                     } else {
                        if (parsubQ && (strcmp(infile[i][j], "*-") == 0)) {
                           continue;
                        }
                        cout << infile[i][j] << "\n";
                     }
                  } else {
                     if (parsubQ && (strcmp(infile[i][j], "*-") == 0)) {
                        continue;
                     } else {
                        cout << infile[i][j];
                     }
                     taber++;
                  }
               }
            }
            if (taber > 0) {
               cout << endl;
            }
            break;

         case E_humrec_data:

            taber = 0;
            for (j=0; j<infile[i].getFieldCount(); j++) {
               currsub[infile[i].getPrimaryTrack(j)]++;
               if (currsub[infile[i].getPrimaryTrack(j)] != subspine) {
                  continue;
               }
               if (track == infile[i].getPrimaryTrack(j)) {
                  if (taber > 0) {
                     cout << "\t";
                  }
                  if (chordQ) {
                     printChordSeparator(infile, i, j, Separator, nth);
                     counter++;
                  } else {
                     cout << infile[i][j];
                     counter++;
                  }
                  if (subspineQ) {
                     cout << "\n";
                  } else {
                     taber++;
                  }
               }
            }
            if (taber > 0) {
               cout << endl;
            }

            break;
         case E_humrec_global_comment:
         case E_humrec_empty:
         case E_humrec_bibliography:
         case E_humrec_none:
         default:
            cout << infile[i] << endl;
            break;
      }
      if (infile[i].isInterpretation() && (strcmp(infile[i][0], "*-") == 0)) {
         break;
      }
   }

   // if ((counter > 0) && markerQ) {
   //    cout << marker << endl;
   // }

}



//////////////////////////////
//
// interpmatch -- 
//

int interpmatch(HumdrumFile& infile, int track, int start, 
      string& interp) {
   int i;
   for (i=0; i<infile[start].getFieldCount(); i++) {
      if (track == infile[start].getPrimaryTrack(i)) {
         if (strcmp(infile[start].getExInterp(i), interp.c_str()) == 0) {
            return 1;
         } else {
            return 0;
         }
      }
   }

   return 0;
}



//////////////////////////////
//
// printTrack -- print all data in a particular track.
//

void printTrack(HumdrumFile& infile, int track, int start) {
   int i, j;
   int taber;
   int tcount;

   if (interp.size() > 1) {
      if (!interpmatch(infile, track, start, interp)) {
         return;
      }
   }
   vector<int> currsub;
   currsub.resize(infile.getMaxTracks()+1);

   for (i=start; i<infile.getNumLines(); i++) {
      tcount = 0;
      fill(currsub.begin(), currsub.end(), 0);
      switch (infile[i].getType()) {
         case E_humrec_data_comment:
         case E_humrec_data_kern_measure:

            taber = 0;
            for (j=0; j<infile[i].getFieldCount(); j++) {
               currsub[infile[i].getPrimaryTrack(j)]++;
               if (parsubQ && (currsub[infile[i].getPrimaryTrack(j)] > 1)) {
                  break;
               }
               if (track == infile[i].getPrimaryTrack(j)) {
                  if (leftQ && tcount) {
                     break;
                  }
                  tcount++;
                  if (taber > 0) {
                     cout << "\t";
                  }
                  cout << infile[i][j];
                  if (subspineQ) {
                     cout << "\n";
                  } else {
                     taber++;
                  }
               }
            }
            if (taber > 0) {
               cout << endl;
            }

            break;

         case E_humrec_interpretation:

            taber = 0;
            for (j=0; j<infile[i].getFieldCount(); j++) {
               currsub[infile[i].getPrimaryTrack(j)]++;
               if (parsubQ && (currsub[infile[i].getPrimaryTrack(j)] > 1)) {
                  break;
               }
               if ((strncmp(infile[i][0], "**", 2) == 0) && mergeQ) {
                  // suppress all exclusive interpretations
                  // after the first one if merging
                  if (Exstart == 0) {
                     Exstart++;
                     cout << infile[i][j];
                     taber++;
                     continue;
                  }
                  if (track != 1) {
                     continue;
                  }
               }
               if (markerQ && (strcmp(infile[i][j], "*-") == 0)) {
                  // put marker at the end of the data for the spine so that
                  // the context command will not mix data from 
                  // different spines. (grep -v out after context is run).
                  if (track == infile[i].getPrimaryTrack(j)) {
                     cout << marker << "\n";                  
                  }
               }
               if ((strcmp(infile[i][0], "*-") == 0) && (mergeQ || parsubQ)) {
                  // suppress all spine terminators
                  // before the last one if merging
                  if (track != infile.getMaxTracks()) {
                     continue;
                  }
               }
               if (track == infile[i].getPrimaryTrack(j)) {
                  if (leftQ && tcount) {
                     break;
                  }
                  if (taber > 0) {
                     cout << "\t";
                  }
                  if (subspineQ || parsubQ) {
                     if (infile[i].isSpineManipulator(j)) {
                        // suppress spine manipulator characters
                        // so that output Humdrum file remains
                        // valid.
                        cout << "*" << "\n";
                     } else {
                        if ((mergeQ || parsubQ) && 
                              (strcmp(infile[i][j], "*-") == 0)) {
                           continue;
                        }
                        cout << infile[i][j] << "\n";
                     }
                  } else {
                     if (mergeQ && (strcmp(infile[i][j], "*-") == 0)) {
                        continue;
                     }
                     if (leftQ && (strcmp(infile[i][j], "*^") == 0)) {
                        break;
                     }
                     if (leftQ && (strcmp(infile[i][j], "*v") == 0)) {
                        break;
                     }
                     cout << infile[i][j];
                     taber++;
                     tcount++;
                  }
               }
            }
            if (taber > 0) {
               cout << endl;
            }
            break;

         case E_humrec_data:

            taber = 0;
            for (j=0; j<infile[i].getFieldCount(); j++) {
               currsub[infile[i].getPrimaryTrack(j)]++;
               if (parsubQ && (currsub[infile[i].getPrimaryTrack(j)] > 1)) {
                  break;
               }
               if (track == infile[i].getPrimaryTrack(j)) {
                  if (taber > 0) {
                     cout << "\t";
                  }
                  if (chordQ) {
                     printChordSeparator(infile, i, j, Separator, nth);
                  } else {
                     cout << infile[i][j];
                  }
                  if (subspineQ) {
                     cout << "\n";
                  } else {
                     taber++;
                     tcount++;
                     if (leftQ) {
                        break;
                     }
                  }
               }
            }
            if (taber > 0) {
               cout << endl;
            }

            break;
         case E_humrec_global_comment:
         case E_humrec_empty:
         case E_humrec_bibliography:
         case E_humrec_none:
         default:
            cout << infile[i] << endl;
            break;
      }
      if (infile[i].isInterpretation() && (strcmp(infile[i][0], "*-") == 0)) {
         return;
      }
   }
}



//////////////////////////////
//
// printChordSeparator --
//

void printChordSeparator(HumdrumFile& infile, int row, int col, 
      char separator, int nth) {

   int count = infile[row].getTokenCount(col, separator);
   int len = strlen(infile[row][col]);
   Array<char> buf(len+1);
   char* buffer = buf.getBase();

   if (nth == 0) {
      if (count <= 1) {
         cout << infile[row][col];
         return;
      }

      int i;
      for (i=0; i<count; i++) {
         infile[row].getToken(buffer, col, i, len+1, separator);
         cout << buffer;
         if (i < count - 1) {
            cout << endl;
         }
      }
      return;
   }

   // print only the specified sub-token, or a null token if the
   // subtoken does not exist.

   int index = nth - 1;
   if (nth < 0) {
      index = count + nth; 
   }

   if ((index < 0) || (index >= count)) {
      cout << ".";
      return;
   }
   
   infile[row].getToken(buffer, col, index, len+1, separator);
   cout << buffer;
}



//////////////////////////////
//
// printFooter -- print every line before the last *- in the file.
//

void printFooter(HumdrumFile& infile) {
   int i;
   int start = -1;
   for (i=infile.getNumLines()-1; i>=0; i--) {
      if (infile[i].isInterpretation()) {
         start = i+1;
         break;
      }
   }
   if (start < 0) {
      return;
   }

   for (i=start; i<infile.getNumLines(); i++) {
      cout << infile[i] << endl;
   }
}



//////////////////////////////
//
// printHeader -- print lines in the Humdrum file before the first
//      exclusive interpretaion.
//

int printHeader(HumdrumFile& infile) {
   int output = 0;
   int i;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isInterpretation()) {
         output = i;
         break;
      } 
      cout << infile[i] << endl;
   }
   return output;
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   // merging will not handle spine count changes between tracks.
   opts.define("m|merge=b",      "Merge data into single data spine");
   opts.define("i|interp=s",     "Serialize only specified exclusive interps.");
   opts.define("c|chord=b",      "Serialize notes in chords.");
   opts.define("f|first=b",      "Output only the first sub-token of data.");
   opts.define("l|last=b",       "Output only the last sub-token of data.");
   opts.define("n|number=i:0",   "Output only the nth sub-token of data.");
   opts.define("s|separator=s: ", "Separator character");
   opts.define("S|subspine=b", "Serialize subspines");
   opts.define("t|marker=s:xxx", "end marker for each spine");
   opts.define("p|parsub=b", "Serialize sub-spines in a different manner");
   opts.define("L|left=b", "Follow left splits when serializing");

   opts.define("debug=b");              // determine bad input line num
   opts.define("author=b");             // author of program
   opts.define("version=b");            // compilation info
   opts.define("example=b");            // example usages
   opts.define("h|help=b");             // short description
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Nov 2010" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 2 Nov 2010" << endl;
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

   leftQ      = opts.getBoolean("left");
   chordQ     = opts.getBoolean("chord");
   mergeQ     = opts.getBoolean("merge");
   subspineQ  = opts.getBoolean("subspine");
   nthQ       = opts.getBoolean("number");
   nth        = opts.getInteger("number");
   if (opts.getBoolean("first")) {
      nthQ = 1;
      nth = 1;
   }
   if (opts.getBoolean("last")) {
      nthQ = 1;
      nth = -1;
   }
   if (nthQ) {
      chordQ = 1;
   }

   if (opts.getBoolean("interp")) {
      interp = opts.getString("interp");
      if ((interp.size() < 2) || (interp[0] != '*')) {
         interp.insert(0, "**") ;
      }
   }

   if (strlen(opts.getString("separator").data()) >= 1) {
      Separator = opts.getString("separator").data()[0];
   }
   if (opts.getBoolean("marker")) {
      markerQ = 1;
      marker = opts.getString("marker");
   }
   parsubQ    = opts.getBoolean("parsub");
  
}



//////////////////////////////
//
// example -- example usage of the quality program
//

void example(void) {
   cout <<
   "                                                                         \n"
   "                                                                         \n"
   << endl;
}



//////////////////////////////
//
// usage -- gives the usage statement for the meter program
//

void usage(const char* command) {
   cout <<
   "                                                                         \n"
   "                                                                         \n"
   << endl;
}



// md5sum: 3c27ed7b7aca34e87f12a04b3738c5b2 serialize.cpp [20160320]
