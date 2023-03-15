//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu Dec 16 13:19:14 PST 2004
// Last Modified: Thu Dec 16 13:19:16 PST 2004
// Last Modified: Wed Apr 17 00:38:30 PDT 2013 Enabled multiple segment input
// Last Modified: Tue Jul 15 11:40:47 PDT 2014 Added written pitch count.
// Last Modified: Mon Sep  1 13:50:43 PDT 2014 Added -P option.
// Filename:      ...sig/examples/all/notecount.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/notecount.cpp
// Syntax:        C++; museinfo
//
// Description:   Counts the number of notes in **kern data.
//

#include "humdrum.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <string>

class CountInfo {
   public:
      CountInfo(void);
      void clear(void);
      int rests;
      int sounding;
      int written;
      string filename;
      Array<int> base40pitch_sounding;
      Array<int> base40pitchclass_sounding;
      Array<int> base40pitch_written;
      Array<int> base40pitchclass_written;
};


class WorkInfo : public CountInfo {
   public:
      WorkInfo(void);
     ~WorkInfo();
      void clear(void);
      void setPartSize(int count);
      Array<CountInfo*> part;
};


CountInfo::CountInfo(void) {
   clear();
}


WorkInfo::WorkInfo(void) {
   part.setSize(0);
   CountInfo::clear();
}


WorkInfo::~WorkInfo() {
   clear();
}


void WorkInfo::setPartSize(int count) {
   int i;
   if (part.getSize() > 0) {
      for (i=0; i<part.getSize(); i++) {
         delete part[i];
         part[i] = NULL;
      }
      part.setSize(0);
   }
   part.setSize(count);
   part.allowGrowth(0);
   for (i=0; i<count; i++)  {
      part[i] = new CountInfo;
   }
}


void WorkInfo::clear(void) {
   CountInfo::clear();
   int i;
   for (i=0; i<part.getSize(); i++) {
      delete part[i];
      part[i] = NULL;
   }
   part.setSize(0);
}
   

void CountInfo::clear(void) {
   base40pitch_sounding.setSize(400);
   base40pitchclass_sounding.setSize(40);
   base40pitch_written.setSize(400);
   base40pitchclass_written.setSize(40);

   base40pitch_sounding.setAll(0);
   base40pitchclass_sounding.setAll(0);
   base40pitch_written.setAll(0);
   base40pitchclass_written.setAll(0);

   base40pitch_sounding.allowGrowth(0);
   base40pitchclass_sounding.allowGrowth(0);
   base40pitch_written.allowGrowth(0);
   base40pitchclass_written.allowGrowth(0);

   rests    = 0;
   sounding = 0;
   written  = 0;

   filename.clear();
}


// function declarations
void      checkOptions       (Options& opts, int argc, char* argv[]);
void      example            (void);
void      usage              (const char* command);
void      processFile        (HumdrumFile& infile, WorkInfo& cinfo);
void      printPitchClassInfo(Array<WorkInfo>& counts);
void      printPitchInfo     (Array<WorkInfo>& counts);

// global variables
Options   options;           // database for command-line arguments
int       summaryQ   = 0;    // used with -s option
int       totalQ     = 1;    // used with -T option
int       pcQ        = 0;    // used with -c option
int       pitchQ     = 0;    // used with -p option
int       writtenQ   = 0;    // used with -w option
int       twelveQ    = 1;    // used with -e option
int       partQ      = 0;    // used with -P option


