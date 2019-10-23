//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu Apr 11 11:43:12 PDT 2002
// Last Modified: Fri Jun 12 22:58:34 PDT 2009 Renamed SigCollection class
// Last Modified: Mon Feb 21 08:29:14 PST 2011 Added --match
// Last Modified: Sun Feb 27 15:09:29 PST 2011 Added fixed vocal colors
// Last Modified: Thu Nov 14 12:17:29 PST 2013 Added choice of P3/P6 image 
// Last Modified: Thu Nov 14 14:01:01 PST 2013 Changed P3 to default output
// Last Modified: Wed Aug 20 11:16:46 PDT 2014 Added JSON output
// Last Modified: Mon Feb  2 00:13:08 PST 2015 Fixed due to new comp. restr.
// Last Modified: Tue Aug 29 13:59:05 PDT 2017 Added physical time to JSON output
// Filename:      ...sig/examples/all/proll.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/proll.cpp
// Syntax:        C++; museinfo
//
// Description:   Generate piano roll plots.
//

#include "humdrum.h"
#include "PerlRegularExpression.h"

#include <string.h>
#include <ctype.h>

typedef SigCollection<PixelColor> PixelRow;


// function declarations
void   checkOptions             (Options& opts, int argc, char* argv[]);
void   example                  (void);
void   usage                    (const char* command);
void   generateBackground       (HumdrumFile& infile, int rfactor, 
                                 Array<PixelRow>& picturedata, 
                                 Array<PixelRow>& background);
int    generatePicture          (HumdrumFile& infile, Array<PixelRow>& picture,
                                 int style);
void   printPicture             (Array<PixelRow>& picturedata, 
                                 Array<PixelRow>& background, int rfactor, 
                                 int cfactor, int minp, int maxp, 
                                 HumdrumFile& infile);
void   placeNote                (Array<PixelRow>& picture, int pitch, 
                                 double start, double duration, int min, 
                                 PixelColor& color, double factor, int match);
PixelColor makeColor            (HumdrumFile& infile, int line, int spine, 
                                 int style, Array<int>& rhylev, int track);
void   getMarkChars             (Array<char>& marks, HumdrumFile& infile);
int    isMatch                  (Array<char>& marks, const char* buffer);
const char* getInstrument       (HumdrumFile& infile, int spine);
void   createJsonProll          (HumdrumFile& infile);
void   printJsonNote            (ostream& out, int b40, RationalNumber& duration, 
                                 const char* kern, HumdrumFile& infile, int line, 
                                 int field, int token, Array<double>& tempos,
		                           Array<double>& realtimes);
void   printRationalNumber      (ostream& out, RationalNumber& rat);
void   pi                       (ostream& out, int count);
void   printJsonHeader          (HumdrumFile& infile, int indent, Array<int>& ktracks,
                                 Array<int>& partmin, Array<int>& partmax, Array<double>& tempos,
		                           Array<double>& realtime);
void   printPartNames           (HumdrumFile& infile);
void   printPitch               (ostream& out, int b40, const char* kern);
void   printBarlines            (ostream& out, HumdrumFile& infile, int indent, Array<double>& realtime);
void   printSectionLabel        (ostream& out, HumdrumFile& infile, int line);
void   printMensuration         (ostream& out, HumdrumFile& infile, int index);
double checkForTempo            (HumdrumRecord& record);
void   printTempos              (ostream& out, HumdrumFile& infile, int indent, Array<double>& tempos);
void   calculateRealTimesFromTempos(Array<double>& realtimes, HumdrumFile& infile,
	                              Array<double>& tempos);
void calculateTempos            (HumdrumFile& infile, Array<double>& tempos);

// global variables
Options   options;                   // database for command-line arguments
int       debugQ    = 0;             // used with --debug option
int       markQ     = 0;             // used with --mark option
int       maxwidth  = 3000;          // used with -w option
int       maxheight = 400;           // used with -h option
int       rfactor   = 1;
int       cfactor   = 1;
int       gminpitch = 0;
int       gmaxpitch = 127;
int       maxfactor = 5;
int       measureQ  = 1;              // used with the -M option
int       keyboardQ = 1;              // used with the -K option
int       style     = 'H';            // used with the -s option
int       P3Q       = 1;              // used with -3 option
int       P6Q       = 0;              // used with -6 option
int       jsonQ     = 0;
int       metQ      = 0;              // used with --met option
int       met2Q     = 0;              // used with --met2 option
const char* optionfilename = "";      // used with -f option
const char* keyboardcolor = "151515"; // used with the -k option
const char* bgcolor = "000000";       // used with the -b option

