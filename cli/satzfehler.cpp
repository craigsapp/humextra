//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Fri Nov 20 17:55:43 PST 2015
// Last Modified: Fri Nov 20 17:55:47 PST 2015
// Filename:      ...sig/examples/all/satzfehler.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/satzfehler.cpp
// Syntax:        C++; museinfo
//
// Description:   Identify Satzfehler: one voices does 1-7-1 harmonic pattern
//                while another voies sings at some point inbetween a 7.
// 

#include "humdrum.h"
#include <string>

class NoteNode {
   public:
      int base40;
      int track;
      int measure;
      int startline;
      int endline;
      int spine;
};


// function declarations
void   checkOptions               (Options& opts, int argc, char* argv[]);
void   example                    (void);
void   usage                      (const char* command);
int    analyzeSatzfehler          (HumdrumFile& infile);
void   getNotes                   (HumdrumFile& infile, 
                                   Array<Array<NoteNode> >& notes,
                                   Array<int>& ktracks, Array<int>& rtracks);
void   printNotes                 (Array<Array<NoteNode> >& notes);
int    identifySatzfehler         (HumdrumFile& infile, 
                                   Array<Array<NoteNode> >& notes,
                                   Array<int>& ktracks, Array<int>& rtracks);
int    checkForStazfehler         (HumdrumFile& infile, int startline,
                                   int endline, int xtrack, int tpitch,
                                    Array<int>& targetlines,
                                    Array<int>& targetspines);
void   markToken                  (HumdrumFile& infile, int line, int spine, 
                                   string& signifier);
void  extractSatzfehler           (HumdrumFile& infile, int line, int spine, 
		                             Array<int>& targetlines, 
                                   Array<int>& targetspines);

// global variables
Options   options;            // database for command-line arguments
int       markQ = 1;          // mark notes (currently stuck on).
int       countQ = 0;         // used with -c option
int       fileQ = 0;          // used with -f option
string    Signifier = "Z";    // string to mark satzfehler notes.
int       extractQ = 0;       // used with -x option


///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
   HumdrumFileSet infiles;

   // process the command-line options
   checkOptions(options, argc, argv);
   infiles.read(options);
   int numinputs = infiles.getCount();

   int i;
   int count;
   int sum = 0;
   for (i=0; i<numinputs; i++) {
      count = analyzeSatzfehler(infiles[i]);
      sum += count;
      if (markQ) {
         cout << infiles[i];
         if (count) {
            cout << "!!!RDF**kern: " << Signifier 
                 << " = Satzfehler note" << endl;
         }
      } 
      if (countQ) {
         if (markQ) {
            cout << "!!saztfehler-count: " << count << endl;
         } else {
            if (fileQ) {
            } else {
               cout << "count" << endl;
            }
         }
      }
   }

   if (countQ && (numinputs > 1)) {
      cout << "!!satzfehler-total: " << sum << endl;
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// analyzeSatzfehler --
//

int analyzeSatzfehler(HumdrumFile& infile) {
   Array<Array<NoteNode> > notes;
   Array<int> ktracks;
   Array<int> rtracks;
   getNotes(infile, notes, ktracks, rtracks);
   // printNotes(notes);
   int count = identifySatzfehler(infile, notes, ktracks, rtracks);
   return count;
}



//////////////////////////////
//
// identifySatzfehler --
//

int identifySatzfehler(HumdrumFile& infile, Array<Array<NoteNode> >& notes,
      Array<int>& ktracks, Array<int>& rtracks) {
   int i, j;
   int status;
	Array<int> targetlines(3);
	Array<int> targetspines(3);
	int int1, int2;
   int counter = 0;
   for (i=0; i<notes.getSize(); i++) {
      for (j=2; j<notes[i].getSize(); j++) {
         int1 = notes[i][j-1].base40 - notes[i][j-2].base40;
         int2 = notes[i][j].base40 - notes[i][j-1].base40;
         if ((int1 == -5) && (int2 == 5)) {
				targetlines[0]  = notes[i][j-2].startline;
				targetlines[1]  = notes[i][j-1].startline;
				targetlines[2]  = notes[i][j-0].startline;
				targetspines[0] = notes[i][j-2].spine;
				targetspines[1] = notes[i][j-1].spine;
				targetspines[2] = notes[i][j-0].spine;
            status = checkForStazfehler(infile, notes[i][j-2].startline, 
               notes[i][j].startline, notes[i][j-1].track, 
               notes[i][j-1].base40, targetlines, targetspines);
            if (status) {
               counter++;
            }
            if (status && !markQ && !extractQ) {
               if (fileQ) {
                  cout << infile.getFilename() << "\t";
               }
               cout << "m" << notes[i][j-2].measure 
                      << endl;
            } 
         }
      }
   }
   return counter;
}



//////////////////////////////
//
// markToken -- 
//

void markToken(HumdrumFile& infile, int line, int spine, string& signifier) {
   char buffer[1024] = {0};
   strcpy(buffer, infile[line][spine]);
   strcat(buffer, Signifier.c_str());
   infile[line].setToken(spine, buffer);
}



//////////////////////////////
//
// checkForStazfehler --
//

int checkForStazfehler(HumdrumFile& infile, int startline,
      int endline, int xtrack, int tpitch, 
		Array<int>& targetlines, Array<int>& targetspines) {
   int i, j, k;
   int tpc = tpitch % 40;
   int upc;
   int track;
   int result = 0;
   for (i=startline; i<=endline; i++) {
      if (!infile[i].isData()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }
         track = infile[i].getTrack(j);
         if (track == xtrack) {
            continue;
         }
         if (!infile[i].hasNoteAttack(j)) {
            continue;
         }
         upc = Convert::kernToBase40(infile[i][j]) % 40;
         if (upc == tpc) {
            result++;
				markToken(infile, i, j, Signifier);
				if (result == 1) {
					for (k=0; k<3; k++) {
						markToken(infile, targetlines[k], targetspines[k], Signifier);
					}
				}
				if (extractQ) {
					extractSatzfehler(infile, i, j, targetlines, targetspines);
				}
         }
      }
   }

   return result;
}



