//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Apr 15 20:05:01 PDT 2002
// Last Modified: Mon Apr 15 20:05:04 PDT 2002
// Last Modified: Sun Mar 20 23:37:41 PST 2005 Added *rit and *accel
// Last Modified: Sat Oct  8 22:14:11 PDT 2005 Added time interpolation
// Last Modified: Wed Mar 29 20:19:00 PST 2006 Fixed various errors for kglee
// Last Modified: Tue Apr  9 08:07:21 PDT 2013 Enabled multiple segment input
// Last Modified: Mon Nov 11 23:41:54 PST 2013 Output null interpretations
// Last Modified: Mon Nov 11 23:49:10 PST 2013 Defaut time is seconds
// Filename:      ...sig/examples/all/gettime.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/gettime.cpp
// Syntax:        C++; museinfo
//
// Description:   Add a spine indicating the time of performance for
//                a given line of music.
//

#include <string.h>
#include <stdio.h>
#include <math.h>

#include <vector>

#include "humdrum.h"


// function declarations
void   checkOptions             (Options& opts, int argc, char* argv[]);
void   example                  (void);
void   printAnalysis            (HumdrumFile& infile, vector<double>& timings,
                                 vector<double>& tempo, int setcount);
void   analyzeTiming            (HumdrumFile& infile, vector<double>& timings,
                                 vector<double>& tempo);
void   usage                    (const char* command);
void   interpolateGeometric     (HumdrumFile& infile, vector<double>& tempo,
                                 int startindex, int stopindex);
int    getNextTempoIndex        (HumdrumFile& infile, int startindex);
void   printtime                (const string& filename, double totaldur);
void   doLinearInterpolation    (HumdrumFile& infile, int setcount);
void   interpolateTimings       (vector<double>& timings, HumdrumFile& infile,
                                 int startindex, int endindex);
void   fixendingtimes           (vector<double>& timings, HumdrumFile& infile);

// global variables
Options      options;            // database for command-line arguments
int          appendQ    = 0;     // used with -a option
int          prependQ   = 0;     // used with -p option
int          debugQ     = 0;     // used with --debug option
double       offset     = 0.0;   // used with -o option
int          style      = 's';   // used with -m option
double       dtempo     = 60.0;  // used with -d option
int          tempoQ     = 0;     // used with -t option
int          changeQ    = 1;     // used with -C option
int          roundQ     = 1;     // used with -R option
int          totalQ     = 0;     // used with --total option
int          interpQ    = 0;     // used with -i option