///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
   checkOptions(options, argc, argv);

   Array<PixelRow> picturedata;
   Array<PixelRow> background;
   HumdrumFile infile;

   int numinputs = options.getArgCount();
   if (numinputs > 0) {
      const char* filenameIn  = options.getArg(1).c_str();
      infile.read(filenameIn);
   } else {
      infile.read(cin);
   }

   if (infile.getFilename().size() == 0) {
      infile.setFilename(optionfilename);
   }

   if (jsonQ) {
      createJsonProll(infile);
   } else {
      int rfactor = generatePicture(infile, picturedata, style);
      generateBackground(infile, rfactor, picturedata, background);
      printPicture(picturedata, background, rfactor, cfactor, 
            gminpitch, gmaxpitch, infile);
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// createJsonProll -- Create Proll data for display in 
//

void createJsonProll(HumdrumFile& infile) {
   infile.analyzeRhythm("4");
   Array<int> ktracks;
   infile.getTracksByExInterp(ktracks, "**kern");
   Array<int> rktracks(infile.getMaxTracks()+1);
   rktracks.allowGrowth(0);
   rktracks.setAll(-1);
   int i, j, k;
   for (i=0; i<ktracks.getSize(); i++) {
      rktracks[ktracks[i]] = i;
   }

   int ksize = ktracks.getSize();
   stringstream* staves;
   staves = new stringstream[ksize];

   char buffer[1024] = {0};
   int b40;
   RationalNumber duration;
   int track;
   int tcount;

   Array<Array<char> > partnames(ktracks.getSize());

   Array<int> partmax(ktracks.getSize());
   Array<int> partmin(ktracks.getSize());
   partmax.setAll(-10000);
   partmin.setAll(+10000);

   Array<int> noteinit(ktracks.getSize());
   noteinit.setAll(0);

	Array<double> tempos;
	calculateTempos(infile, tempos);
	Array<double> realtimes;
	calculateRealTimesFromTempos(realtimes, infile, tempos);

   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isData()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }
         if (strcmp(infile[i][j], ".") == 0) {
            continue;
         }
         tcount = infile[i].getTokenCount(j);
         track = infile[i].getPrimaryTrack(j);
         for (k=0; k<tcount; k++) {
            infile[i].getToken(buffer, j, k);
            if (strchr(buffer, 'r') != NULL) {
               continue;
            }
            if (strchr(buffer, ']') != NULL) {
               continue;
            }
            if (strchr(buffer, '_') != NULL) {
               continue;
            }
            if (strcmp(buffer, ".") == 0) {
               continue;
            }
            b40 = Convert::kernToBase40(buffer);
            duration = infile.getTiedDurationR(i, j, k);
            if (noteinit[rktracks[track]] == 0) {
               noteinit[rktracks[track]] = 1;
               pi(staves[rktracks[track]], 4);
               staves[rktracks[track]] << "{\n";
            } else {
               pi(staves[rktracks[track]], 4);
               staves[rktracks[track]] << "},\n";
               pi(staves[rktracks[track]], 4);
               staves[rktracks[track]] << "{\n";
            }
            printJsonNote(staves[rktracks[track]], b40, duration, buffer, 
                  infile, i, j, k, tempos, realtimes);

            if (b40 > partmax[rktracks[track]]) {
               partmax[rktracks[track]] = b40;
            }
            if (b40 < partmin[rktracks[track]]) {
               partmin[rktracks[track]] = b40;
            }
         }
      }
   }

   printJsonHeader(infile, 0, ktracks, partmin, partmax, tempos, realtimes);


   pi(cout, 2);
   cout << "[\n";

   pi(cout, 2);
   cout << "{\n";

   int pindex = 0;
   for (i=ktracks.getSize()-1; i>=0; i--) {
      pi(cout, 3);
      cout << "\"partindex\"\t:\t" << pindex++ << ",\n";

      pi(cout, 3);
      cout << "\"notedata\"\t:\t" << "\n";

      pi(cout, 4);
      cout << "[\n";
     
      cout << staves[i].str();

      pi(cout, 4);
      cout << "}\n";

      pi(cout, 4);
      cout << "]\n";
  
      if (i > 0) {
         pi(cout, 2);
         cout << "},\n";
         pi(cout, 2);
         cout << "{\n";
      } else {
         pi(cout, 2);
         cout << "}\n";
      }
   }

   pi(cout, 2);
   cout << "]\n";

   pi(cout, 0);
   cout << "}\n";

   delete [] staves;
}



//////////////////////////////
//
// calculateRealTimesFromTempos --
//

void calculateRealTimesFromTempos(Array<double>& realtimes, HumdrumFile& infile,
	Array<double>& tempos) {

	realtimes.setSize(infile.getNumLines());
	realtimes.setAll(0.0);

	double current = 0.0;
	for (int i=0; i<infile.getNumLines(); i++) {
		realtimes[i] = current;
		if (tempos[i] == 0.0) {
			cerr << "Warning, tempo is set to 0 for some reason at index " << i << endl;
		}
		current += infile[i].getDuration() * 60 / tempos[i];
	}
}




//////////////////////////////
//
// printPartNames --
//

void printPartNames(HumdrumFile& infile) {
   int i, j;
   Array<Array<char> > names;
   Array<int> ktracks;
   infile.getTracksByExInterp(ktracks, "**kern");
   names.setSize(ktracks.getSize());
   char buffer[1024] = {0};
   for (i=0; i<names.getSize(); i++) {
      sprintf(buffer, "part %ld", names.getSize() - i);
      names[i] = buffer;
   }
   Array<int> rkern;
   rkern.setSize(infile.getMaxTracks()+1);
   rkern.setAll(-1);
   for (i=0; i<ktracks.getSize(); i++) {
      rkern[ktracks[i]] = i;
   }
   int track;


   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         break;
      }
      if (!infile[i].isInterpretation()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }
         if (strncmp(infile[i][j], "*I\"", 3) == 0) {
            track = infile[i].getPrimaryTrack(j);
            names[rkern[track]] = &(infile[i][j][3]);
         }
      }
   }

   cout << "[";
   for (i=names.getSize()-1; i>=0; i--) {
      cout << "\"" << names[i] << "\"";
      if (i > 0) {
         cout << ", ";
      }
   }
   cout << "]";
}



