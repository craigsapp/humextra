//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Aug 25 16:56:33 PDT 2003
// Last Modified: Sat Nov  9 07:41:17 PST 2013 Added -n, -p, -i options.
// Filename:      ...sig/examples/all/blank.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/blank.cpp
// Syntax:        C++; museinfo
//
// Description:   Create a blank spine with the same number of lines
//                as the input file.
//

#include "humdrum.h"

// function declarations
void      checkOptions       (Options& opts, int argc, char* argv[]);
void      example            (void);
void      usage              (const char* command);
ostream&  printOutput        (ostream& out, HumdrumFile& infile);
ostream&  printBlanks        (ostream& out, HumdrumFile& infile, int line, 
                              const char* string, int count);
ostream&  printInterpretation(ostream& out, HumdrumFile& infile, int line, 
                              int count);
ostream&  printExclusiveInterpretations(ostream& out, HumdrumFile& infile, 
                              int line, int count, 
                              Array<SigString>& exinterps);
int       afterMeasure       (HumdrumFile& infile, int line);
void      blankSpines        (HumdrumFile& infile, Array<int>& trackstates);
void      addRests           (HumdrumFile& infile, Array<int>& trackstates);
void      fillFieldData      (Array<int>& field, Array<int>& subfield, 
                              Array<int>& model, const char* fieldstring, 
                              HumdrumFile& infile);
void      processFieldEntry  (Array<int>& field, Array<int>& subfield, 
                              Array<int>& model, const char* string, 
                              HumdrumFile& infile);
void      removeDollarsFromString(Array<char>& buffer, int maxtrack);

// global variables
Options   options;            // database for command-line arguments
int       appendQ    = 0;     // used with -a option
int       prependQ   = 0;     // used with -p option
int       restQ      = 0;     // used with -r option
int       invisibleQ = 0;     // used with -y option
int       timesigQ   = 0;     // used with -t option
int       keysigQ    = 0;     // used with -k option
int       Count      = 0;     // used with -c option
int       spineQ     = 0;     // used with -s/-S options
int       inverseQ   = 0;     // used with -S option
const char* fieldstring = ""; // used with -s/-S options
Array<SigString> Exinterps;   // used with -i option

///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
   checkOptions(options, argc, argv);
   HumdrumFileSet infiles;
   infiles.read(options);

   for (int i=0; i<infiles.getCount(); i++) {
      printOutput(cout, infiles[i]);
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////

//////////////////////////////
//
// printOutput --
//

ostream& printOutput(ostream& out, HumdrumFile& infile) {
   if (restQ) {
      infile.analyzeRhythm("4");
   }
   Array<int> trackstates;

   if (spineQ) {
      blankSpines(infile, trackstates);
      if (restQ) {
         addRests(infile, trackstates);
      }
      out << infile;
      return out;
   }

   for (int i=0; i<infile.getNumLines(); i++) {
      switch (infile[i].getType()) {
         case E_humrec_data_comment:
            printBlanks(out, infile, i, "!", Count);
            break;
         case E_humrec_data_kern_measure:
            printBlanks(out, infile, i, infile[i][0], Count);
            break;
         case E_humrec_interpretation:
            printInterpretation(out, infile, i, Count);
            break;
         case E_humrec_data:
            printBlanks(out, infile, i, ".", Count);
            break;
         case E_humrec_none:
         case E_humrec_empty:
         case E_humrec_global_comment:
         case E_humrec_bibliography:
         default: // unknown line type, so just echo it to output
                  // (such as a technically illegal blank line)
            out << infile[i] << "\n";
            break;
      }
   }
   return out;
}



//////////////////////////////
//
// addRests --
//

void addRests(HumdrumFile& infile, Array<int>& trackstates) {
   char buffer[1024] = {0};
   int i, j;
   int barline  = -1;
   int track;
   RationalNumber rn;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isBarline()) {
         barline = i;
         continue;
      }
      if (!infile[i].isData()) {
         continue;
      }
      if (infile[i].getDuration() == 0.0) {
         continue;
      }

      if (barline > 0) {
         rn = infile[barline].getBeatR();
         barline = 0;
      } else if (barline < 0) {
         rn = infile.getPickupDurationR();
         barline = 0;
      } else {
         continue;
      }
      Convert::durationRToKernRhythm(buffer, rn);
      strcat(buffer, "r");
      if (invisibleQ) {
         strcat(buffer, "yy");
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            // don't add rests to non-kern spines
            continue;
         }
         track = infile[i].getPrimaryTrack(j);
         if (!trackstates[track]) {
            // don't process unblanked spines
            continue;
         }
         infile[i].changeField(j, buffer);
      }
   }
}