//////////////////////////////
//
// extractSatzfehler --
//

void extractSatzfehler(HumdrumFile& infile, int line, int spine, 
		Array<int>& targetlines, Array<int>& targetspines) {

	int track1 = infile[targetlines[0]].getPrimaryTrack(targetspines[0]);
	int track2 = infile[line].getPrimaryTrack(spine);
	int track;

	int minline = targetlines[0];
	int maxline = targetlines.last();
	int i, j;
	int scount;
	cout << "**kern\t**kern\n";
	for (i=minline; i<=maxline; i++) {
		scount = 0;
		for (j=0; j<infile[i].getFieldCount(); j++) {
			track = infile[i].getPrimaryTrack(j);
			if ((track == track1) || (track == track2)) {
				scount++;
				if (scount > 1) {
					cout << "\t";
				}
				if (track == track2) {
					if ((i == minline)
							&& (strstr(infile[i][j], Signifier.c_str()) == NULL)) {
						cout << "rX";
					} else {
						cout << infile[i][j];
					}
				} else {
					cout << infile[i][j];
				}
			}
		}
		cout << endl;
	}
	cout << "*-\t*-\n";
}



//////////////////////////////
//
// printNotes --  for debugging.
//

void printNotes(Array<Array<NoteNode> >& notes) {
   int i, j;
   for (i=0; i<notes.getSize(); i++) {
      for (j=0; j<notes[i].getSize(); j++) {
         cout << notes[i][j].base40 << " ";
      }
      cout << "\n";
      cout << "\n";
   }
}



//////////////////////////////
//
// getNotes -- Get a list of note attackes in each voices (only
//    the first note from each voice in a chord or split voice).
//

void getNotes(HumdrumFile& infile, Array<Array<NoteNode> >& notes,
      Array<int>& ktracks, Array<int>& rtracks) {
   notes.setSize(0);
   infile.getKernTracks(ktracks);
   if (ktracks.getSize() == 0) {
      return;
   }
   notes.setSize(ktracks.getSize());
   rtracks.setSize(infile.getMaxTracks() + 1);
   rtracks.setAll(-1);
   int i, j;
   for (i=0; i<notes.getSize(); i++) {
      notes[i].setSize(infile.getNumLines());
      notes[i].setSize(0);
      rtracks[ktracks[i]] = i;
   }

   int measure = 0;
   NoteNode note;
   int col;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isBarline()) {
         sscanf(infile[i][0], "=%d", &measure);
      }
      if (!infile[i].isData()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }
         if (!(infile[i].hasNoteAttack(j) || infile[i].isRest(j))) {
            continue;
         }
         note.base40 = Convert::kernToBase40(infile[i][j]);
         note.track = infile[i].getTrack(j);
         note.startline = i;
         note.measure = measure;
         note.startline = i;
         note.spine = j;
         col = rtracks[note.track];
         if (infile[i].isRest(j)) {
            if (notes[col].getSize() > 0) {
               if (notes[col].last().base40 < 0) {
                  continue;
               }
            }
         }
         notes[col].append(note);
      }
   }
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("debug=b",       "trace input parsing");   
   opts.define("author=b",      "author of the program");   
   opts.define("version=b",     "compilation information"); 
   opts.define("example=b",     "example usage"); 
   opts.define("h|help=b",      "short description"); 

   opts.define("s|signifier=s:Z",   "Satzfehler marker in data");
   opts.define("c|count=b",         "count Satzfehlers");
   opts.define("f|file|filename=b", "display filenames");
   opts.define("x|extract=b",       "extract Satzfehler voice pairs");

   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Nov 2015" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 20 Nov 2015" << endl;
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


   Signifier = opts.getString("signifier");
   countQ    = opts.getBoolean("count");
   fileQ     = opts.getBoolean("filename");
   extractQ  = opts.getBoolean("extract");
   if (extractQ) {
      markQ = 0;
   }

}



//////////////////////////////
//
// example -- example usage of the maxent program
//

void example(void) {
   cout <<
   "                                                                        \n"
   << endl;
}



//////////////////////////////
//
// usage -- gives the usage statement for the quality program
//

void usage(const char* command) {
   cout <<
   "                                                                        \n"
   << endl;
}