//////////////////////////////
//
// printJsonHeader --
//

void printJsonHeader(HumdrumFile& infile, int indent, Array<int>& ktracks,
      Array<int>& partmin, Array<int>& partmax, Array<double>& tempos, Array<double>& realtimes) {
   pi(cout, indent);
   cout << "{\n";

   pi(cout, indent);
   cout << "\t\"dataformat\"\t:\t\"pianoroll\",\n";

   pi(cout, indent);
   cout << "\t\"version\"\t:\t\"2.0\",\n";

   pi(cout, indent);
   cout << "\t\"creationdate\"\t:\t\"";

   int minpitch = partmin[0];
   int maxpitch = partmax[0];
   int i;
   for (i=1; i<partmin.getSize(); i++) {
      if (minpitch > partmin[i]) {
         minpitch = partmin[i];
      }
      if (maxpitch < partmax[i]) {
         maxpitch = partmax[i];
      }
   }

   struct tm *current;
   time_t now;
   time(&now);
   current = localtime(&now);
   int year = current->tm_year + 1900;
   int month = current->tm_mon + 1;
   int day = current->tm_mday;
   cout << year;
   if (month < 10) {
      cout << "0";
   } 
   cout << month;
   if (day < 10) {
      cout << "0";
   } 
   cout << day;
   cout << "\",\n";

   pi(cout, indent);
   cout << "\t\"filename\"\t:\t\"" << infile.getFilename() << "\",\n";

   pi(cout, indent);
   cout << "\t\"scorelength\"\t:\t";
   RationalNumber value = infile[infile.getNumLines()-1].getAbsBeatR();
   printRationalNumber(cout, value);
   cout << ",\n";

   pi(cout, indent);
   cout << "\t\"scorelengthsec\" :\t";
	cout << realtimes.last();
   cout << ",\n";

   pi(cout, indent);
   cout << "\t\"partcount\"\t:\t";
   cout << ktracks.getSize();
   cout << ",\n";

   pi(cout, indent);
   cout << "\t\"partnames\"\t:\t";
   printPartNames(infile);
   cout << ",\n";

   pi(cout, indent);
   cout << "\t\"minpitch\"\t:\t";
   printPitch(cout, minpitch, "");
   cout << ",\n";

   pi(cout, indent);
   cout << "\t\"maxpitch\"\t:\t";
   printPitch(cout, maxpitch, "");
   cout << ",\n";
   

   pi(cout, indent);
   cout << "\t\"rangemin\"\t:\t";
   cout << "[";
   for (i=partmin.getSize()-1; i>=0; i--) {
      if (abs(partmin[i]) >= 1000) {
         cout << "null";
      } else {
         printPitch(cout, partmin[i], "");
      }
      if (i > 0) {
         cout << ", ";
      }
   }
   cout << "],\n";

   pi(cout, indent);
   cout << "\t\"rangemax\"\t:\t";
   cout << "[";
   for (i=partmax.getSize()-1; i>=0; i--) {
      if (abs(partmax[i]) >= 1000) {
         cout << "null";
      } else {
         printPitch(cout, partmax[i], "");
      }
      if (i > 0) {
         cout << ", ";
      }
   }
   cout << "],\n";

   printBarlines(cout, infile, indent, realtimes);
	
   printTempos(cout, infile, indent, tempos);

   pi(cout, indent);
   cout << "\t\"partdata\"\t:\n";
}


//////////////////////////////
//
// calculateTempos --
//

void calculateTempos(HumdrumFile& infile, Array<double>& tempos) {
	tempos.setSize(infile.getNumLines());
	tempos.setAll(0);
   
   for (int i=0; i<infile.getNumLines(); i++) {
		if (infile[i].isData()) {
			continue;
		}
		double tempo = checkForTempo(infile[i]);
		if (tempo <= 0) {
			continue;
		}
		tempos[i] = tempo;
   }

	if (tempos.getSize() == 0) {
		return;
	}
	double value = tempos[0];

	for (int i=1; i<tempos.getSize(); i++) {
		if (tempos[i] == 0) {
			tempos[i] = value;
		} else {
			value = tempos[i];
		}
	}

	value = tempos[tempos.getSize() - 2];
	for (int i=tempos.getSize()-2; i>=0; i--) {
		if (tempos[i] == 0) {
			tempos[i] = value;
		} else {
			value = tempos[i];
		}
	}

}



//////////////////////////////
//
// printTempos --
//

void printTempos(ostream& out, HumdrumFile& infile, int indent, Array<double>& tempos) {
   pi(out, indent);
   out << "\t\"tempos\"\t:\n";
   
   pi(out, indent);
   out << "\t\t[\n";

   indent++;
   indent++;
   
   int barinit = 0;
   PerlRegularExpression pre;
   RationalNumber timeval;
   RationalNumber measuredur;
	double tempo;

   for (int i=0; i<infile.getNumLines(); i++) {
		if (infile[i].isData()) {
			continue;
		}
		tempo = checkForTempo(infile[i]);
		if (tempo <= 0) {
			continue;
		}

      if (barinit) {
         out << ",\n";
      } else {
         barinit = 1;
      }
   
      pi(out, indent);
      out << "{\"time\":";
      timeval = infile[i].getAbsBeatR();
      printRationalNumber(out, timeval);
      out << ", \"tpq\":" << tempo;
      out << "}";
   }

   out << "\n\t\t],\n";
}