//////////////////////////////
//
// blankSpines -- erase contents of specific spines.
//

void blankSpines(HumdrumFile& infile, Array<int>& trackstates) {
   Array<int>   field;              // used with -s option

   // ignoring subfield and model:
   Array<int>   subfield;           // used with -s option
   Array<int>   model;              // used with -p, or -e options and similar

   fillFieldData(field, subfield, model, fieldstring, infile);

   int i;
   int j;
   int track;
   trackstates.setSize(infile.getMaxTracks()+1);
   trackstates.setAll(0);
   trackstates.allowGrowth(0);

   for (i=0; i<field.getSize(); i++) {
      trackstates[field[i]] = 1;
   }
   if (inverseQ) {
      for (i=0; i<trackstates.getSize(); i++) {
         trackstates[i] = !trackstates[i];
      }
   }

   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isData()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         track = infile[i].getPrimaryTrack(j);
         if (trackstates[track] == 0) {
            continue;
         }
         if (strcmp(infile[i][j], ".") == 0) {
            continue;
         }
         infile[i].changeField(j, ".");
      }
   }
}



//////////////////////////////
//
// printInterpretation --
//

ostream& printInterpretation(ostream& out, HumdrumFile& infile, int line, 
      int count) {

   if (strncmp(infile[line][0], "**", 2) == 0) {
      printExclusiveInterpretations(out, infile, line, count, Exinterps);
   } else if (strcmp(infile[line][0], "*-") == 0) {
      printBlanks(out, infile, line, "*-", count);
   } else if (strncmp(infile[line][0], "*>", 2) == 0) {
      // expansion labels, such as *>[A,A,B], *>A, *B.
      printBlanks(out, infile, line, infile[line][0], count);
   } else {
      printBlanks(out, infile, line, "*", count);
   }
   return out;
}



//////////////////////////////
//
// printExclusiveInterpretations -- print exclusive interpretations
//    which may be the same or different for each new blank spine.
//

ostream& printExclusiveInterpretations(ostream& out, HumdrumFile& infile, 
      int line, int count, Array<SigString>& exinterps) {
   if (appendQ) {
      out << infile[line] << '\t';
   }
   int j, jj;
   for (j=0; j<count; j++) {
      jj = j;
      if (jj > exinterps.getSize() - 1) {
         jj = exinterps.getSize() - 1 ;
      }
      out << "**" << exinterps[jj];
      if (j < count - 1) {
         out << '\t';
      }
   }
   if (prependQ) {
      out << '\t' << infile[line];
   } 
   out << '\n';
   return out;
}



//////////////////////////////
//
// printBlanks --
//

ostream& printBlanks(ostream& out, HumdrumFile& infile, int line, 
     const char* string, int count) {
   if (appendQ) {
      out << infile[line] << '\t';
   }
   int j;
   int measureline = -1;
   RationalNumber rn;
   char buffer[1024] = {0};
   for (j=0; j<count; j++) {
      if (restQ && (strcmp(string, ".") == 0)) {
         measureline = afterMeasure(infile, line);
         if (measureline >= 0) {
            if (measureline > 0) {
               rn = infile[measureline].getBeatR();
            } else if (measureline == 0) {
               rn = infile.getPickupDurationR();
            }
            Convert::durationRToKernRhythm(buffer, rn);
            cout << buffer << "r";
            if (invisibleQ) {
               cout << "yy";
            }
         } else {
            out << string;
         }
      } else if (timesigQ && (infile[line].isTimeSig(0)
            || infile[line].isTempo(0) || infile[line].isMetSig(0))) {
         out << infile[line][0];
      } else if (keysigQ && (infile[line].isKeySig(0) 
            || infile[line].isKey(0))) {
         out << infile[line][0];
      } else {
         out << string;
      }
      if (j < count - 1) {
         out << '\t';
      }
   }
   if (prependQ) {
      out << '\t' << infile[line];
   } 
   out << '\n';
   return out;
}