///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
   HumdrumFileSet infiles;
   checkOptions(options, argc, argv);
   infiles.read(options);
   Array<WorkInfo> counts;
   counts.setSize(infiles.getCount());
   string filename;
   int i, j;

   for (i=0; i<infiles.getCount(); i++) {
      processFile(infiles[i], counts[i]);
   }

   if (summaryQ) {
      int sounding    = 0;
      int written     = 0;
      int rests       = 0;
      for (i=0; i<counts.getSize(); i++) {
         sounding += counts[i].sounding;
         written  += counts[i].written;
         rests    += counts[i].rests;
      }
      cout << sounding << "\t" << written << "\t" << rests << endl;
   } else if (pcQ) {
      printPitchClassInfo(counts);
   } else if (pitchQ) {
      printPitchInfo(counts);
   } else {
      cout << "**file\t";
      if (partQ) {
         cout << "**part\t";
      }
      cout << "**sound\t**print\t**rests\n";
      for (i=0; i<counts.getSize(); i++) {
         filename = counts[i].filename;
         if (filename.compare("") == 0) {
            filename = ".";
         }
         cout << filename           << "\t";
         if (partQ) {
            cout << "0\t";
         }
         cout << counts[i].sounding << "\t";
         cout << counts[i].written  << "\t";
         cout << counts[i].rests    << endl;
         if (partQ) {
            for (j=0; j<counts[i].part.getSize(); j++) {
               cout << ".\t" << (j+1) << "\t";
               cout << counts[i].part[j]->sounding << "\t";
               cout << counts[i].part[j]->written  << "\t";
               cout << counts[i].part[j]->rests    << endl;
            }
         }
      }
      if (totalQ && (counts.getSize() > 1)) {
         int sounding    = 0;
         int written     = 0;
         int rests       = 0;
         for (i=0; i<counts.getSize(); i++) {
            sounding += counts[i].sounding;
            written  += counts[i].written;
            rests    += counts[i].rests;
         }
         cout << "!!sounding-notes:\t" << sounding << endl;
         cout << "!!written-notes:\t" << written << endl;
         cout << "!!written-rests:\t" << rests << endl;
      }
      cout << "*-\t";
      if (partQ) {
         cout << "*-\t";
      }
      cout << "*-\t*-\t*-\n";
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////


void printPitchInfo(Array<WorkInfo>& counts) {
   // do nothing for now...
}


//////////////////////////////
//
// printPitchClassInfo --
//

void printPitchClassInfo(Array<WorkInfo>& counts) {
   Array<string> labels;
   Array<int>    totals12;
   Array<int>    totals40;
   if (twelveQ) {
      labels.setSize(12);
   } else {
      labels.setSize(40);
   }
   totals40.setSize(40);
   totals40.setAll(0);
   totals40.allowGrowth(0);
   totals12.setSize(12);
   totals12.setAll(0);
   totals12.allowGrowth(0);
   labels.allowGrowth(0);
   int base12;
   char buffer[1024] = {0};
   int i, j;


   for (i=0; i<counts.getSize(); i++) {
      if (twelveQ) {
         for (j=0; j<40; j++) {
            base12 = (Convert::base40ToMidiNoteNumber(j) + 120) % 12;
            if (writtenQ) {
               totals12[base12] += counts[i].base40pitchclass_written[j];
            } else {
               totals12[base12] += counts[i].base40pitchclass_sounding[j];
            }
         }
      }
      for (j=0; j<40; j++) {
         if (writtenQ) {
            totals40[j] += counts[i].base40pitchclass_written[j];
         } else {
            totals40[j] += counts[i].base40pitchclass_sounding[j];
         }
      }
   }

   if (twelveQ) {
      // 0	2	C
      // 0	38	B#
      // 0	6	D--
      if ((totals40[2] > totals40[38]) && (totals40[2] > totals40[6])) {
         labels[0] = "C";
      } else if (totals40[38] > totals40[6]) {
         labels[0] = "B#";
      } else {
         labels[0] = "D--";
      }

      // 1	3	C#
      // 1	39	B##
      // 1	7	D-
      if ((totals40[3] > totals40[39]) && (totals40[3] > totals40[7])) {
         labels[1] = "C#";
      } else if (totals40[39] > totals40[7]) {
         labels[1] = "B##";
      } else {
         labels[1] = "D-";
      }

      // 2	12	E--
      // 2	4	C##
      // 2	8	D
      if ((totals40[12] > totals40[4]) && (totals40[12] > totals40[8])) {
         labels[2] = "E--";
      } else if (totals40[4] > totals40[8]) {
         labels[2] = "C##";
      } else {
         labels[2] = "D";
      }

      // 3	9	D#
      // 3	13	E-
      // 3	17	F--
      if ((totals40[9] > totals40[13]) && (totals40[9] > totals40[17])) {
         labels[3] = "D#";
      } else if (totals40[13] > totals40[17]) {
         labels[3] = "E-";
      } else {
         labels[3] = "F--";
      }

      // 4	10	D##
      // 4	14	E
      // 4	18	F-
      if ((totals40[10] > totals40[14]) && (totals40[10] > totals40[18])) {
         labels[4] = "D##";
      } else if (totals40[14] > totals40[18]) {
         labels[4] = "E";
      } else {
         labels[4] = "F-";
      }

      // 5	15	E#
      // 5	19	F
      // 5	23	G--
      if ((totals40[15] > totals40[19]) && (totals40[15] > totals40[23])) {
         labels[5] = "E#";
      } else if (totals40[19] > totals40[23]) {
         labels[5] = "F";
      } else {
         labels[5] = "G--";
      }

      // 6	16	E##
      // 6	20	F#
      // 6	24	G-
      if ((totals40[16] > totals40[20]) && (totals40[16] > totals40[24])) {
         labels[6] = "E##";
      } else if (totals40[20] > totals40[24]) {
         labels[6] = "F#";
      } else {
         labels[6] = "G-";
      }

      // 7	21	F##
      // 7	25	G
      // 7	29	A--
      if ((totals40[21] > totals40[25]) && (totals40[21] > totals40[29])) {
         labels[7] = "F##";
      } else if (totals40[25] > totals40[29]) {
         labels[7] = "G";
      } else {
         labels[7] = "A--";
      }

      // 8	26	G#
      // 8	30	A-
      if (totals40[26] > totals40[30]) {
         labels[8] = "G#";
      } else {
         labels[8] = "A-";
      }

      // 9	27	G##
      // 9	31	A
      // 9	35	B--
      if ((totals40[27] > totals40[31]) && (totals40[27] > totals40[35])) {
         labels[9] = "G##";
      } else if (totals40[31] > totals40[35]) {
         labels[9] = "A";
      } else {
         labels[9] = "B--";
      }

      // 10	0	C--
      // 10	32	A#
      // 10	36	B-
      if ((totals40[0] > totals40[32]) && (totals40[0] > totals40[36])) {
         labels[10] = "C--";
      } else if (totals40[32] > totals40[36]) {
         labels[10] = "A#";
      } else {
         labels[10] = "B-";
      }

      // 11	1	C-
      // 11	33	A##
      // 11	37	B
      if ((totals40[1] > totals40[33]) && (totals40[1] > totals40[37])) {
         labels[11] = "C-";
      } else if (totals40[33] > totals40[37]) {
         labels[11] = "A##";
      } else {
         labels[11] = "B-";
      }
   } else {
      for (i=0; i<40; i++) {
         labels[i] = Convert::base40ToKern(buffer, 1024, i+40*3);
      }
   }

   string filename;
   Array<int> pc12counts(12);
   if (twelveQ) {
      for (i=0; i<totals12.getSize(); i++) {
         cout << "**count\t";
      }
      cout << "**total\t**file" << endl;
      if (writtenQ) {
         cout << "!! Written note counts by pitch class" << endl;
      } else {
         cout << "!! Sounding note counts by pitch class" << endl;
      }
      for (i=0; i<totals12.getSize(); i++) {
         cout << "!" << labels[i] << "\t";
      }
      cout << "!all\t!filename" << endl;
      for (i=0; i<counts.getSize(); i++ ) {
         pc12counts.setAll(0);
         for (j=0; j<40; j++) {
            base12 = (Convert::base40ToMidiNoteNumber(j) + 120) % 12;
            if (writtenQ) {
               pc12counts[base12] += counts[i].base40pitchclass_written[j];
            } else {
               pc12counts[base12] += counts[i].base40pitchclass_sounding[j];
            }
         }

         for (j=0; j<12; j++) {
            cout << pc12counts[j] << "\t";
         }
         if (writtenQ) {
            cout << counts[i].written;
         } else {
            cout << counts[i].sounding;
         }
         cout << "\t";
         filename = counts[i].filename;
         if (filename.compare("") == 0) {
            filename = ".";
         }
         cout << filename << endl;
      }
      if (counts.getSize() > 1) {
         int sum = 0;
         for (j=0; j<totals12.getSize(); j++) {
            sum += totals12[j];
         }
         for (j=0; j<totals12.getSize(); j++) {
            cout << totals12[j] << "\t";
         }
         cout << sum;
         cout << "\t[TOTAL]";
         cout << endl;
      }
      for (j=0; j<12; j++) {
         cout << "*-\t";
      }
      cout << "*-\t*-";
      cout << endl;
   } else {


   }


}



//////////////////////////////
//
// processFile --
//

void processFile(HumdrumFile& infile, WorkInfo& cinfo) {
   int i, j, k;
   cinfo.clear();
   cinfo.filename = infile.getFilename();
   Array<Array<char> > subtokens;
   int base40   = 0;
   int base40pc = 0;
   Array<int> ktracks;
   infile.getTracksByExInterp(ktracks, "**kern");

   Array<int> rktracks;
   rktracks.setSize(infile.getMaxTracks() + 1);
   rktracks.setAll(-1);
   for (i=0; i<ktracks.getSize(); i++) {
      rktracks[ktracks[i]] = i;
   }
   cinfo.setPartSize(ktracks.getSize());
   int pindex;

   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].getType() != E_humrec_data) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }
         if (strcmp(infile[i][j], ".") == 0) {
            continue;
         }

         pindex = rktracks[infile[i].getPrimaryTrack(j)];

         infile[i].getTokens(subtokens, j);
         
         for (k=0; k<subtokens.getSize(); k++) {
            if (strchr(subtokens[k].getBase(), 'r') != NULL) {
               cinfo.rests++;
               cinfo.part[pindex]->rests++;
               continue;
            }
            cinfo.written++;
            cinfo.part[pindex]->written++;
            if (pcQ) {
               base40 = Convert::kernToBase40(subtokens[k].getBase());
               base40pc = base40 % 40;
               cinfo.base40pitch_written[base40]++;
               cinfo.base40pitchclass_written[base40pc]++;
               // not keeping track of base40 for parts for now.
            }
            if (strchr(subtokens[k].getBase(), '_') != NULL) {
               continue;
            }
            if (strchr(subtokens[k].getBase(), ']') != NULL) {
               continue;
            }
            cinfo.sounding++;
            cinfo.part[pindex]->sounding++;
            if (pcQ) {
               cinfo.base40pitch_sounding[base40]++;
               cinfo.base40pitchclass_sounding[base40pc]++;
            }
         }
      }
   }
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("s|summary=b", "just give the raw numbers");
   opts.define("T|no-total=b", "don't give the total count");
   opts.define("c|pc|pitch-class=b", "Display counts by pitch class");
   opts.define("p|pitch=b", "Display counts by absolute pitch");
   opts.define("P|parts=b", "Display counts by parts as well");
   opts.define("e|enharmonic=b", "don't collapse to base12");
   opts.define("w|written=b", "written pitch for pitch(class) counts");
   opts.define("debug=b");                // determine bad input line num
   opts.define("author=b");               // author of program
   opts.define("version=b");              // compilation info
   opts.define("example=b");              // example usages
   opts.define("h|help=b");               // short description
   opts.process(argc, argv);

   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Dec 2004" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: July 2014" << endl;
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

   writtenQ  =  opts.getBoolean("written");
   summaryQ  =  opts.getBoolean("summary");
   partQ     =  opts.getBoolean("parts");
   totalQ    = !opts.getBoolean("no-total");
   pcQ       =  opts.getBoolean("pitch-class");
   pitchQ    =  opts.getBoolean("pitch");
   if (pcQ && pitchQ) {
      pitchQ = 0;
   }
   twelveQ   = !opts.getBoolean("enharmonic");
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