///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
   HumdrumFileSet infiles;
   vector<double>  timings;
   vector<double>  tempo;

   checkOptions(options, argc, argv);
   infiles.read(options);

   double totaldur = 0.0;

   int i;
   for (i=0; i<infiles.getCount(); i++) {
      if (interpQ) {
         doLinearInterpolation(infiles[i], infiles.getCount());
      } else {
         analyzeTiming(infiles[i], timings, tempo);
         if (totalQ) {
            totaldur += timings[(int)timings.size()-1];
            if (infiles.getCount() > 1) {
               printtime(infiles[i].getFilename(),
                     timings[(int)timings.size()-1]);
            }
         } else {
            printAnalysis(infiles[i], timings, tempo, infiles.getCount());
         }
      }
   }

   if (totalQ) {
      printtime("", totaldur);
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////



///////////////////////////////
//
// doLinearInterpolation -- interpolate the timings values in the
//     score based on timing values which are currently present.
//
//

#define INVALIDTIME -100000
#define UNKNOWNTIME -1
// #define UNKNOWNTIME 0

void doLinearInterpolation(HumdrumFile& infile, int setcount) {
   infile.analyzeRhythm("4");

   // extract the timing data using -1 for undefined
   vector<double> timings(infile.getNumLines(), INVALIDTIME);
   vector<double> absbeat;
   double atime;

   for (int i=0; i<infile.getNumLines(); i++) {
      if (infile[i].getType() != E_humrec_data) {
         continue;
      }
      for (int j=0; j<infile[i].getFieldCount(); j++) {
         if (strcmp("**time", infile[i].getExInterp(j)) == 0) {
            if (strcmp(infile[i][j], ".") == 0) {
               timings[i] = UNKNOWNTIME;
            } else {
               sscanf(infile[i][j], "%lf", &atime);
               timings[i] = atime;
            }
            break;
         }

      }

   }

   int lasttimeindex = -1;
   for (int i=0; i<(int)timings.size(); i++) {
      if (timings[i] < 0) {
         continue;
      }

      if (lasttimeindex < 0) {
         lasttimeindex = i;
         continue;
      }

      interpolateTimings(timings, infile, lasttimeindex, i);

      lasttimeindex = i;

   }

   fixendingtimes(timings, infile);

   // now insert the interpolated timings back into the score.
   if (setcount > 1) {
      infile.printNonemptySegmentLabel(cout);
   }
   int tfound = 0;
   for (int i=0; i<infile.getNumLines(); i++) {
      if (infile[i].getType() == E_humrec_data) {
         tfound = 0;
         for (int j=0; j<infile[i].getFieldCount(); j++) {
            if ((tfound == 0) &&
                (strcmp(infile[i].getExInterp(j), "**time") == 0)) {
               tfound = 1;
               if (timings[i] < 0) {
                  cout << "0.0";
               } else {
                  cout << timings[i];
               }
            } else {
               cout << infile[i][j];
            }
            if (j < infile[i].getFieldCount() - 1) {
               cout << "\t";
            }
         }
         cout << "\n";
      } else {
         cout << infile[i] << "\n";
      }
   }

}



//////////////////////////////
//
// fixendingtimes --
//

void fixendingtimes(vector<double>& timings, HumdrumFile& infile) {
   int i;
   int lastdatai = -1;
   int lastlastdatai = -1;
   for (i=(int)timings.size()-1; i>=0; i--) {
      if ((lastdatai == -1) && (timings[i] >= 0)) {
         lastdatai = i;
      } else if ((lastlastdatai == -1) && (timings[i] >= 0)) {
         lastlastdatai = i;
         break;
      }
   }

   if ((lastdatai < 0) || (lastlastdatai < 0)) {
      return;
   }

   double beatincrement = (timings[lastdatai] - timings[lastlastdatai]) /
         (infile[lastdatai].getAbsBeat() - infile[lastlastdatai].getAbsBeat());
   double beatdiff = 0.0;


   for (i=lastdatai+1; i<(int)timings.size(); i++) {
      if (timings[i] == UNKNOWNTIME) {
         beatdiff = infile[i].getAbsBeat() - infile[lastdatai].getAbsBeat();
         timings[i] = timings[lastdatai] + beatdiff * beatincrement;
      }
   }

}




//////////////////////////////
//
// interpolateTimings -- do the actual interpolation work.
//

void interpolateTimings(vector<double>& timings, HumdrumFile& infile,
      int startindex, int endindex) {

   int i;
   double beatwidth = infile[endindex].getAbsBeat() -
                            infile[startindex].getAbsBeat();
   double timewidth = timings[endindex] - timings[startindex];
   double lbeat = 0.0;

   for (i=startindex+1; i<endindex; i++) {
      if (timings[i] == UNKNOWNTIME) {
         lbeat = infile[i].getAbsBeat() - infile[startindex].getAbsBeat();
         timings[i] = timewidth / beatwidth * lbeat + timings[startindex];
         if (timings[i] < timings[i-1]) {
            cerr << "Error new timing is less than old time" << endl;
            cerr << "New Time: " << timings[i] << endl;
            cerr << "Old Time: " << timings[i-1] << endl;
            cerr << "Beat width = " << beatwidth << endl;
            cerr << "Time width = " << timewidth << endl;
            cerr << "Start index = " << startindex << endl;
            cerr << "End   index = " << endindex << endl;
            exit(1);
         }
         if (roundQ) {
            if (style == 's') {
               timings[i] = (int)(1000.0 * timings[i] + 0.5) / 1000.0;
            } else {
               // milliseconds
               timings[i] = (int)(timings[i] + 0.5);
            }
         }
      }
   }

}



//////////////////////////////
//
// printtime --
//

void printtime(const string& filename, double totaldur) {
   if (filename[0] != '\0') {
      cout << filename << ":\t";
   } else {
      cout << "Total time:\t";
   }

   if (options.getBoolean("simple")) {
      cout << totaldur << endl;
      return;
   }

   int hours = 0;
   int minutes = 0;
   if (totaldur >= 3600) {
      hours = (int)(totaldur / 3600);
      totaldur -= hours * 3600;
   }
   if (totaldur >= 60) {
      minutes = (int)(totaldur / 60);
      totaldur -= minutes * 60;
   }
   if (hours > 0) {
      cout << hours;
      cout << ":";
      if (minutes < 10) {
         cout << "0";
      }
      cout << minutes << ":";
      if (totaldur < 10) {
         cout << "0";
      }
      cout << totaldur;
      cout << " hours" << endl;
   } else if (minutes > 0) {
      cout << minutes << ":";
      if (totaldur < 10) {
         cout << "0";
      }
      cout << totaldur;
      cout << " minutes" << endl;
   } else {
      cout << totaldur << " seconds" << endl;
   }

}



//////////////////////////////
//
// analyzeTiming -- determing the absolute time position of each
//     line in the file.
//

void analyzeTiming(HumdrumFile& infile, vector<double>& timings,
      vector<double>& tempo) {
   infile.analyzeRhythm("4");

   timings.resize(infile.getNumLines());
	std::fill(timings.begin(), timings.end(), 0.0);
   tempo.resize(infile.getNumLines());
	std::fill(tempo.begin(), tempo.end(), dtempo);
   tempo[0] = dtempo;
   double currtempo = dtempo;
   double input = 0.0;
   int count;
   vector<double> tempoindicators;
   vector<int>    tempoindex;

   tempoindicators.reserve(1000);
   tempoindex.reserve(1000);

   double ritard = -1.0;
   double accel  = -2.0;

   for (int i=1; i<infile.getNumLines(); i++) {
      // check for new tempo...
      if (infile[i].getType() == E_humrec_interpretation) {
         for (int j=0; j<infile[i].getFieldCount(); j++) {
            if (strcmp(infile[i].getExInterp(j), "**kern") != 0) {
               continue;
            }
            if (strncmp(infile[i][j], "*MM", 3) == 0) {
               count = sscanf(infile[i][j], "*MM%lf", &input);
               if (count == 1) {
                  currtempo = input;
                  tempoindicators.push_back(currtempo);
                  tempoindex.push_back(i);
                  if (i > 0) {
                     tempo[i-1] = currtempo;
                  }
                  tempo[i] = currtempo;
               }
            } else if (strcmp(infile[i][j], "*accel") == 0) {
               tempoindicators.push_back(accel);
               tempoindex.push_back(i);
            } else if (strcmp(infile[i][j], "*rit") == 0) {
               tempoindicators.push_back(ritard);
               tempoindex.push_back(i);
            }
            break;
         }
      }

      tempo[i] = currtempo;
      timings[i] = timings[i-1] + (infile[i].getAbsBeat() -
            infile[i-1].getAbsBeat()) * 60.0/currtempo;
   }

   if (!changeQ) {
      // do not adjust for *rit and *accel markers
      return;
   }

   // go back and geometrically interpolate the tempo markings
   // when there are *accel and *rit markers in the data.

   for (int i=1; i<(int)tempoindicators.size()-1; i++) {
      if ((tempoindicators[i] < 0) && (tempoindicators[i+1] > 0)) {
         interpolateGeometric(infile, tempo, tempoindex[i], tempoindex[i+1]);
      }
   }


   // the following code will have to be debugged (mostly for off-by-one
   // errors).

   // adjust the timing values
   double beatdiff;
   double increment = 1.0/32.0;
   double starttempo;
   double stoptempo;
   double timesum;
   double logtem1;
   double logtem2;
   double tfraction;
   double ftempo;
   double beatduration;
   int    ntindex;
   double startbeat;
   double stopbeat;

   for (int i=1; i<infile.getNumLines(); i++) {
      starttempo   = tempo[i];
      ntindex      = getNextTempoIndex(infile, i);
      stoptempo    = tempo[ntindex];

      startbeat    = infile[i-1].getAbsBeat();
      stopbeat     = infile[i].getAbsBeat();
      beatduration = stopbeat - startbeat;
      beatdiff     = beatduration;

      if (starttempo == stoptempo) {
         timings[i] = timings[i-1] + beatduration * 60/tempo[i];
      } else {
         // calculate the time as the tempo changes
         logtem1      = log10(starttempo);
         logtem2      = log10(stoptempo);

         timesum = 0.0;
         while (beatdiff >= increment) {
            tfraction = ((beatduration - beatdiff) / beatduration);
            ftempo = pow(10.0, logtem1 + (logtem2 - logtem1) * tfraction);
            timesum += increment * 60/ftempo;
            beatdiff -= increment;
         }

         if (beatduration > 0.0) {
            tfraction = ((beatduration - beatdiff) / beatduration);
            ftempo = pow(10.0, logtem1 + (logtem2 - logtem1) * tfraction);
            timesum += beatdiff * 60/ftempo;
         } else {
            timesum = 0.0;
         }

         timings[i] = timings[i-1] + timesum;
      }
   }

}



//////////////////////////////
//
// getNextTempoIndex --
//

int getNextTempoIndex(HumdrumFile& infile, int startindex) {
   int i = startindex + 1;
   int stopindex = -1;
   while (i < infile.getNumLines()) {
      if (infile[i].getAbsBeat() > infile[startindex].getAbsBeat()) {
         stopindex = i;
         break;
      }
      i++;
   }

   if (stopindex < 0) {
      stopindex = infile.getNumLines() - 1;
   }

   return stopindex;
}



//////////////////////////////
//
// interpolateGeometric --
//

void interpolateGeometric(HumdrumFile& infile, vector<double>& tempo,
      int startindex, int stopindex) {
   double duration   = 0.0;
   double newtempo   = 0.0;
   double beatstart  = infile[startindex].getAbsBeat();
   double beatstop   = infile[stopindex].getAbsBeat();
   double starttempo = tempo[startindex];
   double stoptempo  = tempo[stopindex];
   double logstart   = log10(starttempo);
   double logstop    = log10(stoptempo);
   double tfraction;
   duration = beatstop - beatstart;
   int i;

   for (i=startindex+1; i<stopindex; i++) {
      if (infile[i].getAbsBeat() == beatstart) {
         tempo[i] = tempo[startindex];
      } else if (infile[i].getAbsBeat() == beatstop) {
         tempo[i] = tempo[stopindex];
      } else {
         tfraction = (infile[i].getAbsBeat() - beatstart)/duration;
         newtempo = pow(10.0, (logstart + (logstop - logstart) * tfraction));
         tempo[i] = newtempo;
      }
   }

}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("T|total=b",         "calculate total time duration of input.");
   opts.define("C|no-change=b",     "do not do ritard and accels.");
   opts.define("R|no-round=b",      "do not round millisecond values.");
   opts.define("a|append=b",        "append analysis to input data");
   opts.define("p|prepend=b",       "prepend analysis to input data");
   opts.define("i|interpolation=b", "interpolate times in prexisting spine");
   opts.define("o|offset=d:0.0",    "starting time offset");
   opts.define("m|ms|milliseconds=b", "use seconds as the time unit");
   opts.define("simple=b",          "print total time in pure seconds");
   opts.define("t|tempo=b",         "display tempo rather than time");
   opts.define("d|default-tempo=d:60.0",  "default tempo to use if none in file");

   opts.define("debug=b",   "trace input parsing");
   opts.define("author=b",  "author of the program");
   opts.define("version=b", "compilation information");
   opts.define("example=b", "example usage");
   opts.define("h|help=b",  "short description");
   opts.process(argc, argv);

   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, April 2002" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 15 April 2002" << endl;
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

   tempoQ  =  opts.getBoolean("tempo");
   changeQ = !opts.getBoolean("no-change");
   roundQ  = !opts.getBoolean("no-round");
   interpQ =  opts.getBoolean("interpolation");
   debugQ  =  opts.getBoolean("debug");
   offset  =  opts.getDouble("offset");
   if (opts.getBoolean("milliseconds")) {
      style = 'm';
   } else {
      style = 's';
   }
   dtempo = opts.getDouble("default-tempo");

   // don't let people set the tempo to slow or negative
   if (dtempo < 1.0) {
      dtempo = 60.0;
   }

   totalQ   = opts.getBoolean("total");
   appendQ  = opts.getBoolean("append");
   prependQ = opts.getBoolean("prepend");
   if (appendQ) { // exclusive options
      prependQ = 0;
   }
}