//////////////////////////////
//
// printBarlines --
//

void printBarlines(ostream& out, HumdrumFile& infile, int indent, Array<double>& realtimes) {
   int i;
   pi(out, indent);
   out << "\t\"barlines\"\t:\n";
   
   pi(out, indent);
   out << "\t\t[\n";

   indent++;
   indent++;
   
   int barinit = 0;
   int number;
   int terminal;
   int invisible;
   PerlRegularExpression pre;
   RationalNumber timeval;
   RationalNumber measuredur;

   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isBarline()) {
         continue;
      }
      if (pre.search(infile[i][0], "=(\\d+)")) {
         number = atoi(pre.getSubmatch(1));
      } else {
         number = -1;
      }
      terminal = 0;
      if (strstr(infile[i][0], "||") != NULL) {
         terminal = 1;
      } else if (strstr(infile[i][0], "==") != NULL) {
         terminal = 1;
      }
      invisible = 0;
      if (strchr(infile[i][0], '-') != NULL) {
         invisible = 1;
      }

      if ((invisible) && (number < 0)) {
         continue;
      }


      if (barinit) {
         out << ",\n";
      } else {
         barinit = 1;
      }
   
      pi(out, indent);
      out << "{\"time\":";
      timeval = infile[i].getAbsBeatR();
      printRationalNumber(out, timeval);
      out << ", \"timesec\":" << realtimes[i];
      if (number >= 0) {
         out << ", \"label\":\"" << number << "\"";
      }
      
      if (terminal) {
         out << ", \"terminal\":\"" << "true" << "\"";
      }

      printSectionLabel(out, infile, i);
      printMensuration(out, infile, i);
      out << "}";
   }

   out << "\n\t\t],\n";
}



//////////////////////////////
//
// printMensuration --
//

void printMensuration(ostream& out, HumdrumFile& infile, int index) {
   int i;
   PerlRegularExpression pre;

   for (i=index; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         break;
      }
      if (!infile[i].isGlobalComment()) {
         continue;
      }
      if (pre.search(infile[i][0], 
            "^!!primary-mensuration:\\s*met\\(([^)]+)\\)")) {
         out << ", \"mensuration\":\"" << pre.getSubmatch(1) << "\"";
         return;
      }
   }

   for (i=index; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         break;
      }
      if (!infile[i].isInterpretation()) {
         continue;
      }
      if (pre.search(infile[i][0], "^\\*met\\(([^)]+)\\)")) {
         out << ", \"mensuration\":\"" << pre.getSubmatch(1) << "\"";
         return;
      }
   }

   for (i=index-1; i>=0; i--) {
      if (infile[i].isData()) {
         break;
      }
      if (!infile[i].isGlobalComment()) {
         continue;
      }
      if (pre.search(infile[i][0], 
            "^!!primary-mensuration:\\s*met\\(([^)]+)\\)")) {
         out << ", \"mensuration\":\"" << pre.getSubmatch(1) << "\"";
         return;
      }
   }

   for (i=index-1; i>=0; i--) {
      if (infile[i].isData()) {
         break;
      }
      if (!infile[i].isInterpretation()) {
         continue;
      }
      if (pre.search(infile[i][0], "^\\*met\\(([^)]+)\\)")) {
         out << ", \"mensuration\":\"" << pre.getSubmatch(1) << "\"";
         return;
      }
   }

}



//////////////////////////////
//
// printSectionLabel --
//

void printSectionLabel(ostream& out, HumdrumFile& infile, int line) {
   int i;
   char buffer[1024] = {0};

   for (i=line; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         break;
      }
      if (!infile[i].isBibliographic()) {
         continue;
      }
      if (strcmp(infile[i].getBibKey(buffer, 1000), "OMD") == 0) {
         infile[i].getBibValue(buffer, 1000);
         out << ", \"sectionlabel\":\"" << buffer << "\"";
         return;
      }
   }

   for (i=line-1; i>=0;  i--) {
      if (infile[i].isData()) {
         break;
      }
      if (!infile[i].isBibliographic()) {
         continue;
      }
      if (strcmp(infile[i].getBibKey(buffer, 1000), "OMD") == 0) {
         infile[i].getBibValue(buffer, 1000);
         out << ", \"sectionlabel\":\"" << buffer << "\"";
         return;
      }
   }
}



//////////////////////////////
//
// printJsonNote --
//

void printJsonNote(ostream& out, int b40, RationalNumber& duration, const char* kern, 
      HumdrumFile& infile, int line, int field, int token, Array<double>& tempos,
		Array<double>& realtimes) {
   int indent = 4;

   RationalNumber metpos    = infile[line].getBeatR();
   RationalNumber starttime = infile[line].getAbsBeatR();

//   pi(out, indent); 
//   out << "{\n";

   pi(out, indent); 
   out << "\t\"pitch\"\t\t:\t";
   printPitch(out, b40, kern);
   out << ",\n";

   pi(out, indent); 
   out << "\t\"starttime\"\t:\t";
   printRationalNumber(out, starttime);
   out << ",\n";

   pi(out, indent); 
   out << "\t\"starttimesec\"\t:\t";
   out << realtimes[line];
   out << ",\n";

   pi(out, indent); 
   out << "\t\"duration\"\t:\t";
   printRationalNumber(out, duration);
   out << ",\n";

	// assuming tempo does not change during note 
	// (should mostly be true).
   pi(out, indent); 
   out << "\t\"durationsec\"\t:\t";
	out << duration.getFloat() * 60.0 / tempos[line];
   out << ",\n";

   pi(out, indent); 
   out << "\t\"metpos\"\t:\t";
   printRationalNumber(out, metpos);
   out << "\n";

//   pi(out, indent); 
//   out << "}\n";

}



