//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu Dec 13 14:58:53 PST 2012
// Last Modified: Thu Dec 13 22:56:07 PST 2012
// Filename:      ...sig/examples/all/humsplit.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/humsplit.cpp
// Syntax:        C++; museinfo
//
// Description:   Splits multiple-segment Humdrum streams into separate files.
//

#include "humdrum.h"
#include "PerlRegularExpression.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   /* for stat function in fileExists */

#ifndef OLDCPP
   #include <iostream>
   using namespace std;
#else
   #include <iostream.h>
#endif


// function declarations
void    checkOptions           (Options& opts, int argc, char* argv[]);
void    example                (void);
void    usage                  (const char* command);
int     getHumdrumSegmentCount (HumdrumStream& streamer);
void    extractSegment         (HumdrumFile& infile, HumdrumStream& streamer, 
                                int extractNum);
void    saveToDisk             (HumdrumFile& infile, int count);
int     fileExists             (Array<char>& filename);

// global variables
Options      options;            // database for command-line arguments
int          countQ        = 0;  // used with -c option
int          extractQ      = 0;  // used with -x option
int          extractNum    = 0;  // used with -x option
int          segmentLabelQ = 0;  // used with -s option
int          overwriteQ    = 0;  // used with -O option (capital O)
int          directoryQ    = 0;  // used with -d option
Array<char>  Directory;          // used with -d option
int          extensionQ    = 0;  // used with -e option
Array<char>  Extension;          // used with -e option
int          prefixQ       = 0;  // used with -p option
Array<char>  Prefix;             // used with -p option
int          Width         = 5;  // used with -w option
int          Start         = 1;  // used with -n option


///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
   checkOptions(options, argc, argv);

   HumdrumStream streamer(options);
   HumdrumFile infile;

   if (countQ) {
      int count = getHumdrumSegmentCount(streamer);
      cout << count << endl;
      exit(0);
   }
   if (extractQ) {
      extractSegment(infile, streamer, extractNum);
      if (segmentLabelQ) {
         infile.printSegmentLabel(cout);
      }
      cout << infile;
      exit(0);
   }

   int filecounter = Start;
   while (streamer.read(infile)) {
      saveToDisk(infile, filecounter++);
   }
   
   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// saveToDisk -- 
//

void saveToDisk(HumdrumFile& infile, int count) {
   Array<char> writename;
   writename = infile.getFilename();
   PerlRegularExpression pre;
   pre.sar(writename, ".*/", "");   // remove old directory path (or URI)

   if (strlen(writename.getBase()) == 0) {
      // There is no file, so create a synthetic one: count.humsplit
      char filename[1024] = {0};
      char format[32] = {0};
      sprintf(format, "%s0%d.humsplit", "%", Width);
      sprintf(filename, format, count);
      writename = filename;
   }

   if (extensionQ) {
      // remove old filename extension and replace with new one
      pre.sar(writename, "\\.[^.]*$", "");
      pre.sar(writename, "$", Extension.getBase());
   }
   if (prefixQ) {
      // prefix to give to filename (useful for auto-naming).
      pre.sar(writename, "^", Prefix.getBase());
   }
   if (directoryQ) {
      // place in new directory if requested
      pre.sar(Directory, "/*$", "/");
      pre.sar(writename, "^", Directory.getBase());
   }

   if (!overwriteQ && fileExists(writename)) {
      // print warning if trying to overwrite an existing file
      cerr << "Warning: " << writename 
           << " already exists.  Skipping..." << endl;
      return;
   }

   ofstream output;
   output.open(writename.getBase());
   if (!output.is_open()) {
      cerr << "Warning: " << writename 
           << " could not be written.  Skipping..." << endl;
   }

   if (segmentLabelQ) {
      // preserve segment label
      infile.printSegmentLabel(output);
   }
   int i;
   int start = 0;
   if (strncmp(infile[0][0], "!!!!SEGMENT", strlen("!!!!SEGMENT")) == 0) {
      start = 1;
   }
   for (i=start; i<infile.getNumLines(); i++) {
      output << infile[i] << '\n';
   }
   output.close();
}



//////////////////////////////
//
// fileExists -- see if the file exists.
//

int fileExists(Array<char>& filename) {
   struct stat buf;
   return stat(filename.getBase(), &buf) == -1 ? 0 : 1;
}



//////////////////////////////
//
// extractSegment --
//

void extractSegment(HumdrumFile& infile, HumdrumStream& streamer, 
      int extractNum) {
   int count = 0;
   while (streamer.read(infile)) {
      count++;
      if (count == extractNum) {
         return;
      }
   }
   infile.clear();
}



//////////////////////////////
//
// getHumdrumSegmentCount --
//

int getHumdrumSegmentCount(HumdrumStream& streamer) {
   int count = 0;
   HumdrumFile infile;
   while (streamer.read(infile)) {
      count++;
   }
   return count;
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("c|count=b:",        "Count number of segments in input stream");
   opts.define("x|extract=i:1",     "Extract the nth stream");
   opts.define("s|segment=b",       "Display segment line in output (with -x)");
   opts.define("O|overwrite=b",     "Overwrite any existing files");
   opts.define("d|directory=s:.",   "Directory to save files to");
   opts.define("e|extension=s:.krn","File extension replacement");
   opts.define("p|prefix=s:",       "Filename prefix");
   opts.define("w|width=i:5",       "Number width for automatic names");
   opts.define("n|number=i:1",      "Starting number for automatic names");

   opts.define("debug=b");                      // determine bad input line num
   opts.define("author=b");                     // author of program
   opts.define("version=b");                    // compilation info
   opts.define("example=b");                    // example usages
   opts.define("h|help=b");                     // short description
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, 13 Dec 2012" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: Dec 2012" << endl;
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

   countQ         = opts.getBoolean("count");
   extractQ       = opts.getBoolean("extract");
   extractNum     = opts.getInteger("extract");
   segmentLabelQ  = opts.getBoolean("segment");
   directoryQ     = opts.getBoolean("directory");
   Directory      = opts.getString("directory").data();
   overwriteQ     = opts.getBoolean("overwrite");
   extensionQ     = opts.getBoolean("extension");
   Extension      = opts.getString("extension").data();
   prefixQ        = opts.getBoolean("prefix");
   Prefix         = opts.getString("prefix").data();
   Width          = opts.getInteger("width");
   Start          = opts.getInteger("number");
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



// md5sum: 64c445cc6dae1aeb96d946f72779aaaf humsplit.cpp [20130531]
