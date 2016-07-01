//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu Jul 31 13:11:46 PDT 2014
// Last Modified: Thu Jul 31 13:11:49 PDT 2014
// Filename:      ...sig/examples/all/melseq.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/melseq.cpp
// Syntax:        C++; museinfo
//
// Description:   Generate melodic sequence information from
//                polyphonic music with monophic lines.
//

#include "humdrum.h"


// function declarations
void      checkOptions       (Options& opts, int argc, char* argv[]);
void      example            (void);
void      usage              (const char* command);
void      processFile        (HumdrumFile& infile);
void      printTrack         (ostream& out, int voice, int vcount, int track, 
                              HumdrumFile& infile, int attribution, 
                              Array<char>& genre);
int       getAttribution     (HumdrumFile& infile);
void      getGenre           (Array<char>& genre, HumdrumFile& infile);
int       getTrackSpine      (HumdrumFile& infile, int line, int track);

 
// global variables
Options   options;             // database for command-line arguments

///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
   checkOptions(options, argc, argv);
   HumdrumFileSet infiles;
   infiles.read(options);

   for (int i=0; i<infiles.getCount(); i++) {
      processFile(infiles[i]);
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////

//////////////////////////////
//
// processFile --
//

void processFile(HumdrumFile& infile) {
   Array<int> ktracks;
   infile.getTracksByExInterp(ktracks, "**kern");
   int i; 
   int size = ktracks.getSize();
   int attribution = getAttribution(infile);
   Array<char> genre;
   getGenre(genre, infile);
   for (i=size-1; i>=0; i--) {
      printTrack(cout, size-i, size, ktracks[i], infile, attribution, genre);
   }
}



//////////////////////////////
//
// getGenre --
//

void getGenre(Array<char>& genre, HumdrumFile& infile) {
   PerlRegularExpression pre;
   int i;
   genre.setSize(0);
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isBibliographic()) {
         continue;
      }
      if (pre.search(infile[i][0], "!!!AGN:\\s*([^;]+)")) {
         genre = pre.getSubmatch(1);
         pre.sar(genre, "\\s+$", "");
         pre.sar(genre, "\\s+", "_");
         pre.sar(genre, "_section$", "");
         pre.sar(genre, "Chanson", "Song", "g");
      }
   }
}



//////////////////////////////
//
// getAttribution -- 
//

int getAttribution(HumdrumFile& infile) {
   int i;
   int output = 0;
   PerlRegularExpression pre;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isGlobalComment()) {
         continue;
      }
      if (pre.search(infile[i][0], "!!attribution-level:\\s*(\\d+)")) {
         output = atoi(pre.getSubmatch(1));
      }
   }
   return output;
}



//////////////////////////////
//
// printTrack --
//

void printTrack(ostream& out, int voice, int vcount, int track, 
      HumdrumFile& infile, int attribution, Array<char>& genre) {
   infile.analyzeRhythm("4");
   int i, j;
   PerlRegularExpression pre;
   string tag;
   if (pre.search(infile.getFilename(), "([A-Z][a-z][a-z]\\d{4}[^-]*)-")) {
      tag = pre.getSubmatch(1);
   } else if (pre.search(infile.getFilename(), "([A-Z][a-z][a-z]\\d{4}[^-]*)$")) {
      tag = pre.getSubmatch(1);
   } else {
      tag = infile.getFilename();
   }

   out << tag << "\t";
  
   if (attribution > 0) {
      cout << "*A" << attribution << " ";
   }

   out << "*V" << voice << "/" << vcount << " ";
 
   if (genre.getSize() > 0) {
      out << "*G" << genre << " ";
   }

   int restline = -1;
   int b40, lastb40 = -1;
   double time1, time2, rdur;
   int interval;

   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isInterpretation()) {
         j = getTrackSpine(infile, i, track);
         if (pre.search(infile[i][j], "^\\*I'([^\\s]+)")) {
            cout << "*I" << pre.getSubmatch(1) << " ";
         } else if (pre.search(infile[i][j], "^\\*met\\(([^)]+)\\)")) {
            cout << "*M(" << pre.getSubmatch(1) << ") ";
         }
      } else if (infile[i].isBarline()) {
         j = getTrackSpine(infile, i, track);
         if (strstr(infile[i][j], "||") != NULL) {
            cout << "Y ";
            continue;
         }
      } else if (infile[i].isData()) {
         j = getTrackSpine(infile, i, track);
         if (infile[i].isNullToken(j)) {
            continue;
         }
         if (strchr(infile[i][j], ']') != NULL) {
            continue;
         }
         if (strchr(infile[i][j], '_') != NULL) {
            continue;
         }
         if (strchr(infile[i][j], 'r') != NULL) {
            // this is a rest, store the line of the rest for later insertion
            // of a rest into the melodic stream with the rests duration.
            if (restline < 0) {
               restline = i;
            }
            continue;
         } else { 
            b40 = Convert::kernToBase40(infile[i][j]);
            if (b40 <= 0) {
               // something strange happened...
               continue;
            }
            if (restline >= 0) {
               time1 = infile[restline].getAbsBeat();
               time2 = infile[i].getAbsBeat();
               rdur = (time2 - time1)/2;
               out << "R" << rdur << " ";
               restline = -1;
            }
            if (lastb40 > 0) {
               interval = Convert::base40ToDiatonic(b40) - 
                     Convert::base40ToDiatonic(lastb40);
               if (interval >= 0) {
                  interval += 1;
               } else if (interval < 0) {
                  interval -= 1;
               }
               out << interval << " ";
            }
            lastb40 = b40;
         }
      }
   }

   out << "\n";
}



//////////////////////////////
//
// getTrackSpine -- Get the spine index for the given track number.
//

int getTrackSpine(HumdrumFile& infile, int line, int track) {
   int tr;
   for (int j=0; j<infile[line].getFieldCount(); j++) {
      tr = infile[line].getPrimaryTrack(j);
      if (tr == track) {
         return j;
      }
   }

   return -1;
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {

   opts.define("debug=b");              // determine bad input line num
   opts.define("author=b");             // author of program
   opts.define("version=b");            // compilation info
   opts.define("example=b");            // example usages
   opts.define("h|help=b");             // short description
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Oct 2000" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 21 April 2010" << endl;
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
}



//////////////////////////////
//
// example -- example usage of the quality program
//

void example(void) {
   cout <<
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
   << endl;
}



// md5sum: fc163c2ae2b24c9ff36c71c22481ace6 melseq.cpp [20140817]