//////////////////////////////
//
// afterMeasure -- Search backwards from the given line for the
//    previous barline.  If there is another data line with a non-zero
//    duration before the previous line, then return -1 since the entry
//    is not at the start of a measure.
//

int afterMeasure(HumdrumFile& infile, int line) {
   int i;
   double duration;
   for (i=line-1; i>=0; i--) {
      if (i == 0) {
         // deal with pickup measure in the calling function.
         return 0;
      }
      if (infile[i].isMeasure()) {
         return i;
      }
      if (infile[i].isData()) {
         duration = infile[i].getDuration();
         if (duration != 0.0) {
            return -1;
         }
      }
   }

   return -1;
}


//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("a|assemble|append=b",  "append analysis spine to input data");
   opts.define("p|prepend=b",          "new spines at start of input lines");
   opts.define("s|spines=s",           "list of spines to blank");
   opts.define("S|not-spines=s",       "list of spines not to blank");
   opts.define("n|count=i",            "number of spines to add");
   opts.define("i|x|exinterp=s:blank", "set column exclusive interpretations");
   opts.define("r|rests=b",            "put rests in **kern added spines");
   opts.define("y|invisible=b",        "added rests are made invisible");
   opts.define("k|keysig=b",           "added key sigs to blank spines");
   opts.define("t|timesig=b",          "added time sigs to blank spines");

   opts.define("debug=b");                // determine bad input line num
   opts.define("author=b");               // author of program
   opts.define("version=b");              // compilation info
   opts.define("example=b");              // example usages
   opts.define("h|help=b");               // short description
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Aug 2003" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: Nov 2013" << endl;
      cout << "compiled: " << __DATE__ << endl;
      cout << MUSEINFO_VERSION << endl;
      exit(0);
   } else if (opts.getBoolean("help")) {
      usage(opts.getCommand());
      exit(0);
   } else if (opts.getBoolean("example")) {
      example();
      exit(0);
   }

   restQ     = opts.getBoolean("rests");
   timesigQ  = opts.getBoolean("timesig");
   keysigQ   = opts.getBoolean("keysig");
   invisibleQ= opts.getBoolean("invisible");
   appendQ   = opts.getBoolean("assemble");
   spineQ    = opts.getBoolean("spines");
   fieldstring = opts.getString("spines");

   if (opts.getBoolean("not-spines")) {
      spineQ = 1;
      fieldstring = opts.getString("not-spines");
      inverseQ = 1;
   }

   prependQ  = opts.getBoolean("prepend");
   if (appendQ) {
      // mutually exclusive options
      prependQ = 0;
   }
   Count     = opts.getInteger("count");
   if (Count < 1) {
      Count = 1;
   } else if (Count > 1000) {
      // don't allow a ridiculously large number
      // of blank spines to be generated.
      Count = 1000;
   }
   PerlRegularExpression pre;
   pre.getTokens(Exinterps, "[\\s,\\*]+", opts.getString("exinterp"));
}
  


//////////////////////////////
//
// example -- example usage of the program
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

//////////////////////////////////////////////////////////////////////
//
// spine field processing functions
//

//////////////////////////////
//
// fillFieldData --
//

void fillFieldData(Array<int>& field, Array<int>& subfield, Array<int>& model,
      const char* fieldstring, HumdrumFile& infile) {

   int maxtrack = infile.getMaxTracks();

   field.setSize(maxtrack);
   field.setGrowth(maxtrack);
   field.setSize(0);
   subfield.setSize(maxtrack);
   subfield.setGrowth(maxtrack);
   subfield.setSize(0);
   model.setSize(maxtrack);
   model.setGrowth(maxtrack);
   model.setSize(0);

   PerlRegularExpression pre;
   Array<char> buffer;
   buffer.setSize(strlen(fieldstring)+1);
   strcpy(buffer.getBase(), fieldstring);
   pre.sar(buffer, "\\s", "", "gs");
   int start = 0;
   int value = 0;
   value = pre.search(buffer.getBase(), "^([^,]+,?)");
   while (value != 0) {
      start += value - 1;
      start += strlen(pre.getSubmatch(1));
      processFieldEntry(field, subfield, model, pre.getSubmatch(), infile);
      value = pre.search(buffer.getBase() + start, "^([^,]+,?)");
   }
}



//////////////////////////////
//
// processFieldEntry -- 
//   3-6 expands to 3 4 5 6
//   $   expands to maximum spine track
//   $-1 expands to maximum spine track minus 1, etc.
//

