//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Oct 14 23:58:44  2002
// Last Modified: Mon Apr 25 11:30:20 PDT 2005
// Last Modified: Thu Dec 13 21:03:33 PST 2012 Added -s option
// Filename:      ...sig/examples/all/humcat.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/humcat.cpp
// Syntax:        C++; museinfo
//
// Description:   Concatenate multiple humdrum files into one continuous
//                data stream.
//

#include "humdrum.h"

#include <string.h>
#include <stdio.h>

#ifndef OLDCPP
   #include <iostream>
   #include <fstream>
#else
   #include <iostream.h>
   #include <fstream.h>
#endif

// function declarations:
void      checkOptions          (Options& opts, int argc, char** argv);
void      example               (void);
void      usage                 (const char* command);
void      printFile             (HumdrumFile& infile, int start, int stop);
int       getIdTags             (Array<Array<char> >& idtags, 
                                 HumdrumFile& infile);
void      printFileID           (HumdrumFile& infile, int index, int count, 
                                 Array<Array<char> >& primaryids);
int       findTag               (Array<Array<char> >& primaryids, 
                                 Array<char>& idtags);
void      printLineID           (HumdrumFile& infile, int index, 
                                 Array<Array<char> >& primaryids, 
				 Array<Array<char> >& idtags);
int       hasSegment            (HumdrumFile& infile);

// User interface variables:
Options   options;
int       segmentQ = 0;         // used with -s option

//////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
   // process the command-line options
   checkOptions(options, argc, argv);
   HumdrumStream streamer(options);

   Array<Array<char> > idtags;
   int idtagQ = 0;

   HumdrumFile infiles[2];

   int good1 = streamer.read(infiles[0]);
   int good2 = streamer.read(infiles[1]);

   if (good1 == 0) {
      // nothing to do
      exit(1);
   }

   // if printing segments, then don't do extra work to suppress **/*-:
   int ii;
   int hassegment;
   if (segmentQ && good1) {
      infiles[0].printNonemptySegmentLabel(cout);
      hassegment = hasSegment(infiles[0]);
      for (ii=0; ii<infiles[0].getNumLines(); ii++) {
         if (hassegment) {
            if (strncmp(infiles[0][ii][0], "!!!!SEGMENT", 11) != 0) {
               cout << infiles[0][ii] << '\n';
            }
         } else {
            cout << infiles[0][ii] << '\n';
         }
      }
   }
   if (segmentQ && good2) {
      infiles[1].printNonemptySegmentLabel(cout);
      hassegment = hasSegment(infiles[1]);
      for (ii=0; ii<infiles[1].getNumLines(); ii++) {
         if (hassegment) {
            if (strncmp(infiles[1][ii][0], "!!!!SEGMENT", 11) != 0) {
               cout << infiles[1][ii] << '\n';
            }
         } else {
            cout << infiles[1][ii] << '\n';
         } 
      }
   }

   if (!segmentQ) {
      // if there is only one file, then just print it out and do nothing:
      if (good2 == 0) {
         if (segmentQ) {
            infiles[0].printSegmentLabel(cout);
         }
         cout << infiles[0];
         exit(0);
      }

      // extract tags from first input
      idtagQ = getIdTags(idtags, infiles[0]);

      // print the first file
      printFile(infiles[0], 1, 0);
   }

   int currindex = 0;
   int count = 1;
   while (streamer.read(infiles[currindex])) {
      count++;
      infiles[currindex].printNonemptySegmentLabel(cout);
      hassegment = hasSegment(infiles[currindex]);
      if (segmentQ) {
         for (ii=0; ii<infiles[currindex].getNumLines(); ii++) {
            if (hassegment) {
               if (strncmp(infiles[currindex][ii][0], "!!!!SEGMENT", 11) != 0) {
                  cout << infiles[currindex][ii] << '\n';
               }
            } else {
               cout << infiles[currindex][ii] << '\n';
            } 
         }
         currindex = !currindex;
         continue;
      }

      // print each file as it arrives.  
      if (idtagQ) {
         printFileID(infiles[!currindex], 0, 0, idtags);
      } else {
         printFile(infiles[!currindex], 0, 0);
      }
      currindex = !currindex;
   }

   if ((count > 0) && !segmentQ) {
      // print the last file, including data terminators
      if (idtagQ) {
         printFileID(infiles[!currindex], 0, 1, idtags);
      } else {
         printFile(infiles[!currindex], 0, 1);
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// hasSegment -- true if the content of the Humdrum file has a line starting
//      with !!!!SEGMENT .  This is necessary to suppress writing more than
//      one SEGMENT marker for the file.  Only looks at the first line in the
//      file.
//

int hasSegment(HumdrumFile& infile) {
   if (strncmp(infile[0][0], "!!!!SEGMENT", 11) == 0) {
      return 1;
   } else {
      return 0;
   }
}



//////////////////////////////
//
// getIdTags --
//

int getIdTags(Array<Array<char> >& idtags, HumdrumFile& infile) {
   int i, j;
   idtags.setSize(infile.getMaxTracks());
   for (i=0; i<idtags.getSize(); i++) {
      idtags[i].setSize(128);
      idtags[i][0] = '\0';
   }
   int foundids = 0;
   char tag[128] = {0};
   
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].getType() != E_humrec_data_comment) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (strncmp(infile[i][j], "!ID=", 4) == 0) {
            strcpy(tag, &(infile[i][j][4]));
            if (strlen(tag)) {
               foundids++;
               strcpy(idtags[infile[i].getPrimaryTrack(j)-1].getBase(), tag);
            }
         }
      }
   }

   return foundids;
}



