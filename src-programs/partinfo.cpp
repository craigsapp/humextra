//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Sep  2 00:01:11 PDT 2014
// Last Modified: Tue Sep  2 00:01:14 PDT 2014
// Filename:      ...sig/examples/all/partinfo.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/partinfo.cpp
// Syntax:        C++; museinfo
//
// Description:   List part names and abbreviations.
//

#include "humdrum.h"

// function declarations
void      checkOptions       (Options& opts, int argc, char* argv[]);
void      example            (void);
void      usage              (const char* command);
void      processFile        (HumdrumFile& infile);

// global variables
Options   options;           // database for command-line arguments

///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
   HumdrumFileSet infiles;
   checkOptions(options, argc, argv);
   infiles.read(options);
   string filename;

   // for now only deal with a single segment:
   processFile(infiles[0]);
   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// processFile --
//

void processFile(HumdrumFile& infile) {
   int i, j;
   int pindex;
   int hastext = 0;

   vector<int> ktracks;
   infile.getTracksByExInterp(ktracks, "**kern");
   int partcount = ktracks.size();

   vector<int> rktracks;
   rktracks.resize(infile.getMaxTracks() + 1);
   fill(rktracks.begin(), rktracks.end(), -1);
   for (i=0; i<(int)ktracks.size(); i++) {
      rktracks[ktracks[i]] = i;
   }

   char buffer[1024] = {0};
   vector<string> partname(partcount);
   vector<string> partabbr(partcount);
   for (i=0; i<partcount; i++) {
      sprintf(buffer, "part %d", partcount - i);
      partname[i] = buffer;
      sprintf(buffer, "P%d", partcount - i);
      partabbr[i] = buffer;
   }

   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         break;
      }
      if (!infile[i].isInterpretation()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (infile[i].isExInterp(j, "**text")) {
            hastext = 1;
         }
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }

         pindex = rktracks[infile[i].getPrimaryTrack(j)];

         if (strncmp(infile[i][j], "*I\"", 3) == 0) {
            partname[pindex] = &(infile[i][j][3]);
         } else if (strncmp(infile[i][j], "*I'", 3) == 0) {
            partabbr[pindex] = &(infile[i][j][3]);
         }
      }
   }

   cout << "**spine\t**pname\t**pabbr\n";
   cout << "!!voice-count:\t" << partcount << "\n";
   for (i=0; i<(int)ktracks.size(); i++) {
      cout << ktracks[i] << "\t";
      cout << partname[i] << "\t";
      cout << partabbr[i];
      cout << "\n";
   }
   if (hastext) {
      cout << "!!has-text:\ttrue\n";
   }
   cout << "*-\t*-\t*-\t\n";

}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("debug=b");                // determine bad input line num
   opts.define("author=b");               // author of program
   opts.define("version=b");              // compilation info
   opts.define("example=b");              // example usages
   opts.define("h|help=b");               // short description
   opts.process(argc, argv);

   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, September 2014" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: September 2014" << endl;
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



// md5sum: 355973db7e7f396d44df105afedcce9e partinfo.cpp [20160312]