//////////////////////////////
//
// printPitch --
//

void printPitch(ostream& out, int b40, const char* kern) {
   int b12   = Convert::base40ToMidiNoteNumber(b40);
   int b7    = Convert::base40ToDiatonic(b40);
   int accid = Convert::base40ToAccidental(b40);
   char buffer[32] = {0};

   Convert::base40ToKern(buffer, b40 % 40 + 120);
   
   if ((accid == 0) && (strchr(kern, 'n') == NULL)) {
      accid = -100000;
   }

   out << "{";
   out << "\"name\":\"" << buffer << b40/40 << "\", ";
   out << "\"b7\":"    << b7;
   out << ", \"b12\":" << b12;
//    out << ", \"b40\":" << b40;  // can be calculated from b7 and b12.
   if (accid > -1000) {
      out << ", \"accid\":" << accid;
   }
   out << "}";
}



//////////////////////////////
//
// printRationalNumber --  
//

void printRationalNumber(ostream& out, RationalNumber& rat) {
   double floatpart = rat.getFloat();
   int intpart = (int)floatpart;
   RationalNumber fraction;
   fraction = rat - intpart;
   out << "[" << floatpart;
   if (fraction.getNumerator() != 0) {
      out << ", " << fraction.getNumerator();
      out << ", " << fraction.getDenominator();
   }
   out << "]";
}


//////////////////////////////
//
// pi -- print an indent amount.
//

void pi(ostream& out, int count) {
   for (int i=0; i<count; i++) {
      out << '\t';
   }
}



//////////////////////////////
//
// printPicture --
//

void printPicture(Array<PixelRow>& picturedata, Array<PixelRow>& background,
      int rfactor, int cfactor, int minp, int maxp, HumdrumFile& infile) {


   if (minp > 0) {
      minp--;
   }
   if (maxp < 127) {
      maxp++;
   }
   int i, j;
   int m;
   int width = picturedata[0].getSize();
   int height = (maxp - minp + 1);
   cfactor = (int)(maxheight / height);
   if (cfactor <= 0) {
      cfactor = 1;
   }
   if (cfactor > maxfactor) {
      cfactor = maxfactor;
   }
   PixelColor temp;
   PixelColor black(0,0,0);
   PixelColor backcolor(bgcolor);
   height = cfactor * height;
   if (P3Q) {
      cout << "P3\n" << width << " " << height << "\n255\n";
   } else {
      cout << "P6\n" << width << " " << height << "\n255\n";
   }
   for (i=maxp; i>=minp; i--) {
      for (m=0; m<cfactor; m++) {
         for (j=0; j<picturedata[i].getSize(); j++) {
            if (picturedata[i][j] == backcolor) {
               if (P3Q) {
                  background[i][j].writePpm3(cout);
               } else {
                  background[i][j].writePpm6(cout);
               }
            } else {
               if ((i > 0) && (cfactor > 1) && (m == cfactor-1) && 
                     (picturedata[i-1][j] == picturedata[i][j])) {
                  temp = picturedata[i][j] * 0.667;
                  if (P3Q) {
                     temp.writePpm3(cout);
                  } else {
                     temp.writePpm6(cout);
                  }
               } else {
                  if (P3Q) {
                     picturedata[i][j].writePpm3(cout);
                  } else {
                     picturedata[i][j].writePpm6(cout);
                  }
               }
            }
         }
         if (P3Q) {
            cout << "\n";
         }
      }
   }
}



//////////////////////////////
//
// generatePicture -- create the picture.  Returns the number of
//    pixel repetitions for each row's pixel.
//