//////////////////////////////
//
// printFile --
//

void printFile(HumdrumFile& infile, int start, int stop) {

   int i;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isInterpretation()) {
         if ((strcmp(infile[i][0], "*-") == 0) && (stop)) {
            cout << infile[i] << "\n";
         } else if ((strncmp(infile[i][0], "**", 2) == 0) && (start)) {
            cout << infile[i] << "\n";
         } else {
            if ((strcmp(infile[i][0], "*-") == 0) && (!stop)) {
               // print nothing
            } else if ((strncmp(infile[i][0], "**", 2) == 0) && (!start)) {
               // print nothing
            } else {
               cout << infile[i] << "\n";
            }
         }
      } else {
         cout << infile[i] << "\n";
      }
   }

}



//////////////////////////////
//
// printFileID --
//

void printFileID(HumdrumFile& infile, int index, int count, 
      Array<Array<char> >& primaryids) {

   Array<Array<char> > idtags;
   int idtagQ = getIdTags(idtags, infile);
   if (!idtagQ) {
      cout << "ERROR: no ID tags found in file: " << endl;
      cout << infile;
      exit(1);
   }

   int i;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isInterpretation()) {
         if ((strcmp(infile[i][0], "*-") == 0) && (index == count - 1)) {
            cout << infile[i] << "\n";
         } else if ((strncmp(infile[i][0], "**", 2) == 0) && (index == 0)) {
            cout << infile[i] << "\n";
         } else {
            if ((strcmp(infile[i][0], "*-") == 0) && (index != count - 1)) {
               // print nothing
            } else if ((strncmp(infile[i][0], "**", 2) == 0) && (index != 0)) {
               // print nothing
            } else {
               printLineID(infile, i, primaryids, idtags);
               // cout << infile[i] << "\n";
            }
         }
      } else {
         printLineID(infile, i, primaryids, idtags);
         // cout << infile[i] << "\n";
      }
   }

}



//////////////////////////////
//
// printLineID --
//

void printLineID(HumdrumFile& infile, int index, 
      Array<Array<char> >& primaryids, Array<Array<char> >& idtags) {
   Array<Array<int> > spines;
   spines.setSize(primaryids.getSize());
   int i;
   for (i=0; i<spines.getSize(); i++) {
      spines[i].setSize(32);
      spines[i].setSize(0);
   }

   int mapping;
   for (i=0; i<idtags.getSize(); i++) {
      mapping = findTag(primaryids, idtags[i]);
      spines[mapping].append(i);
   }

   int j;
   for (i=0; i<spines.getSize(); i++) {
      if (spines[i].getSize() == 0) {
         cout << "X";
      } else {
         for (j=0; j<spines[i].getSize(); j++) {
            cout << "Y";
         }
      }
      cout << "\t";
   }

}



//////////////////////////////
//
// findTag --
//

int findTag(Array<Array<char> >& primaryids, Array<char>& idtags) {
   int i;
   for (i=0; i<primaryids.getSize(); i++) {
      if (strcmp(primaryids[i].getBase(), idtags.getBase()) == 0) {
         return i;
      }
   }

   return -1;
}




//////////////////////////////
//
// checkOptions -- 
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("h|header=s:",  "Header filename for placement in output");
   opts.define("t|trailer=s:", "Trailer filename for placement in output");
   opts.define("s|segment=b", "Do not merge files, but leave as segments");

   opts.define("author=b",  "author of program"); 
   opts.define("version=b", "compilation info");
   opts.define("example=b", "example usages");   
   opts.define("help=b",  "short description");
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Oct 2002" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 14 Oct 2002" << endl;
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


   segmentQ = opts.getBoolean("segment");
   
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

void usage(const char* command) {

}



// md5sum: 85e01639c1b4dd251c57995e590035b8 humcat.cpp [20151120]