void processFieldEntry(Array<int>& field, Array<int>& subfield, 
      Array<int>& model, const char* string, HumdrumFile& infile) {

   int maxtrack = infile.getMaxTracks();
   int modletter;
   int subletter;

   PerlRegularExpression pre;
   Array<char> buffer;
   buffer.setSize(strlen(string)+1);
   strcpy(buffer.getBase(), string);

   // remove any comma left at end of input string (or anywhere else)
   pre.sar(buffer, ",", "", "g");

   // first remove $ symbols and replace with the correct values
   removeDollarsFromString(buffer, infile.getMaxTracks());

   int zero = 0;
   if (pre.search(buffer.getBase(), "^(\\d+)-(\\d+)$")) {
      int firstone = strtol(pre.getSubmatch(1), NULL, 10);
      int lastone  = strtol(pre.getSubmatch(2), NULL, 10);

      if ((firstone < 1) && (firstone != 0)) {
         cerr << "Error: range token: \"" << string << "\"" 
              << " contains too small a number at start: " << firstone << endl;
         cerr << "Minimum number allowed is " << 1 << endl;
         exit(1);
      }
      if ((lastone < 1) && (lastone != 0)) {
         cerr << "Error: range token: \"" << string << "\"" 
              << " contains too small a number at end: " << lastone << endl;
         cerr << "Minimum number allowed is " << 1 << endl;
         exit(1);
      }
      if (firstone > maxtrack) {
         cerr << "Error: range token: \"" << string << "\"" 
              << " contains number too large at start: " << firstone << endl;
         cerr << "Maximum number allowed is " << maxtrack << endl;
         exit(1);
      }
      if (lastone > maxtrack) {
         cerr << "Error: range token: \"" << string << "\"" 
              << " contains number too large at end: " << lastone << endl;
         cerr << "Maximum number allowed is " << maxtrack << endl;
         exit(1);
      }

      int i;
      if (firstone > lastone) {
         for (i=firstone; i>=lastone; i--) {
            field.append(i);
            subfield.append(zero);
            model.append(zero);
         }
      } else {
         for (i=firstone; i<=lastone; i++) {
            field.append(i);
            subfield.append(zero);
            model.append(zero);
         }
      }
   } else if (pre.search(buffer.getBase(), "^(\\d+)([a-z]*)")) {
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
         cerr << "Error: range token: \"" << string << "\"" 
              << " contains too small a number at end: " << value << endl;
         cerr << "Minimum number allowed is " << 1 << endl;
         exit(1);
      }
      if (value > maxtrack) {
         cerr << "Error: range token: \"" << string << "\"" 
              << " contains number too large at start: " << value << endl;
         cerr << "Maximum number allowed is " << maxtrack << endl;
         exit(1);
      }
      field.append(value);
      if (value == 0) {
         subfield.append(zero);
         model.append(zero);
      } else {
         subfield.append(subletter);
         model.append(modletter);
      }
   }
}



//////////////////////////////
//
// removeDollarsFromString -- substitute $ sign for maximum track count.
//

void removeDollarsFromString(Array<char>& buffer, int maxtrack) {
   PerlRegularExpression pre;
   char buf2[128] = {0};
   int value2;

   if (pre.search(buffer.getBase(), "\\$$")) {
      sprintf(buf2, "%d", maxtrack);
      pre.sar(buffer, "\\$$", buf2);
   }

   if (pre.search(buffer.getBase(), "\\$(?![\\d-])")) {
      // don't know how this case could happen, however...
      sprintf(buf2, "%d", maxtrack);
      pre.sar(buffer, "\\$(?![\\d-])", buf2, "g");
   }

   if (pre.search(buffer.getBase(), "\\$0")) {
      // replace $0 with maxtrack (used for reverse orderings)
      sprintf(buf2, "%d", maxtrack);
      pre.sar(buffer, "\\$0", buf2, "g");
   }

   while (pre.search(buffer.getBase(), "\\$(-?\\d+)")) {
      value2 = maxtrack - (int)fabs(strtol(pre.getSubmatch(1), NULL, 10));
      sprintf(buf2, "%d", value2);
      pre.sar(buffer, "\\$-?\\d+", buf2);
   }

}





// md5sum: ec5bfed5a848eeff7508e533e478fd5c blank.cpp [20131109]