int generatePicture(HumdrumFile& infile, Array<PixelRow>& picture, int
      style) {

   Array<char> marks;
   getMarkChars(marks, infile);
   PixelColor matchcolor(255,255,255);

   infile.analyzeRhythm("4");
   int min = infile.getMinTimeBase();
   double totaldur = infile.getTotalDuration();
   
   int columns = (int)(totaldur * min / 4.0 + 0.5) + 5;
   if (columns > 50000) {
      cout << "Error: picture will be too big to generate" << endl;
      exit(1);
   }
   int factor = (int)(maxwidth / columns);
   if (factor <= 0) {
      factor = 1;
   }
   if (factor > maxfactor) {
      factor = maxfactor;
   }

   // set picture to black first.  Black regions will be filled in
   // with the background later.
   picture.setSize(128);
   int i, j, k;
   PixelColor backcolor(bgcolor);
   for (i=0; i<picture.getSize(); i++) {
      picture[i].setSize(columns * factor);
      for (j=0; j<picture[i].getSize(); j++) {
         picture[i][j] = backcolor;
         // picture[i][j].setRed(0);
         // picture[i][j].setGreen(0);
         // picture[i][j].setBlue(0);
      }
   }

   // examine metric levels for metric coloration
   Array<int>rhylev;
   infile.analyzeMetricLevel(rhylev);
   for (i=0; i<rhylev.getSize(); i++) {
      // reverse sign so that long notes are positive.
      rhylev[i] = -rhylev[i];
   }
   
   PixelColor color;
   int minpitch = 128;
   int maxpitch = -1;
   int pitch = 0;
   double duration = 0;
   double start = 0;
   char buffer[1024] = {0};
   for (i=0; i<infile.getNumLines(); i++) {
      if (debugQ) {
         cout << "Processing input line " << i + 1 << '\t' << infile[i] << endl;
      }
      if (infile[i].isData()) {
         start = infile[i].getAbsBeat();
         for (j=0; j<infile[i].getFieldCount(); j++) {
            if (strcmp(infile[i].getExInterp(j), "**kern") != 0) {
               continue;
            }
            // duration = Convert::kernToDuration(infile[i][j]);
            duration = infile.getTiedDuration(i, j);
            color = makeColor(infile, i, j, style, rhylev, 
                  infile[i].getPrimaryTrack(j));
            for (k=0; k<infile[i].getTokenCount(j); k++) {
               infile[i].getToken(buffer, j, k);
               if (strchr(buffer, '_') != NULL) {
                  continue;
               }
               if (strchr(buffer, ']') != NULL) {
                  continue;
               }

               pitch = Convert::kernToMidiNoteNumber(buffer);
               if (pitch < 0) {
                  // ignore rests
                  continue;
               }
               if (pitch < minpitch) {
                  minpitch = pitch;
               }
               if (pitch > maxpitch) {
                  maxpitch = pitch;
               }
               if (isMatch(marks, buffer)) {
                  placeNote(picture, pitch, start, duration, min, 
                        color, factor, 1);
               } else {
                  placeNote(picture, pitch, start, duration, min, 
                        color, factor, 0);
               }
            }
         }
      }

   }

   gmaxpitch = maxpitch;
   gminpitch = minpitch;
   return factor;
}



//////////////////////////////
//
// isMatch -- returns true if the string has a match character in it
//

int isMatch(Array<char>& marks, const char* buffer) {
   int i;
   for (i=0; i<marks.getSize(); i++) {
      if (strchr(buffer, marks[i]) != NULL) {
         return 1;
      }
   }
   return 0;
}



//////////////////////////////
//
// makeColor --
//

PixelColor makeColor(HumdrumFile& infile, int line, int spine, int style,
      Array<int>& rhylev, int track) {
   PixelColor output;
   int trackCount;
   PerlRegularExpression pre;
   const char* instrument = "";

   PixelColor purple     (225, 121, 255);
   PixelColor yellowgreen(150, 200,   0);

   switch (toupper(style)) {
      case 'M':    // color by metric position
         if (rhylev[line] >= 2) {
            output.setColor("red");
         } else if (rhylev[line] == 1) {
            output.setColor("lightorange");
         } else if (rhylev[line] == 0) {
            output.setColor("yellow");
         } else if (rhylev[line] == -1) {
            output.setColor("green");
         } else if (rhylev[line] == -2) {
            output.setColor("blue");
         } else if (rhylev[line] <= -3) {
            output.setColor("violet");
         } else {
            output.setColor("silver");
         }
         break;

      case 'V':    // color spines by voice
         instrument = getInstrument(infile, track);
         if (pre.search(instrument, "Bassus", "i")) {
            output.setColor("red");
         } else if (pre.search(instrument, "Contra", "i")) {
            output.setColor("darkorange");
         } else if (pre.search(instrument, "Tenor", "i")) {
            output.setColor("blue");
         } else if (pre.search(instrument, "Altus", "i")) {
            output = purple;
         } else if (pre.search(instrument, "Superius", "i")) {
            output.setColor("limegreen");
         } else if (pre.search(instrument, "Cantus", "i")) {
            output.setColor("limegreen");
         } else if (pre.search(instrument, "Discantus", "i")) {
            output = yellowgreen;
         } else {
            output.setColor("black");
         }
         break;
         
      case 'H':    // color spines by hue
      default:
         trackCount = infile.getMaxTracks();
         output.setHue(((int)infile[line].getTrack(spine))/(double)trackCount);
   }

   PixelColor bcolor(bgcolor);
   double csum = (bcolor.Red + bcolor.Green + bcolor.Blue)/(255*3);
   if (csum > 0.5) {
      output.Red = output.Red / 2;
      output.Green = output.Green / 2;
      output.Blue = output.Blue / 2;
   }

   return output;
}



//////////////////////////////
//
// getInstrument --
//

const char* getInstrument(HumdrumFile& infile, int track) {
   int i, j;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isInterpretation()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (track != infile[i].getPrimaryTrack(j)) {
            continue;
         }
         if (strncmp(infile[i][j], "*I\"", 3) == 0) {
            return &infile[i][j][3];
         }
      }
   }
   return "";
}



//////////////////////////////
//
// placeNote -- draw a note in the picture area
//