//////////////////////////////
//
// example -- example usage of the addtime program
//

void example(void) {
   cout <<
   "                                                                        \n"
   << endl;
}



//////////////////////////////
//
// printAnalysis --
//

void printAnalysis(HumdrumFile& infile, vector<double>& timings,
      vector<double>& tempo, int setcount) {
   int tempomark = 0;
   int i, j;
   int m;
   if (setcount > 1) {
      infile.printNonemptySegmentLabel(cout);
   }
   for (i=0; i<infile.getNumLines(); i++) {
      switch (infile[i].getType()) {
      case E_humrec_global_comment:
      case E_humrec_bibliography:
      case E_humrec_none:
      case E_humrec_empty:
         cout << infile[i].getLine() << "\n";
         break;
      case E_humrec_data:

         if (tempomark == 0) {
            tempomark = 1;
            if (appendQ) {
               for (j=0; j<infile[i].getFieldCount(); j++) {
                  cout << "*MM" << tempo[i] << "\t";
               }
            }
            cout << "*MM" << tempo[i];
            if (prependQ) {
               for (j=0; j<infile[i].getFieldCount(); j++) {
                  cout << "\t" << "*MM" << tempo[i];
               }
            }
            cout << "\n";
         }

         if (appendQ) {
            cout << infile[i].getLine() << "\t";
         }

         if (tempoQ) {
            if (roundQ) {
               cout << (int)(tempo[i] + 0.5);
            } else {
               cout << tempo[i];
            }
         } else {
            if (style == 's') {
               if (roundQ) {
                  cout << (int)(1000.0 * (timings[i] + offset) + 0.5)/1000.0;
               } else {
                  cout << timings[i] + offset;
               }
            } else {
               if (roundQ) {
                  cout << (int)((timings[i] + offset + 0.0005)* 1000);
               } else {
                  cout << (timings[i] + offset) * 1000.0;
               }
            }
         }

         if (prependQ) {
            cout << "\t" << infile[i].getLine();
         }

         cout << "\n";
         break;
      case E_humrec_data_comment:
         if (appendQ) {
            cout << infile[i].getLine() << "\t";
         }
         cout << "!";
         if (prependQ) {
            cout << "\t" << infile[i].getLine();
         }
         cout << "\n";
         break;
      case E_humrec_data_measure:
         if (appendQ) {
            cout << infile[i].getLine() << "\t";
         }
         if (infile[i].equalFieldsQ("**kern")) {
            cout << infile[i][0];
         } else {
            cout << "=";
         }
         if (prependQ) {
            cout << "\t" << infile[i].getLine();
         }
         cout << "\n";
         break;
      case E_humrec_data_interpretation:
         if (strncmp(infile[i][0], "**", 2) == 0) {
            if (appendQ) {
               cout << infile[i].getLine() << "\t";
            }
            if (tempoQ) {
               cout << "**tempo";
            } else {
               cout << "**time";
            }
            if (prependQ) {
               cout << "\t" << infile[i].getLine();
            }
            cout << "\n";

            // print units marker
            if (appendQ) {
               for (m=0; m<infile[i].getFieldCount(); m++) {
                  cout << "*";
                  cout << "\t";
               }
            }
            cout << "*u=";
            switch (style) {
               case 's': cout << "sec";  break;
               case 'm': cout << "msec"; break;
               default : cout << "msec"; break;
            }
            if (prependQ) {
               cout << "\t";
               for (m=0; m<infile[i].getFieldCount(); m++) {
                  cout << "*";
                  if (m < infile[i].getFieldCount() - 1) {
                     cout << "\t";
                  }
               }
            }
            cout << "\n";
         } else {
            if (appendQ) {
               cout << infile[i] << '\t';
            }
            if (strcmp(infile[i][0], "*-") == 0) {
               cout << "*-";
            } else if (strncmp(infile[i][0], "*MM", 3) == 0) {
               tempomark = 1;
               cout << infile[i][0];
            } else if (strncmp(infile[i][0], "*>", 2) == 0) {
               // expansion label
               cout << infile[i][0];
            } else {
               cout << "*";
            }

            if (prependQ) {
               cout << "\t" << infile[i];
            }
            cout << "\n";
         }
         break;
      }

   }
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