void placeNote(Array<PixelRow>& picture, int pitch, double start, 
      double duration, int min, PixelColor& color, double factor, int match) {
   int startindex = (int)(start * min / 4.0 * factor);
   int endindex = (int)((start + duration) * min / 4.0 * factor) - 1;

   PixelColor zcolor = color;
   if (match) {
      zcolor.Red   = (zcolor.Red   + 4*255)/5;
      zcolor.Green = (zcolor.Green + 4*255)/5;
      zcolor.Blue  = (zcolor.Blue  + 4*255)/5;
   }
   PixelColor black(0,0,0);
   if (startindex-1 >= 0) {
      if (picture[pitch][startindex-1] == color) {
         picture[pitch][startindex-1] *= 0.667;
      }
   }
   for (int i=startindex; i<=endindex; i++) {
      if (picture[pitch][i] == black) {
         picture[pitch][i] = zcolor;
      } else {
         if (match) {
            picture[pitch][i].Red   = (2*zcolor.Red 
                  + picture[pitch][i].Red)/3;
            picture[pitch][i].Green = (2*zcolor.Green 
                  + picture[pitch][i].Green)/3;
            picture[pitch][i].Blue  = (2*zcolor.Blue 
                  + picture[pitch][i].Blue)/3;
         } else {
            picture[pitch][i].Red   = (color.Red 
                  + picture[pitch][i].Red)/2;
            picture[pitch][i].Green = (color.Green 
                  + picture[pitch][i].Green)/2;
            picture[pitch][i].Blue  = (color.Blue 
                  + picture[pitch][i].Blue)/2;
         }
      }
   }
}



//////////////////////////////
//
// generateBackground -- create the picture.
//

void generateBackground(HumdrumFile& infile, int rfactor, 
      Array<PixelRow>& picturedata, Array<PixelRow>& background) {

   background.setSize(picturedata.getSize());
   int i, j;

   PixelColor backcolor(bgcolor);
   for (i=0; i<picturedata.getSize(); i++) {
      background[i].setSize(picturedata[i].getSize());
      for (j=0; j<picturedata[i].getSize(); j++) {
         background[i][j] = backcolor;
      }
   }

   PixelColor whitekeys(keyboardcolor);
   if (keyboardQ) {
      for (i=0; i<background.getSize(); i++) {
         switch (i % 12) {
            case 0: case 2: case 4: case 5: case 7: case 9: case 11:
               for (j=0; j<background[i].getSize(); j++) {
                  background[i][j] = whitekeys;
               }
               break;
         }
      }
   }


   int index;
   int min = infile.getMinTimeBase();
   PixelColor measureColor;
   measureColor.setColor(25, 25, 25);
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isMeasure()) {
         index = (int)(infile[i].getAbsBeat() * min / 4.0 * rfactor);
         for (j=0; j<background.getSize(); j++) {
            background[j][index] = measureColor;
         }
      }
   }

}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("w|width=i:3000",      "maximum width allowable for image");   
   opts.define("h|height=i:400",      "maximum height allowable for image");   
   opts.define("M|no-measure=b",      "do not display measure lines on image");
   opts.define("K|no-keyboard=b",     "do not display keyboard in background");
   opts.define("f|filename=s",        "used to label file with standard input");
   opts.define("k|keyboard=s:151515", "keyboard white keys color");
   opts.define("b|background=s:000000", "background color");
   opts.define("s|style=s:H",         "Coloring style");
   opts.define("mark=b",              "highlight marked/matched notes");
   opts.define("3|p3|P3=b",           "output as P3 (ASCII) Portable anymap");
   opts.define("6|p6|P6=b",           "output as P6 (binary) Portable anymap");
   opts.define("j|json=b",            "output proll data in JSON format");
   opts.define("met=d:232",           "tempo control from metrical symbols");
   opts.define("met2=d:336",          "tempo control from metrical symbols, older era");

   opts.define("debug=b",          "trace input parsing");   
   opts.define("author=b",         "author of the program");   
   opts.define("version=b",        "compilation information"); 
   opts.define("example=b",        "example usage"); 
   opts.define("help=b",           "short description"); 
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, April 2002" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 8 April 2002" << endl;
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

   debugQ         =  opts.getBoolean("debug");
   markQ          =  opts.getBoolean("mark");
   maxwidth       =  opts.getInteger("width");
   maxheight      =  opts.getInteger("height");
   measureQ       = !opts.getBoolean("no-measure");
   keyboardQ      = !opts.getBoolean("no-keyboard");
   keyboardcolor  =  opts.getString("keyboard").c_str();
   style          =  opts.getString("style").c_str()[0];
   jsonQ          =  opts.getBoolean("json");
   optionfilename =  opts.getString("filename").c_str();
   P6Q            =  opts.getBoolean("p6");
   bgcolor        =  opts.getString("background").c_str();
   if (opts.getBoolean("p6")) {
      P3Q = 0;
   } else {
      P3Q = 1;
   }


   metQ = int(opts.getDouble("met")+0.5);
	if (metQ < 40) {
		metQ = 40;
	} else if (metQ > 4000) {
		metQ = 4000;
	}

	if (opts.getBoolean("met2")) {
		met2Q = 0;
	} else {
   	met2Q = int(opts.getDouble("met2")+0.5);
   	metQ = met2Q;
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



//////////////////////////////
//
// getMarkChars --
//

void getMarkChars(Array<char>& marks, HumdrumFile& infile) {
   PerlRegularExpression pre;
   Array<char>& colorchar = marks;

   colorchar.setSize(0);
   char value;
   int i;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isBibliographic()) {
         continue;
      }
      // !!!RDF**kern: N= mark color="#ff0000", root
      if (pre.search(infile[i].getLine(), 
            "^!!!RDF\\*\\*kern:\\s*([^\\s])\\s*=\\s*match", "i") ||
          pre.search(infile[i].getLine(), 
            "^!!!RDF\\*\\*kern:\\s*([^\\s])\\s*=\\s*mark", "i")
         ) {
         value = pre.getSubmatch(1)[0];
         colorchar.append(value);
      }
   }
}



//////////////////////////////
//
// checkForTempo --
//

double checkForTempo(HumdrumRecord& record) {
	int tassoQ = false;
	int timeQ = false;
	double tscaling = 1.0;
	// int metQ = 252;
	// int met2Q = 0;

   if (timeQ) {
      // don't encode tempos if the --time option is set.
      return -1.0;
   }
   int i;
   float tempo = 60.0;
   PerlRegularExpression pre;

   // if (!metQ) {

      for (i=0; i<record.getFieldCount(); i++) {
         if (strncmp(record[i], "*MM", 3) == 0) {
            sscanf(&(record[i][3]), "%f", &tempo);
            // cout << "Found tempo marking: " << record[i] << endl;
            return (double)tempo * tscaling;
         }
      }

   // } else {
   if (tassoQ) {
      // C  = 132 bpm
      // C| = 176 bpm

      char mensuration[1024] = {0};
      if (record.isGlobalComment() && pre.search(record[0],
            "^\\!+primary-mensuration:.*omet\\((.*?)\\)\\s*$")) {
         strcpy(mensuration, pre.getSubmatch(1));
      } else if (record.isInterpretation() && record.equalFieldsQ("**kern")) {
         for (i=0; i<record.getFieldCount(); i++) {
            if (record.isExInterp(i, "**kern")) {
               if (pre.search(record[i], "omet\\((.*?)\\)")) {
                  strcpy(mensuration, pre.getSubmatch(1));
               }
               break;
            }
         }
      }
      if (strcmp(mensuration, "C") == 0) {
         return 132.0;
      } else if (strcmp(mensuration, "C|") == 0) {
         return 176.0;
      }
   } else if (metQ) {

      // mensural tempo scalings
      // O           = 58 bpm
      // O.          = 58 bpm
      // C.          = 58 bpm
      // C           = 58 bpm
      // C|          = 72 bpm
      // O2          = 75 bpm
      // C2          = 75 bpm
      // O|          = 76 bpm
      // C|3, 3, 3/2 = 110 bpm
      // C2/3        = 1.5 * 72 = 108 bpm
      // C3          = 110 bpm
      // O3/2        = 58 * 1.5 = 87 bpm
      // O/3         = 110 bpm
      // C|2, Cr     = 144 bpm (previously 220 bpm but too fast)

      char mensuration[1024] = {0};
      if (record.isGlobalComment() && pre.search(record[0],
            "^\\!+primary-mensuration:.*met\\((.*?)\\)\\s*$")) {
         strcpy(mensuration, pre.getSubmatch(1));
      } else if (record.isInterpretation() && record.equalFieldsQ("**kern")) {
         for (i=0; i<record.getFieldCount(); i++) {
            if (record.isExInterp(i, "**kern")) {
               if (pre.search(record[i], "met\\((.*?)\\)")) {
                  strcpy(mensuration, pre.getSubmatch(1));
               }
               break;
            }
         }
      }

      if (strcmp(mensuration, "O") == 0) {
         return (double)metQ * 1.0;
      } else if (strcmp(mensuration, "C|") == 0) {
         return (double)metQ * 1.25;
      } else if (strcmp(mensuration, "C.") == 0) {
         return (double)metQ * 1.0;
      } else if (strcmp(mensuration, "O.") == 0) {
         return (double)metQ * 1.0;
      } else if (strcmp(mensuration, "C") == 0) {
         if (met2Q) {
            return (double)metQ * 1.25;
         } else {
            return (double)metQ * 1.0;
         }
      } else if (strcmp(mensuration, "O|") == 0) {
			if (met2Q) {
         	return (double)metQ * 2.0;
			} else {
         	return (double)metQ * 1.310448;
			}
      } else if (strcmp(mensuration, "C|3") == 0) {
         return (double)metQ * 1.8965517;
      } else if (strcmp(mensuration, "C3") == 0) {
         return (double)metQ * 1.2413793;
      } else if (strcmp(mensuration, "C2/3") == 0) {
         return (double)metQ * 1.8;
      } else if (strcmp(mensuration, "3") == 0) {
         return (double)metQ * 1.8965517;
      } else if (strcmp(mensuration, "3/2") == 0) {
         return (double)metQ * 1.8965517;
      } else if (strcmp(mensuration, "O/3") == 0) {
         return (double)metQ * 1.8965517;
      } else if (strcmp(mensuration, "O2") == 0) {
         return (double)metQ * 1.25;
      } else if (strcmp(mensuration, "O3/2") == 0) {
         return (double)metQ * 1.5;
      } else if (strcmp(mensuration, "C2") == 0) {
         return (double)metQ * 1.25;
      } else if (strcmp(mensuration, "C|2") == 0) {
         return (double)metQ * 2.48276;
      } else if (strcmp(mensuration, "Cr") == 0) {
         return (double)metQ * 2.48276;
      }
   }

   return -1.0;
}



