//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Fri Aug  7 15:07:31 PDT 2009
// Last Modified: Sat Aug 29 05:42:04 PDT 2009 Added meter/key changes
// Last Modified: Tue Mar 16 03:58:41 PST 2010 Update to MEI 1.9.1b
// Last Modified: Thu Jan  6 09:13:15 PST 2011 Update to MEI 2010-05
// Last Modified: Sun Jul 10 00:20:04 PDT 2011 Avoid language duplicates
// Last Modified: Wed Nov 12 14:40:01 PST 2014 Update for MEI 2013
//
// Filename:      ...sig/examples/all/hum2mei.cpp 
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/hum2mei.cpp
// Syntax:        C++; museinfo
//
// Description:   Converts Humdrum files into MEI data 
//                (monophonic spines only allowed at the moment).
//

#include <math.h>

#ifndef OLDCPP
   #include <iostream>
   #include <sstream>
   #define SSTREAM stringstream
   #define CSTRING str().c_str()
   using namespace std;
#else
   #include <iostream.h>
   #ifdef VISUAL
      #include <strstrea.h>
   #else
      #include <strstream.h>
   #endif
   #define SSTREAM strstream
   #define CSTRING str()
#endif

#include "string.h"

#include "humdrum.h"
#include "PerlRegularExpression.h"


#define BAR_NONE     (0)
#define BAR_SINGLE   (1)
#define BAR_END      (2)
#define BAR_RPTSTART (3)
#define BAR_RPTEND   (4)
#define BAR_RPTBOTH  (5)
#define BAR_INVIS    (6)
#define BAR_DBL      (7)

#define XML_SCOPE    "xml:"

class MeasureInfo {
   public:
           MeasureInfo(void) { clear(); }
      void clear(void) { 
            left     = right = BAR_NONE; 
            num      = -1000; 
            complete = letter = 0;
            ispickup = pickupbefore = 0;
            nextbar  = lastbar = -1;
            valid    = 0;
	    layers.setSize(0);
      };
      int  right;             // visual style of right-side barline of meas.
      int  left;              // visual style of left-side barline of meas.
      int  num;               // measure number
      int  nextbar;           // index of next barline
      int  lastbar;           // index of last barline
      char letter;            // sub measure enumeration
      char complete;          // c = complete, i = underfull, o = overfull
      char pickupbefore;      // true if pickup measure preceeds this 
      char ispickup;          // true if this measure is a pickup measure
      char valid;             // true if index represents a real barline
      Array<char> layers;
};

ostream& operator<<(ostream& out, MeasureInfo& m) {
   out << "valid\t= "  << (int)m.valid << "\n";
   out << "num\t= "    << m.num   << "\tletter = ";
   if (m.letter == 0) {
      out << 0;
   } else {
      out << m.letter;
   } 
   out << "\n";
   out << "last\t= "   << m.lastbar << "\tnext = "   << m.nextbar  << "\n";
   out << "left\t= "   << m.left    << "\tright = "  << m.right    << "\n";
   out << "pickup\t= " << (int)m.ispickup 
       << "\tpickup_before = " << (int)m.pickupbefore << "\n";
   out << "complete= "; 
   if (m.complete == 0) {  
      out << 0;
   } else { 
      out << m.complete;
   }
   out << "\n";
   out << "layers:";
   for (int i=0; i<m.layers.getSize(); i++) {
      out << "\t" << (int)m.layers[i];
   }
   out << endl;
   return out;
}
	  

//////////////////////////////////////////////////////////////////////////

// function declarations:
void      checkOptions         (Options& opts, int argc, char** argv);
void      example              (void);
void      usage                (const char* command);
int       validateHumdurmFile  (HumdrumFile& infile);
void      convertHumdrumToMei  (int indent, SSTREAM& out, HumdrumFile& infile);
void      createMeiHeader      (int indent, SSTREAM& out, HumdrumFile& infile);
void      createMeiMusic       (int indent, SSTREAM& out, HumdrumFile& infile);
void      createMeiMusicBody   (int indent, SSTREAM& out, HumdrumFile& infile);
void      createMeiMusicBodyMdiv(int indent, SSTREAM& out, 
                               HumdrumFile& infile);
void      createMeiMusicBodyMdivScore(int indent, SSTREAM& out, 
                                HumdrumFile& infile);
void      Indent               (SSTREAM& out, int indent, 
                                const char* string = NULL);
void      printHeadFiledesc    (int indent, SSTREAM& out, HumdrumFile& infile);
void      printMeiheadProfiledesc(int indent, SSTREAM& out, 
                                HumdrumFile& infile);
void      printTitleStmtRespStmt(int indent, SSTREAM& out, 
                                HumdrumFile& infile);
void      printFiledescTitleStmt(int indent, SSTREAM& out, 
                                HumdrumFile& infile);
void      printTitleStmtTitle  (int indent, SSTREAM& out, HumdrumFile& infile);
void      printAsCdata         (SSTREAM& out, const char* string);
void      printMdivScoreScoredef(int indent, SSTREAM& out, 
		                HumdrumFile& infile);
void      printInitialKeySignature(SSTREAM& out, HumdrumFile& infile);
void      printInitialMeterSignature(SSTREAM& out, HumdrumFile& infile);
int       getMidiTicksPerQuarterNote(HumdrumFile& infile);
void      printScoredefStaffgrp(int indent, SSTREAM& out, HumdrumFile& infile);
void      getStaffCount        (Array<int>& primarytracks, HumdrumFile& infile);
void      printStaffgrpStaffdef(int indent, SSTREAM& out, HumdrumFile& infile, 
                                Array<int>& tracks, int index);
void      printInitialClef     (SSTREAM& out, HumdrumFile& infile, int ptrack);
void      printBibliographicRecords(int indent, SSTREAM& out, 
                                HumdrumFile& infile);
void      getSectionInfo       (Array<int>& sections, HumdrumFile& infile);
void      printScoreSection    (int indent, SSTREAM& out, HumdrumFile& infile, 
                                Array<int>& sections, int sindex);
void      printSingleSection   (int indent, SSTREAM& out, HumdrumFile& infile,
                                int startline, int stopline);
void      printMeasure         (int indent, SSTREAM& out, HumdrumFile& infile, 
                                Array<int>& measures, int mindex, 
                                int startsection, int stopsection,
				Array<int>& ptrack);
void      getMeasureInfoForSection(Array<int>& measures, HumdrumFile& infile, 
                                int startline, int stopline);
void      generateMeasureInfo  (Array<MeasureInfo>& minfo, HumdrumFile& infile);
void      printBarlineStyle    (SSTREAM& out, int stylecode);
void      calculateLayerInformation(Array<MeasureInfo>& minfo, 
                                HumdrumFile& infile, Array<int>& ptrack);
void      printMeasureStaff    (int indent, SSTREAM& out, HumdrumFile& infile, 
                                Array<MeasureInfo>& minfo, int mindex, 
                                int startsection, int stopsection, 
                                Array<int>& ptrack, int staffindex);
void      printStaffLayer      (int indent, SSTREAM& out, HumdrumFile& infile, 
                                Array<MeasureInfo>& minfo, int mindex, 
                                int startsection, int stopsection, 
                                Array<int>& ptrack, int staffindex, 
                                int layerindex);
int       getLayerCountInMeasure(Array<MeasureInfo>& minfo, int mindex, 
                                int startsection, int stopsection, 
                                int staffindex);
int       getLayerFieldIndex   (int primary, int lindex, int line, 
                                HumdrumFile& infile);
void      addElementToList     (Array<Array<int> >& layeritems, int lindex, 
                                int line, int spine);
void      getLayerItems        (Array<Array<int> >& layeritems, 
                                HumdrumFile& infile, 
                                int staffindex, Array<int>& ptrack, 
                                int layerindex, Array<MeasureInfo>& minfo, 
                                int mindex, int start, int stop);
void      printLayerItems      (int indent, SSTREAM& out, 
		                Array<Array<int> >& layeritems,
                                HumdrumFile& infile, Array<MeasureInfo>& minfo,
			       	int mindex, int start, int stop, 
                                int layerindex);
int       getStartOfData       (HumdrumFile& infile);
int       getBeamAdjustment    (const char* token);
void      printNote            (int indent, SSTREAM& out, const char* string,
                                int i, int j, int k, int layerindex);
void      printRest            (int indent, SSTREAM& out, const char* string,
                                int i, int j, int k, int layerindex);
void      printKernData        (int indent, SSTREAM& out, HumdrumFile& infile, 
                                int row, int col, int layerindex);
void      printTempExpandRules (int indent, SSTREAM& out, HumdrumFile& infile);
void      printDtdAlterations  (ostream& out);
void      initializeGlobalVariables(HumdrumFile& infile);
void      printProfiledescLangusage(int indent, SSTREAM& out, 
                                HumdrumFile& infile, Array<const char*> langs);
int       strcompare           (const void* A, const void* B);
void      printMeiKeySignatureAttributes(SSTREAM& out, const char* keysig);
void      printMeiKeyAttributes(SSTREAM& out, const char* key);
void      printMeiMeterSymbolAttributes   (SSTREAM& out, const char* metstring);
void      printMeiMeterSignatureAttributes(SSTREAM& out, const char* metersig);
void      checkForTimeAndOrKeyChange(int indent, SSTREAM& out, 
                                 HumdrumFile& infile, Array<int>& measures, 
                                 int mindex, Array<int>& ptrack, 
                                 Array<MeasureInfo>& minfo);
void      printPreamble          (ostream& out);
void      addToLANGS             (const char* lang);


// global variables:
int                MIDITPQ = 0;     // used to calculate @dur.ges information
Array<int>         PTRACK;          // used to control the staff number;
Array<MeasureInfo> MINFO;           // used to create <measure> attributes
const char*        IDMARKER = "sc"; // used for @id value generation
int                DATASTART = 0;   // first line of actual data
string             INDENT = "\t";   // indenting string
Array<const char*> LANGS;           // for list of languages used in meiHead
const char*        DTDLOCATION  = "/DTD/mei19b-full.dtd"; 


// User interface variables:
Options options;
int         debugQ    = 0;             // used with --debug option
int         verboseQ  = 0;             // used with -v option
const char* xmlscope = XML_SCOPE;      // used with -X option


//////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
   HumdrumFile infile;

   // initial processing of the command-line options
   checkOptions(options, argc, argv);

   if (options.getArgCount() < 1) {
      infile.read(cin);
   } else {
      infile.read(options.getArg(1));
   }
   infile.analyzeRhythm("4");

   initializeGlobalVariables(infile);

   int validation = validateHumdurmFile(infile);
   if (validation == 0) {
      cerr << "This humdrum file cannot be processed." << endl;
      cerr << "Only files with monophonic spines are allowed." << endl;
      exit(1);
   }

   int indent = 0;
   SSTREAM meidata;

   printPreamble(meidata);
   convertHumdrumToMei(indent, meidata, infile);
   Indent(meidata, indent);

   meidata << ends;
   cout    << meidata.CSTRING;

   return 0;
}


//////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// printPreamble -- print information used by validators
//

void printPreamble(ostream& out) {

   // preamble for MEI 2010-05:
   out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
   out << "<?oxygen SCHSchema=\"http://music-encoding.org/mei/schemata/2010-05/rng/mei-all.rng\"?>\n";
   out << "<?oxygen RNGSchema=\"http://music-encoding.org/mei/schemata/2010-05/rng/mei-all.rng\" type=\"xml\"?>\n";

   // For version 1.9.1b:
   // meidata << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";

   // For earlier versions than MEI 1.9.1b:
   // meidata << "<!DOCTYPE mei SYSTEM \"" << DTDLOCATION << "\"";
   // printDtdAlterations(meidata);
   // meidata << ">\n";
   //
}



//////////////////////////////
//
// initializeGlobalVariables -- variables which are easier to make global
//     than to pass as function parameters...  The are associated with
//     a particular Humdrum file, and need to be reset for each Humdrum file.
//

void initializeGlobalVariables(HumdrumFile& infile) {
   PTRACK.setSize(0);
   getStaffCount(PTRACK, infile);
   DATASTART = getStartOfData(infile);
   LANGS.setSize(0);
}



//////////////////////////////
//
// printDtdAlterations -- print DTD changes.
//

void printDtdAlterations(ostream& out) {

   // Adding measure/@join feature to MEI 1.9b until it is incorporated
   // into the next version of the MEI DTD.
   out << "[\n";
   out << "<!ENTITY % att.anl.measure\n";
   out << "           'corresp           IDREFS                #IMPLIED\n";
   out << "            join              IDREFS                #IMPLIED'>\n";
   out << "]";

}



//////////////////////////////
//
// getStartOfData -- the index of the first line with data on it in the file.
//

int getStartOfData(HumdrumFile& infile) {
   int i;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         return i;
      }
   }
   return i;
}



//////////////////////////////
//
// validateHumdrumFile --  Currently only allowing single-layer
//   staves.
//

int validateHumdurmFile(HumdrumFile& infile) {
   int i, j;
   for (i=0; i<infile.getNumLines(); i++) {
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (infile[i].getSpineInfo(j)[0] == '(') {
            return 0;
         }
      }
   }

   return 1;
}



//////////////////////////////
//
// convertHumdrumToMei --
//

void convertHumdrumToMei(int indent, SSTREAM& out, HumdrumFile& infile) {
   Indent(out, indent);
   
   // MEI version 1.9.1b:
   // out << "<mei version=\"1.9.1b\">\n";

   // MEI version 2010-05:
   out << "<mei xmlns:xlink=\"http://www.w3.org/1999/xlink\"";
   out << " xmlns=\"http://www.music-encoding.org/ns/mei\" meiversion=\"2010-05\">\n";

   createMeiHeader(indent+1, out, infile);
   createMeiMusic(indent+1, out, infile);

   Indent(out, indent);
   out << "</mei>\n";
}



//////////////////////////////
//
// createMeiHeader -- Fill in the <meiHead> section of <mei>
//     <mei/meiHead>
// 
// <meiHead> 	(altmeiid*, filedesc, (encodingdesc?, profiledesc?), 
//               revisiondesc?)
//

void createMeiHeader(int indent, SSTREAM& out, HumdrumFile& infile) {
   Indent(out, indent);
   out << "<meiHead>\n";

   // altmeiid*

   // filedesc
   printHeadFiledesc(indent+1, out, infile);

   // encodingdesc?

   // profiledesc?
   if (LANGS.getSize() > 0) {
      printMeiheadProfiledesc(indent+1, out, infile);
   }

   // revisiondesc?
   

   printBibliographicRecords(indent+1, out, infile);

   Indent(out, indent);
   out << "</meiHead>\n";
}



//////////////////////////////
//
// printMeiheadProfiledesc --
//

void printMeiheadProfiledesc(int indent, SSTREAM& out, HumdrumFile& infile) {
   Indent(out, indent);
   out << "<profiledesc>\n";

   if (LANGS.getSize() > 0) {
      printProfiledescLangusage(indent+1, out, infile, LANGS);
   }

   Indent(out, indent);
   out << "</profiledesc>\n";
}



//////////////////////////////
//
// printMeiheadProfiledesc --
//

void printProfiledescLangusage(int indent, SSTREAM& out, HumdrumFile& infile, 
   Array<const char*> langs) {

   Indent(out, indent);
   out << "<langusage>\n";

   if (langs.getSize() > 1) {
      qsort(langs.getBase(), langs.getSize(), sizeof(const char*), strcompare);
   }

   int i;
   for (i=0; i<langs.getSize(); i++) {
      Indent(out, indent+1);
      out << "<language " << xmlscope << "id=\"" << langs[i] 
          << "\" authority=\"iso639-2\"/>";
      const char* ptr = HumdrumRecord::getLanguageName(langs[i]);
      if (strlen(ptr) > 0) {
         out << "<!-- " << ptr << " -->";
      }
      out << "\n";
   }

   Indent(out, indent);
   out << "</langusage>\n";
}



//////////////////////////////
//
// strcompare -- compare two strings for alphabetical sorting
//

int strcompare(const void* A, const void* B) {
   const char* a = *(const char**)A;
   const char* b = *(const char**)B;
   return strcmp(a, b);
}



//////////////////////////////
//
// printBibliographicRecords --
//
// Version 1.9.1b convert stored raw Humdrum reference records like this:
// <!-- Humdrum Bibliographic Records:
//   <bib key="COM" meaning="Composer's name" value="Bach, Johann Sebastian"/>
// End Humdrum Bibliographic Records -->
//
//  For the MEI 2010-05 converter, a more eleagant method is used using
//  XML processing instructions for a theoretical Humdrum processor:
//  <?Humdrum type="bib" key="COM" value="Bach, Johann Sebastian" 
//      meaning="Composer's name"?>
//

void printBibliographicRecords(int indent, SSTREAM& out, 
      HumdrumFile& infile) {

   char buffer[1024] = {0};
   Array<char> keymeaning(1);
   keymeaning[0] = '\0';
   // int found = 0;
   int i;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isBibliographic()) {
         continue;
      }
      // Used in MEI 1.9.1b conversion:
      // if (found == 0) {
      //    found = 1;
      //    Indent(out, indent);
      //    out << "<!-- Humdrum Bibliographic Records:\n";
      // }
      Indent(out, indent);
      out << "<?Humdrum type=\"bib\"";
      out << " key=\"";
      printAsCdata(out, infile[i].getBibKey(buffer, 1024));
      out << "\"";

      out << " value=\"";
      printAsCdata(out, infile[i].getBibValue(buffer, 1024));
      out << "\"";

      HumdrumRecord::getBibliographicMeaning(keymeaning, 
            infile[i].getBibKey(buffer, 1024));
      if (keymeaning.getSize() > 1) {
         out << " meaning=\"";
         printAsCdata(out, keymeaning.getBase());
         out << "\"";
         //Indent(out, indent+1);
      }

      out << "?>\n";
   }

   // Used in MEI 1.9.1b conversion:
   //if (found) {
   //   Indent(out, indent);
   //   out << "End Humdrum Bibliographic Records -->\n";
   //}
}



//////////////////////////////
//
// printHeadFiledesc -- Fill in the <filedesc> section of <meiHead>
//      <mei/meiHead/filedesc>
// 
// <filedesc> 		(titleStmt, editionStmt?, extent?, fingerprint?, 
//                       pubStmt, seriesStmt?, notesStmt?, sourceDesc?)
//

void printHeadFiledesc(int indent, SSTREAM& out, HumdrumFile& infile) {
   Indent(out, indent);
   out << "<fileDesc>\n";

   printFiledescTitleStmt(indent+1, out, infile);
   // editionStmt?
   // extent?
   // fingerprint?
  
   // pubStmt: just empty for now
   Indent(out, indent+1);
   out << "<pubStmt/>\n";

   // seriesStmt
   // notesStmt
   // sourcedesc

   Indent(out, indent);
   out << "</fileDesc>\n";
}



//////////////////////////////
//
// printFiledescTitleStmt -- Fill in the <titleStmt> section of <filedesc>
//      <mei/meiHead/filedesc/titleStmt>
// 
// <titleStmt> 		(title+, respStmt?)
//
// Container for title and responsibility meta-data. 
//

void printFiledescTitleStmt(int indent, SSTREAM& out, HumdrumFile& infile) {
   Indent(out, indent);
   out << "<titleStmt>\n";

   // title+
   printTitleStmtTitle(indent+1, out, infile);

   // respStmt?
   printTitleStmtRespStmt(indent+1, out, infile);

   Indent(out, indent);
   out << "</titleStmt>\n";
}



//////////////////////////////
//
// printTitleStmtRespStmt --
//      <mei/meiHead/filedesc/titleStmt/respStmt>
//

void printTitleStmtRespStmt(int indent, SSTREAM& out, HumdrumFile& infile) {


}



//////////////////////////////
//
// addToLANGS -- add a language to the LANGS list, avoiding duplicates.
//

void addToLANGS(const char* lang) {
   int foundQ = 0;
   int i;
   for (i=0; i<LANGS.getSize(); i++) {
      if (strcmp(LANGS[i], lang) == 0) {
         foundQ = 1;
         break;
      }
   }
   if (foundQ == 0) {
      LANGS.append(lang);
   }
}



//////////////////////////////
//
// printTitleStmtTitle -- Fill in the <titleStmt> section of <filedesc>
//      <mei/meiHead/filedesc/titleStmt/title>
// 
// <title>		(#PCDATA | extptr | extref | ptr | ref | address |
//              annot | bibl | abbr | expan | name | corpname | persname |
//              repository | periodname | stylename | date | rend | stack |
//              num | title | identifier | fig | lb | pb | choice | handshift |
//              gap | subst | add | corr | damage | del | orig | reg | restore |
//              sic | supplied | unclear)*
//
// Title of a bibliographic entity. The type attribute may be used to
// classify the title according to some convenient typology. Sample values
// include:
// 
//   main         main title 
//   subordinate  subtitle, title of part 
//   abbreviated  abbreviated form of title
//   alternative  alternate title by which the work is also known
//   translated   translated form of title
//   uniform      uniform title
//  
//  The type attribute is provided for convenience in analysing titles
//  and processing them according to their type; where such specialized
//  processing is not necessary, there is no need for such analysis, and
//  the entire title, including subtitles and any parallel titles, may be
//  enclosed within a single <title> element. The level attribute indicates
//  whether this is the title of an article, monograph, journal, series, or
//  unpublished material. Title parts may be encoded in title sub-elements
//  since title is included in model.textphrase.
// 	
//  Attribute	Type		   Value
//  authority  	CDATA 	           #IMPLIED
//  id    	ID                 #IMPLIED
//  key    	NMTOKEN 	   #IMPLIED
//  lang    	IDREF 	           #IMPLIED
//  level    	a | m | j | s | u  #IMPLIED
//  n    	NMTOKEN 	   #IMPLIED
//  reg    	CDATA 	           #IMPLIED
//  subtype    	NMTOKEN 	   #IMPLIED
//  type    	NMTOKEN 	   #IMPLIED
//

void printTitleStmtTitle(int indent, SSTREAM& out, HumdrumFile& infile) {
   int i;
   char keybuffer[128] = {0};
   const char* ptr = "";
   char valuebuffer[2048] = {0};
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isBibliographic()) {
         continue;
      }

      infile[i].getBibKey(keybuffer, 128);
      // deal with !!!XEN: and similar here...
      if (strstr(keybuffer, "OTL") == NULL) {
         continue;
      }

      infile[i].getBibValue(valuebuffer, 2048);

      if (strstr(keybuffer, "@@") != NULL) {
         // original language title
         Indent(out, indent);
         out << "<title type=\"main\"";
	 ptr = infile[i].getBibLangIso639_2();
	 if ((ptr != NULL) && (strlen(ptr) == 3)) {
            // in MEI version 2010-05 with RNG/Schema need "xml:" prefix:
            out << " " << xmlscope << "lang=\"" << ptr << "\"";
            // MEI version 1.9.1b:
            // out << " lang=\"" << ptr << "\"";
            addToLANGS(ptr);
         } 
	 out << ">";
         printAsCdata(out, valuebuffer);
         out << "</title>\n";
      } else if (strchr(keybuffer, '@') != NULL) {
         // translaged title
         Indent(out, indent);
         out << "<title type=\"translated\"";
	 ptr = infile[i].getBibLangIso639_2();
	 if ((ptr != NULL) && (strlen(ptr) == 3)) {
            // in MEI version 2010-05 with RNG/Schema need "xml:" prefix:
            out << " " << xmlscope << "lang=\"" << ptr << "\"";
            // MEI version 1.9.1b:
            // out << " lang=\"" << ptr << "\"";
            addToLANGS(ptr);
         }
         out << ">";
         printAsCdata(out, valuebuffer);
         out << "</title>\n";
      } else {
         // main title
         Indent(out, indent);
         out << "<title type=\"main\">";
         printAsCdata(out, valuebuffer);
         out << "</title>\n";
      }
   }
}



//////////////////////////////
//
// createMeiMusic -- Fill in the <music> section of <mei>
//      <mei/music>
//
// <music> 	(facsimile*, ((front?, (body | group)?, back?)))
//
//

void createMeiMusic(int indent, SSTREAM& out, HumdrumFile& infile) {
   Indent(out, indent);
   out << "<music>\n";

   createMeiMusicBody(indent+1, out, infile);

   Indent(out, indent);
   out << "</music>\n";
}



//////////////////////////////
//
// createMeiMusicBody --
//      <mei/music/body>
//
// <body> 	mdiv+
//

void createMeiMusicBody(int indent, SSTREAM& out, HumdrumFile& infile) {
   Indent(out, indent);
   out << "<body>\n";

   createMeiMusicBodyMdiv(indent+1, out, infile);

   Indent(out, indent);
   out << "</body>\n";
}



//////////////////////////////
//
// createMeiMusicBodyMdiv --
//      <mei/music/body/mdiv>
//
// <mdiv> 	((score?, parts?) | mdiv*)
//

void createMeiMusicBodyMdiv(int indent, SSTREAM& out, HumdrumFile& infile) {
   Indent(out, indent);
   out << "<mdiv>\n";

   generateMeasureInfo(MINFO, infile);
   createMeiMusicBodyMdivScore(indent+1, out, infile);

   Indent(out, indent);
   out << "</mdiv>\n";
}



//////////////////////////////
//
// generateMeterInfo --
//

void generateMeterInfo(Array<double>& measuredur, HumdrumFile& infile) {
   Array<double>& md = measuredur;
   md.setSize(infile.getNumLines());
   md.allowGrowth(0);
   md.setAll(0.0);
   int i, j;
   PerlRegularExpression pre;
   for (i=0; i<infile.getNumLines(); i++) {
      if (i > 0) {
         md[i] = md[i-1];
      }
      if (!infile[i].isInterpretation()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!pre.search(infile[i][j], "^\\*M(\\d+)/(\\d+)$")) {
            continue;
         }
         md[i] = Convert::kernTimeSignatureTop(infile[i][j]) * 
                 Convert::kernTimeSignatureBottomToDuration(infile[i][j]);
         break;
      }
   }

   for (i=md.getSize()-2; i>=0; i--) {
      if (md[i] == 0.0) {
         md[i] = md[i+1];
      }
   }
}



//////////////////////////////
//
// generateMeasureInfo --
//    int  right;             // visual style of right-side barline of meas.
//    int  left;              // visual style of left-side barline of meas.
//    int  num;               // measure number
//    int  nextbar;           // index of next barline
//    int  lastbar;           // index of last barline
//    char letter;            // sub measure enumeration
//    char complete;          // c = complete, i = underfull, o = overfull
//    char pickupbefore;      // true if pickup measure preceeds this 
//    char ispickup;          // true if this measure is a pickup measure
//    char valid;             // true if index represents a real barline
//    Array<char> layers;     // number of layers for each staff on line
//

void generateMeasureInfo(Array<MeasureInfo>& minfo, HumdrumFile& infile) {
   int i;

   Array<double> measuredur;
   generateMeterInfo(measuredur, infile);

   minfo.setSize(infile.getNumLines());
   minfo.allowGrowth(0);
   PerlRegularExpression pre;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isMeasure()) {
         continue;
      }

      // line in file descibes an actual barline
      minfo[i].valid = 1;

      // store the measure number if specified:
      if (pre.search(infile[i][0], "(\\d+)")) {
         minfo[i].num = strtol(pre.getSubmatch(1), NULL, 10);
      }

      // store the style of the left side of the measure
      if (pre.search(infile[i][0], "==")) {
         minfo[i].left = BAR_END;
      }
      if (pre.search(infile[i][0], "^=+(\\d+)?([^=\\d]+)$")) {
         if (strcmp(pre.getSubmatch(2), ":|!|:") == 0) {
            minfo[i].left = BAR_RPTBOTH;
         } else if (strcmp(pre.getSubmatch(), ":||:") == 0) {
            minfo[i].left = BAR_RPTBOTH;
         } else if (strcmp(pre.getSubmatch(), ":!!:") == 0) {
            minfo[i].left = BAR_RPTBOTH;
         } else if (strcmp(pre.getSubmatch(), ":|!") == 0) {
            minfo[i].left = BAR_RPTEND;
         } else if (strcmp(pre.getSubmatch(), "!|:") == 0) {
            minfo[i].left = BAR_RPTSTART;
         } else if (strcmp(pre.getSubmatch(), "||") == 0) {
            minfo[i].left = BAR_DBL;
         } else if (strcmp(pre.getSubmatch(), ":|") == 0) {
            minfo[i].left = BAR_RPTEND;
         } else if (strcmp(pre.getSubmatch(), ":!") == 0) {
            minfo[i].left = BAR_RPTEND;
         } else if (strcmp(pre.getSubmatch(), "|:") == 0) {
            minfo[i].left = BAR_RPTSTART;
         } else if (strcmp(pre.getSubmatch(), "!:") == 0) {
            minfo[i].left = BAR_RPTSTART;
         } else if (strcmp(pre.getSubmatch(), "|") == 0) {
            minfo[i].left = BAR_SINGLE;
         } else if (strcmp(pre.getSubmatch(), "-") == 0) {
            minfo[i].left = BAR_INVIS;
         } else {
         cout << "<!-- UNKNOWN BARLINE STYLE: " << pre.getSubmatch() 
              << " -->\n" << endl;
         }
      } else {
         minfo[i].left = BAR_SINGLE;
      }

      // store the complete marker: 
      //    c = matches time signature
      //    i = incomplete, underfull
      //    o = overfull
      if (fabs(infile[i].getBeat()) == measuredur[i]) {
         minfo[i].complete = 'c';
      } else if (fabs(infile[i].getBeat()) > measuredur[i]) {
         minfo[i].complete = 'o';
      } else if (fabs(infile[i].getBeat()) < measuredur[i]) {
         minfo[i].complete = 'i';
      }

      // store measure number of a pickup beat:
      if (infile[i].getBeat() < 0) {
         minfo[i].pickupbefore = 1;
      }
   }


   // store the right-side barline style
   int tempnext = -1;
   for (i=minfo.getSize()-1; i>=0; i--) {
      minfo[i].nextbar = tempnext;
      if (minfo[i].valid) {
         if (tempnext >= 0) {
            minfo[i].right = minfo[tempnext].left;
         }
         tempnext = i;
      }
   }
	     
   int templast = -1;
   for (i=0; i<minfo.getSize(); i++) {
      minfo[i].lastbar = templast;
      if (minfo[i].valid) {
         templast = i;
      } 
   }
	     
   // finalize pickup measure information
   for (i=0; i<minfo.getSize(); i++) {
      if (minfo[i].valid) {
         continue;
      }
      if (minfo[i].nextbar >= 0) {
         if (minfo[minfo[i].nextbar].pickupbefore) {
            minfo[i].ispickup = 1;
            minfo[i].left     = BAR_INVIS;
            minfo[i].right    = minfo[minfo[i].nextbar].left;
	    minfo[i].complete = 'i';
	    if (minfo[minfo[i].nextbar].num == 1) {
               minfo[i].num = 0;
            }
         }
      }
   }

   // check for intermediate barlines between two
   // barlines with consecutive numbers
   for (i=0; i<minfo.getSize(); i++) {
      if (minfo[i].valid == 0) {  
         continue;
      }
      if (minfo[i].num >= 0) {
         continue;
      }
      if (minfo[i].nextbar < 0) {
         continue;
      }
      if (minfo[i].lastbar < 0) {
         continue;
      }
      if (minfo[minfo[i].lastbar].num == minfo[minfo[i].nextbar].num - 1) {
         // also should check if the first two bars duration sum
         // to the prevailing meter.
         minfo[i].num = minfo[minfo[i].lastbar].num;
         minfo[i].letter = 'b';
         minfo[minfo[i].lastbar].letter = 'a';
      }
   }

   calculateLayerInformation(minfo, infile, PTRACK);


   if (debugQ) {
      cout << "<!-- MEASURE INFORMATION\n";

      for (i=0; i<minfo.getSize(); i++) {
         cout << "\n\n===  " << i << "  =================================\n";
         cout << minfo[i];
      }
      cout << "-->\n";
   }


}



//////////////////////////////
//
// calculateLayerInformation --  Calculate the number of layers
//    for each staff to be processed for each line of the Humdrum file.
//

void calculateLayerInformation(Array<MeasureInfo>& minfo, 
      HumdrumFile& infile, Array<int>& ptrack) {

   Array<int> reverselookup;
   reverselookup.setSize(infile.getMaxTracks()+10);
   reverselookup.setAll(-1);
   Array<int>& rl = reverselookup;

   int i, j;
   for (i=0; i<ptrack.getSize(); i++) {
      rl[ptrack[i]] = i;
   }

   int tval;
   for (i=0; i<infile.getNumLines(); i++) {
      minfo[i].layers.setSize(ptrack.getSize());
      minfo[i].layers.setAll(0);
      minfo[i].layers.allowGrowth(0);
      if (!(infile[i].isData() || infile[i].isMeasure() ||
            infile[i].isInterpretation() || infile[i].isLocalComment())) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         tval = rl[infile[i].getPrimaryTrack(j)];
         if (tval >= 0) {
            minfo[i].layers[tval]++;
         }
      }
   }
}



//////////////////////////////
//
// createMeiMusicBodyMdivScore --
//      <mei/music/body/mdiv/score>
//
//	<score>		((app | div | pb | sb | scoreDef | staffDef |
//                      staffGrp | annot | curve | line | symbol
//                      | anchoredText | choice | handShift | gap |
//                      subst | add | corr | damage | del | orig | reg |
//                      restore | sic | supplied | unclear)*, ((section
//                      | ending), (app | div | pb | sb | scoreDef |
//                      staffDef | staffGrp | annot | curve | line |
//                      symbol | anchoredText | choice | handShift |
//                      gap | subst | add | corr | damage | del | orig |
//                      reg | restore | sic | supplied | unclear)*)*)
//
//	Full score view of the mdiv. Since the measure element is
//	optional, a score may consist entirely of pagebreaks, each of
//	which points to a page image. Div elements are allowed preceding
//	and following sections of music data in order to accommodate
//	blocks of explanatory text.
//

void createMeiMusicBodyMdivScore(int indent, SSTREAM& out, 
      HumdrumFile& infile) {
   Indent(out, indent);
   out << "<score>\n";

   // <scoreDef>
   printMdivScoreScoredef(indent+1, out, infile);

   // sections
   Array<int> sections;
   getSectionInfo(sections, infile);

   if (sections.getSize() == 1) {
      printSingleSection(indent+1, out, infile, sections[0], 
            infile.getNumLines()-1);
   } else {
      int i;
      for (i=0; i<sections.getSize(); i++) {
         printScoreSection(indent+1, out, infile, sections, i);
      }
   }

   Indent(out, indent);
   out << "</score>\n";
}



//////////////////////////////
//
// printSingleSection -- print a section of music
//

void printSingleSection(int indent, SSTREAM& out, HumdrumFile& infile,
   int startline, int stopline) {

   Array<int> measures;
   getMeasureInfoForSection(measures, infile, startline, stopline);

   int i;
   for (i=0; i<measures.getSize(); i++) {
      printMeasure(indent, out, infile, measures, i, startline, 
            stopline, PTRACK);
   }
}



//////////////////////////////
//
// getMeasuresInfoForSection --
//

void getMeasureInfoForSection(Array<int>& measures, HumdrumFile& infile, 
      int startline, int stopline) {

   measures.setSize(10000);
   measures.setSize(0);
   measures.setGrowth(10000);

   int i;
   int barinsection = 0;
   for (i=startline; i<=stopline; i++) {
      if (infile[i].isData()) {
         break;
      }
      if (infile[i].isMeasure()) {
         barinsection = 1;
         break;
      }
   }

   // int backdata = 0;
   if (barinsection == 0) {
      for (i=startline-1; i>=0; i--) {
         if (infile[i].isData()) {
            // backdata = 1;
         }
         if (infile[i].isMeasure()) {
            barinsection = i - startline;
            break;
         }
      }
   }

   if (barinsection > 0) {
      measures.append(barinsection);
   } else {
      barinsection = startline + barinsection;
      measures.append(barinsection);
   }

   int start = barinsection + 1;
   if (start < startline) {
      start = startline;
   }
   for (i=start; i<=stopline; i++) {
      if (infile[i].isMeasure()) {
         measures.append(i);
      }
   }

   // if there is no data after last measure in section, then don't
   // process it:
   int enddata = 0;
   for (i=measures[measures.getSize()-1]; i<stopline; i++) {
      if (infile[i].isData()) {
         enddata = 1;
         break;
      }
   }
   if (enddata == 0) {
      measures.setSize(measures.getSize()-1);
   }

}



//////////////////////////////
//
// checkForTimeAndOrKeyChange -- print any changes in the meter or key
//    signatures when it does not come at the start of the data.  Look
//    in the neighborhood of non-data lines to find parallel changes
//    in the key signature, time signature, and meter symbol (e.g.
//    cut time).  This function should not be called inside a
//    <measure> element, but after one is finished and before another
//    is started.
//

void checkForTimeAndOrKeyChange(int indent, SSTREAM& out, HumdrumFile& infile, 
      Array<int>& measures, int mindex, Array<int>& ptrack, 
      Array<MeasureInfo>& minfo) {

   if (minfo[measures[mindex]].valid == 0) {
      // measure does not start data section (only allows at start of music)
      // so don't try to print anything.  Initial signatures are handled
      // elsewhere.
      return;
   }

   int keysigline    = -1;
   int meterline     = -1;
   int metline       = -1;
   int keysigspine   = -1;
   int meterspine    = -1;
   int metspine      = -1;
   // not printing key information for now.

   PerlRegularExpression pre;

   int i, j;
   for (i=measures[mindex]; i<minfo[measures[mindex]].nextbar; i++) {
      if (infile[i].isData()) {
         break;
      }
      if (!infile[i].isInterpretation()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (infile[i].isExInterp(j, "**kern")) {
            if (pre.search(infile[i][j], "^\\*M\\d+/\\d+$")) {
               meterline  = i;
               meterspine = j;
            } else if (pre.search(infile[i][j], "^\\*met\\(.*\\)$")) {
               metline  = i;
               metspine = j;
            } else if (pre.search(infile[i][j], "^\\*k\\[.*\\]$")) {
               keysigline  = i;
               keysigspine = j;
            }
         }
      }
   }

   if ((keysigline == -1) && (meterline == -1) && (metline == -1)) {
      // nothing to do
      return;
   }

   Indent(out, indent);
   out << "<scoreDef";

   if (keysigline >= 0) {
      printMeiKeySignatureAttributes(out, infile[keysigline][keysigspine]);
   }
   if (meterline >= 0) {
      printMeiMeterSignatureAttributes(out, infile[meterline][meterspine]);
   }
   if (metline >= 0) {
      printMeiMeterSymbolAttributes(out, infile[metline][metspine]);
   }

   out << "/>\n";
}



//////////////////////////////
//
// printMeasure --
//    <measure>
//

void printMeasure(int indent, SSTREAM& out, HumdrumFile& infile, 
      Array<int>& measures, int mindex, int startsection, int stopsection,
      Array<int>& ptrack) {

   checkForTimeAndOrKeyChange(indent, out, infile, measures, mindex, 
      ptrack, MINFO);
		    

   Indent(out, indent);
   out << "<measure";

   // measure/@n:
   char letter = MINFO[measures[mindex]].letter;

   if (MINFO[measures[mindex]].num >= 0) {
      out << " n=\"";
      out << MINFO[measures[mindex]].num;
      if (letter != 0) {
         if (letter > 'a') {
            out << letter;
         }
      }
      out << "\"";
   }

   // measure/@id:
   out << " " << xmlscope << "id=\"";
   out << "m";
   // mark an "x" if the line in the file being used as a barline
   // does not contain a barline.
   if (MINFO[measures[mindex]].valid == 0) {
      out << "x";
   }
   out << "_" << IDMARKER << "_" << measures[mindex];
   out << "\"";


   if (MINFO[measures[mindex]].left != BAR_NONE) {
      if (verboseQ || (MINFO[measures[mindex]].lastbar == -1)) {
         if (verboseQ || (MINFO[measures[mindex]].left != BAR_SINGLE)) {
            out << " left=\"";
            printBarlineStyle(out, MINFO[measures[mindex]].left);
            out << "\"";
         }
      }
   }
   if (MINFO[measures[mindex]].right != BAR_NONE) {
      if (verboseQ || (MINFO[measures[mindex]].right != BAR_SINGLE)) {
         out << " right=\"";
         printBarlineStyle(out, MINFO[measures[mindex]].right);
         out << "\"";
      }
   }

   // print mesure/@complete if specified:
   switch (MINFO[measures[mindex]].complete) {
      case 'c':  if (verboseQ) { out << " complete=\"c\""; } break;
      case 'i':  out << " complete=\"i\""; break;
      case 'o':  out << " complete=\"o\""; break;
   }

   if (MINFO[measures[mindex]].num >= 0) {
      if ((letter == 'a') && (MINFO[measures[mindex]].nextbar >= 0)) {
         out << " join=\"" << "m_" << IDMARKER << "_" 
             << MINFO[measures[mindex]].nextbar << "\"";
      } else if ((letter == 'b') && (MINFO[measures[mindex]].lastbar >= 0)) {
         out << " join=\"" << "m_" << IDMARKER << "_" 
             << MINFO[measures[mindex]].lastbar << "\"";
      }
   }
	     
   // All barlines are assumed to be MEI "control" barlines.
   // It is possible to have "non-control" barlines in Humdrum files,
   // but the HumdrumFile class currently does not permit them.
   if (verboseQ) {
      out << " control=\"true\"";
   }
   
   out << ">\n";

   int i;
   for (i=0; i<ptrack.getSize(); i++) {
      printMeasureStaff(indent+1, out, infile, MINFO, measures[mindex], 
         startsection, stopsection, ptrack, i);
   }

   Indent(out, indent);
   out << "</measure>\n";
}



//////////////////////////////
//
// printMeasureStaff --  Print a particular staff for a particular measure.
//     <staff>
//

void printMeasureStaff(int indent, SSTREAM& out, HumdrumFile& infile, 
      Array<MeasureInfo>& minfo, int mindex, int startsection,
         int stopsection, Array<int>& ptrack, int staffindex) {
   Indent(out, indent);
   out << "<staff";
   out << " n=\"";
   out << staffindex + 1;
   out << "\"";
   out << ">\n";


   int layercount = getLayerCountInMeasure(minfo, mindex, startsection, 
         stopsection, staffindex);

   int i;
   for (i=0; i<layercount; i++) {
      printStaffLayer(indent+1, out, infile, minfo, mindex, startsection,
            stopsection, ptrack, staffindex, i);
   }

   Indent(out, indent);
   out << "</staff>\n";
}



//////////////////////////////
//
// getLayerCountInMeasure -- return the maximum number of layers in the
//

int getLayerCountInMeasure(Array<MeasureInfo>& minfo, int mindex, 
      int startsection, int stopsection, int staffindex) {

   int maxcount = 0;
   int i;

   int start;
   if (minfo[mindex].valid) {
      start = mindex + 1;
   } else {
      start = mindex;
   }
   for (i=start; i<stopsection; i++) {
      if (minfo[i].valid) {
         break;
      }
      if (minfo[i].layers[staffindex] > maxcount) {
         maxcount = minfo[i].layers[staffindex];
      }
   }

   return maxcount;
}



//////////////////////////////
//
// printStaffLayer --
//

void printStaffLayer(int indent, SSTREAM& out, HumdrumFile& infile, 
      Array<MeasureInfo>& minfo, int mindex, int startsection, int stopsection,
      Array<int>& ptrack, int staffindex, int layerindex) {

   Indent(out, indent);
   out << "<layer";
   out << " n=\"";
   out << layerindex + 1;
   out << "\"";
   out << ">\n";

   int start = mindex;
   if (mindex < startsection) {
      start = startsection;
   }
   int stop = minfo[mindex].nextbar;
   if ((stop < 0) || (stop > stopsection)) {
      stop = stopsection;
   }
   if (start < DATASTART) {
      start = DATASTART;
   }
   if (stop < start) {
      out << "<!-- GOT TO FUNNY PLACE IN CONVERTER -->" << endl;
      int tempval = stop;
      stop = start;
      start = tempval;
   }
	     

   Array<Array<int> > layeritems;
   getLayerItems(layeritems, infile, staffindex, ptrack, layerindex, minfo, 
         mindex, start, stop);

   printLayerItems(indent+1, out, layeritems, infile, minfo, mindex, 
         start, stop, layerindex);

   Indent(out, indent);
   out << "</layer>\n";
}



//////////////////////////////
//
// printLayerItems --
//

void printLayerItems(int indent, SSTREAM& out, Array<Array<int> >& layeritems,
      HumdrumFile& infile, Array<MeasureInfo>& minfo, int mindex, int start, 
      int stop, int layerindex) {

   int bindent = 0;

   int oldbeamstate = 0;
   int newbeamstate = 0;
   // int oldgracebeamstate = 0;
   int newgracebeamstate = 0;

   int graceQ = 0;               // true if a grace note rhythm
   int lastgraceQ = 0;
 
   PerlRegularExpression pre;

   int li;
   int i, j;
   for (li=0; li<layeritems.getSize(); li++) {
      i = layeritems[li][0];
      j = layeritems[li][1];
      if (debugQ) {
         Indent(out, indent);
         out << "<!-- (" << i << "," << j << ")\t=\t" << infile[i][j] 
             << "\t-->\n";
      }

      switch (infile[i].getType()) {
         case E_humrec_data:
            // deal with <beam> containers
            lastgraceQ = graceQ;
            graceQ = pre.search(infile[i][j], "Q", "i");
            if ((graceQ == 0) && (lastgraceQ != 0) 
                  && (newgracebeamstate != 0)) {
               // turn of a hanging gracenote beam
               Indent(out, indent + --bindent);
               out << "</beam>\n";
            }

            if (graceQ) {
               // grace note or rest
               // oldgracebeamstate = newgracebeamstate;
               newgracebeamstate += getBeamAdjustment(infile[i][j]);
               if (newgracebeamstate < 0) {
                  newgracebeamstate = 0;
               }
               if ((newbeamstate > 0) && (oldbeamstate == 0)) {
                  Indent(out, indent + bindent++);
                  out << "<beam>\n";
               }
               printKernData(indent + bindent, out, infile, i, j, layerindex);
               if ((newbeamstate == 0) && (oldbeamstate > 0)) {
                  Indent(out, indent + --bindent);
                  out << "<beam>\n";
               }

            } else {
               // regular note or rest
               oldbeamstate = newbeamstate;
               newbeamstate += getBeamAdjustment(infile[i][j]);
               if (newbeamstate < 0) {
                  newbeamstate = 0;
               }
               if ((newbeamstate > 0) && (oldbeamstate == 0)) {
                  Indent(out, indent + bindent++);
                  out << "<beam>\n";
               }
               printKernData(indent + bindent, out, infile, i, j, layerindex);
               if ((newbeamstate == 0) && (oldbeamstate > 0)) {
                  Indent(out, indent + --bindent);
                  out << "</beam>\n";
               }
            }

            break;
         case E_humrec_interpretation:
            if (strstr(infile[i][j], "*clef") != NULL) {
               out << "<!-- CLEF -->\n";
            } else if (strstr(infile[i][j], "*k[") != NULL) {
               // out << "<!-- Key Signature -->\n";
            } else if (pre.search(infile[i][j], "^\\*M\\d+/\\d+")) {
               out << "<!-- Time Signature -->\n";
            }
            break;
         case E_humrec_global_comment:
            // ignore global comment
            break;
         default:
            Indent(out, indent);
            out << "<!-- UNKNOWN DATALINE TYPE: " 
                << infile[i].getType() << "  -->\n";
      }


   }

   if (newbeamstate != 0) {
      Indent(out, indent);
      out << "<!-- Error in beaming: not cross-staff beams allowed -->\n";
      Indent(out, --indent);
      out << "</beam>\n";
   }
   if (newgracebeamstate != 0) {
      Indent(out, indent);
      out << "<!-- Error in beaming: not cross-staff grace-note beams allowed -->\n";
      Indent(out, --indent);
      out << "</beam>\n";
   }
}



//////////////////////////////
//
// printKernData -- print a note, a chord, or a rest
//

void printKernData(int indent, SSTREAM& out, HumdrumFile& infile, int row,
      int col, int layerindex) {

   int k;
   int subcount;
   char buffer[128] = {0};
   if (strchr(infile[row][col], 'r') != NULL) {
      printRest(indent, out, infile[row][col], row, col, -1, layerindex);
   } else if (strchr(infile[row][col], ' ') != NULL) {
      Indent(out, indent++);
      out << "<chord>\n";
      subcount = infile[row].getTokenCount(col);
      for (k=0; k<subcount; k++) {
         infile[row].getToken(buffer, col, k);
         printNote(indent, out, buffer, row, col, k, layerindex);
      }
      Indent(out, --indent);
      out << "<chord>\n";
   } else {
      printNote(indent, out, infile[row][col], row, col, -1, layerindex);
   }
}



//////////////////////////////
//
// printRest -- convert a Humdrum **kern rest in to an MEI rest.
//

void printRest(int indent, SSTREAM& out, const char* string, int i, int j, 
      int k, int layerindex) {
   if (string == NULL) {
      return;
   }

   if (debugQ) {
      Indent(out, indent);
      out << "<!-- REST: " << string << " " << xmlscope << "id=\"n_" 
          << IDMARKER << "_";
      out << i << "_" << j;
      if (k >= 0) {
         out << "_" << k;
      }
      out << "\"" << " -->\n";
   }

   Indent(out, indent);
   out << "<rest";

   // print id marker for rest
   out << " " << xmlscope << "id=\"n_" << IDMARKER << "_" << i << "_" << j;
   if (k >= 0) {
      out << "_" << k;
   }
   out << "\"";
   

   // print rhythm information /////////////////////////////////////////

   PerlRegularExpression pre;

   if (pre.search(string, "Q", "i")) {
      // deal with grace note identification here
   }

   if (pre.search(string, "(\\d+)")) {
      int durval = strtol(pre.getSubmatch(1), NULL, 10);
      if (strcmp(pre.getSubmatch(), "0") == 0) {
         out << " dur=\"breve\"";
      } else if (strcmp(pre.getSubmatch(), "00") == 0) {
         out << " dur=\"long\"";
      } else if (durval > 0) {
         //int visdur = int(pow(2.0, 
         //      (int(log((double)durval)/log(2.0))+0.00000001)));
         int visdur = int(pow(2.0, 
             int(log((double)durval)/log(2.0)+0.001) ));
         out << " dur=\"" << visdur << "\"";
      }
   }
	     
   if (pre.search(string, "(\\.)")) {
      int dotcount = strlen(pre.getSubmatch(1));
      if ((dotcount >= 0) && (dotcount <= 4)) {
         // allowed range for dots in MEI 1.9b
         out << " dots=\"" << dotcount << "\"";
      } 
   }


   out << "/>\n";
}



//////////////////////////////
//
// printNote -- convert a Humdrum **kern note in to an MEI note.
//

void printNote(int indent, SSTREAM& out, const char* string, int i, int j, 
      int k, int layerindex) {
   if (string == NULL) {
      return;
   }

   SSTREAM accid;
   int edaccidQ = 0;

   if (strcmp(string, ".") == 0) {
      return;
   }

   if (debugQ) {
      Indent(out, indent);
      out << "<!-- NOTE: " << string << " " << xmlscope << "id=\"n_" 
          << IDMARKER << "_";
      out << i << "_" << j;
      if (k >= 0) {
         out << "_" << k;
      }
      out << "\"" << " -->\n";
   }

   Indent(out, indent++);
   out << "<note";

   // print id marker for note
   out << " " << xmlscope << "id=\"n_" << IDMARKER << "_" << i << "_" << j;
   if (k >= 0) {
      out << "_" << k;
   }
   out << "\"";

   PerlRegularExpression pre;
   char pname = 'c';
   if (pre.search(string, "([A-G])", "i")) {
      pname = tolower(pre.getSubmatch(1)[0]);
   } else {
      // kern data can contains rhythms with no pitches, but
      // disallow this case for now
      Indent(out, indent);
      out << "<!-- Error: cannot find diatonic pitch class in: " 
          << string << " -->\n";
      return;
   }

   // print pname (diatonic pitch class) attribute
   out << " pname=\"" << pname << "\"";

   // print accidental attribute:
   if (pre.search(string, "([#n-]+)i")) {
      // print an accid subelement of the <note>:
      if (strcmp(pre.getSubmatch(1), "##") == 0) {
         accid << "<accid accid=\"ss\" func=\"edit\"/>";
      } else if (strcmp(pre.getSubmatch(), "#") == 0) {
         accid << "<accid accid=\"s\" func=\"edit\"/>";
      } else if (strcmp(pre.getSubmatch(), "--") == 0) {
         accid << "<accid accid=\"ff\" func=\"edit\"/>";
      } else if (strcmp(pre.getSubmatch(), "-") == 0) {
         accid << "<accid accid=\"f\" func=\"edit\"/>";
      } else if (strcmp(pre.getSubmatch(), "n") == 0) {
         accid << "<accid accid=\"n\" func=\"edit\"/>";
      } 
      edaccidQ = 1;
   } else if (pre.search(string, "([#n-]+)")) {
      if (strcmp(pre.getSubmatch(1), "##") == 0) {
         out << " accid=\"ss\"";
      } else if (strcmp(pre.getSubmatch(), "#") == 0) {
         out << " accid=\"s\"";
      } else if (strcmp(pre.getSubmatch(), "--") == 0) {
         out << " accid=\"ff\"";
      } else if (strcmp(pre.getSubmatch(), "-") == 0) {
         out << " accid=\"f\"";
      } else if (strcmp(pre.getSubmatch(), "n") == 0) {
         out << " accid=\"n\"";
      } 
   }

   // print octave attribute:
   int octave = Convert::kernToBase40(string) / 40;
   if (octave >= 0) {
      out << " oct=\"" << octave << "\"";
   }

   // print rhythm information /////////////////////////////////////////

   if (pre.search(string, "Q", "i")) {
      // deal with grace note identification here
   }

   if (pre.search(string, "(\\d+)")) {
      int durval = strtol(pre.getSubmatch(1), NULL, 10);
      if (strcmp(pre.getSubmatch(), "0") == 0) {
         out << " dur=\"breve\"";
      } else if (strcmp(pre.getSubmatch(), "00") == 0) {
         out << " dur=\"long\"";
      } else if (durval > 0) {
         int visdur = int(pow(2.0, 
             int(log((double)durval)/log(2.0)+0.001) ));
         out << " dur=\"" << visdur << "\"";
      }
   }
	     
   if (pre.search(string, "(\\.)")) {
      int dotcount = strlen(pre.getSubmatch(1));
      if ((dotcount >= 0) && (dotcount <= 4)) {
         // allowed range for dots in MEI 1.9b
         out << " dots=\"" << dotcount << "\"";
      } 
   }

   ////////////////////////////////////////////////////////////////////
   
   // print tie information
   if (strchr(string, '[') != NULL) {
      out << " tie=\"i\"";
   } else if (strchr(string, '_') != NULL) {
      out << " tie=\"m\"";
   } else if (strchr(string, ']') != NULL) {
      out << " tie=\"t\"";
   }

   // print fermata attribute:
   if (strchr(string, ';') != NULL) {
      if (layerindex == 0) {
         out << " fermata=\"above\"";
      } else {
         out << " fermata=\"below\"";
      }
   }

   int articulationQ = 0;

   // print articulation marks here, such as staccato


   if ((articulationQ == 0) && (edaccidQ == 0)) {
      out << "/>\n";
      indent--;
   } else {
      out << ">\n";
      Indent(out, indent);
      if (edaccidQ) {
         accid << ends;
         out << accid.CSTRING << "\n";
      } else {
         // do  someting with articulation sub elements here.
         out << "\n";
      }
      Indent(out, --indent);
      out << "</note>\n";
   }
}



//////////////////////////////
//
// getBeamAdjustment -- counts the difference in L and J beam start/end 
//    markers.
//

int getBeamAdjustment(const char* token) {
   int output = 0;
   if (token == NULL) {
      return output;
   }
   int i = 0;
   while (token[i] != '\0') {
      switch (token[i]) {
         case 'L': output++; break;
         case 'J': output--; break;
      }
      i++;
   }
   return output;
}



//////////////////////////////
//
// getLayerItems --
//

void getLayerItems(Array<Array<int> >& layeritems, HumdrumFile& infile, 
      int staffindex, Array<int>& ptrack, int layerindex, 
      Array<MeasureInfo>& minfo, int mindex, int start, int stop) {

   layeritems.setSize(1000);
   layeritems.setGrowth(1000);
   layeritems.allowGrowth(1);

   int lindex = 0;
   PerlRegularExpression pre;

   int i, j;
   // int lcounter;
   for (i=start; i<=stop; i++) {
      // lcounter = 0;

      // global comments stored only in first staff, first layer
      if (infile[i].isGlobalComment() && (layerindex == 0)
            && (staffindex == 0)) {
         addElementToList(layeritems, lindex++, i, 0);
         continue;
      }

      if (infile[i].isBibliographic()) {
         // bibliographic records should already have been processed.
         continue;
      }

      // don't search current line for layer information if it has
      // already been determined that there is no layer information
      // on that line (excluding null tokens).
      if (layerindex < minfo[i].layers[staffindex]-1) {
         continue;
      }
      
      // tandem interpretations only stored on first layer of staff
      if (infile[i].isInterpretation() && (layerindex == 0)) {
         j = getLayerFieldIndex(ptrack[staffindex], layerindex, i, infile);
         if (j < 0) {
            continue;
         }
         if (pre.search(infile[i][j], "^\\*clef")) {
            // store a clef change
            addElementToList(layeritems, lindex++, i, j);
         } else if (pre.search(infile[i][j], "^\\*k\\[.*\\]")) {
            // store key change
            addElementToList(layeritems, lindex++, i, j);
         }
         continue;
      }
  
      if (infile[i].isLocalComment()) {
         j = getLayerFieldIndex(ptrack[staffindex], layerindex, i, infile);
         if (j < 0) {
            continue;
         }
         if (!pre.search(infile[i][j], "^!\\s*$")) {
            // store a non-empty local comment for the layer
            addElementToList(layeritems, lindex++, i, j);
         }
      }

      if (infile[i].isData()) {
         j = getLayerFieldIndex(ptrack[staffindex], layerindex, i, infile);
         if (j < 0) {
            continue;
         }
         if (strcmp(infile[i][j], ".") == 0) {
            continue;
         } else {
            addElementToList(layeritems, lindex++, i, j);
         }
      }
   }

   layeritems.setSize(lindex);
   layeritems.allowGrowth(0);
}



//////////////////////////////
//
// getLayerFieldIndex -- return the spine index on the given line
//     in the Humdrum file which matches the primary track and layer index.
//     Returns -1 if the requested track/layer was not found.
//

int getLayerFieldIndex(int primary, int lindex, int line, HumdrumFile& infile) {
   int layercounter = 0;
   int j;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (infile[line].getPrimaryTrack(j) == primary) {
         if (layercounter == lindex) {
            return j;
         }
         layercounter++;
      }
   }

   return -1;
}



//////////////////////////////
//
// addElemetntToList --
//

void addElementToList(Array<Array<int> >& layeritems, int lindex, 
      int line, int spine) {
   layeritems[lindex].setSize(2);
   layeritems[lindex].allowGrowth(0);
   layeritems[lindex][0] = line;
   layeritems[lindex][1] = spine;
}



//////////////////////////////
//
// printBarlineStyle --
// BAR_NONE     
// BAR_SINGLE   single
// BAR_END      end
// BAR_RPTSTART rptstart
// BAR_RPTEND   rptend
// BAR_RPTBOTH  rptboth
// BAR_INVIS    invis
// BAR_DBL      dbl
//

void printBarlineStyle(SSTREAM& out, int stylecode) {
   switch (stylecode) {
      case BAR_SINGLE:   out << "single"    ;break;
      case BAR_END:      out << "end"       ;break;
      case BAR_RPTSTART: out << "rptstart"  ;break;
      case BAR_RPTEND:   out << "rptend"    ;break;
      case BAR_RPTBOTH:  out << "rptboth"   ;break;
      case BAR_INVIS:    out << "invis"     ;break;
      case BAR_DBL:      out << "dbl"       ;break;
   }
}



//////////////////////////////
//
// printScoreSection --
//
// <section>
//   section/@id = name of section
//

void printScoreSection(int indent, SSTREAM& out, HumdrumFile& infile, 
   Array<int>& sections, int sindex) {

   Indent(out, indent);
   out << "<section";

   // check for a name for the section:
   PerlRegularExpression pre;
   if (pre.search(infile[sections[sindex]][0], "^\\*>\\s*([^\\[\\]]+)\\s*$")) {
      out << " " << xmlscope << "id=\"";
      printAsCdata(out, pre.getSubmatch(1));
      out << "\"";
   }

   out << ">\n";

   int startline = sections[sindex];
   int stopline = -1;
   if (sindex >= sections.getSize()-1) {
      stopline = infile.getNumLines()-1;
   } else {
      stopline = sections[sindex+1]-1;
   }
   printSingleSection(indent+1, out, infile, startline, stopline);

   Indent(out, indent);
   out << "</section>\n";
}



//////////////////////////////
//
// getSectionInfo -- identify labeled segments in the music.
//     If no labeled segements, then there is a single segment
//     for all of the music (and that single segment should not
//     be placed inside of a <segment> marker.
//

void getSectionInfo(Array<int>& sections, HumdrumFile& infile) {
   int i;
   int dataline = -1;
   PerlRegularExpression pre;

   sections.setSize(100);
   sections.setSize(0);
   sections.setGrowth(100);

   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         dataline = i;
         if (sections.getSize() == 0) {
            sections.append(dataline);
         }
         continue;
      }
      if (!infile[i].isInterpretation()) {
         continue;
      }
      if (pre.search(infile[i][0], "^\\*>[^\\[\\]]+$")) {
         sections.append(i);
      }
   }
}



//////////////////////////////
//
// printMdivScoreScoredef --
//      <mei/music/body/div/score/scoreDef>
//
// <scoreDef> 	(timeline*, chordTable?, symbolTable?, keysig?, pghead1?, 
//               pghead2?, pgfoot1?, pgfoot2?, (staffGrp? | staffDef?))
//
// Container for score meta-information.
// 
// Score-level encoding strategies for:
// 
//    a) alternating meter sig, e.g. 2/4 3/4 in alternating measures (Read,
//       p. 164-165) and combined meters (Read, p.166-168): explicitly
//       encode meters, make them invisible, display both meter sigs at the
//       start of the section
//    b) compound meter sig, e.g. 2+3+2/4 (Read, p. 168-170): set
//       meter.count=2+3+2
//    c) polymeters, e.g. different simultaneous meters (Read, p. 170-173):
//       1. where barlines coincide, use beaming to elucidate the polymeter
//       2. where barlines sometimes coincide, break into measures
//       according to a common unit of time, draw barlines where visually
//       required
//       3. where barlines never coincide, encode as parts only
//    d) mixed meter sig, e.g. 2/4 + 3/16 in the same measure (Read, p.
//       173-174): encode in common time base, e.g. 11/16, make meter
//       invisible, display both meter sigs at the start of the measure
//    e) fractional meter sig, e.g. 3.5/4 (Read, p. 175-177):
//       set meter.count=3.5
//       The beat count may be displayed as a fraction or as its decimal
//       equivalent.
// 
// 
//  Attribute	        Type	                    Value
//  beam.group    	CDATA 	                    #IMPLIED
//  beam.rests    	true|false 	            #IMPLIED
//  clef.line    	CDATA 	                    #IMPLIED
//  clef.shape    	G|GG|F|C|perc|TAB           #IMPLIED
//  clef.trans    	8va|8vb|15va 	            #IMPLIED
//  dur.default    	long|breve|1|2|4|8|16|32|64|128|256|512|1024|2048
//                      |maxima|longa|brevis|semibrevis|minima|semiminima|
//                      fusa|semifusa 	            #IMPLIED
//  id    	        ID 	                    #IMPLIED
//  key.accid    	s|f|ss|ff|n 	            #IMPLIED
//  key.mode    	major|minor|dorian|phrygian|lydian|
//                      mixolydian|aeolian|locrian  #IMPLIED
//  key.pname    	a|b|c|d|e|f|g 	            #IMPLIED
//  key.sig.mixed    	CDATA 	                    #IMPLIED
//  key.sig    	        7f|6f|5f|4f|3f|2f|1f|0|1s|2s|3s|4s|5s|6s|7s|mixed   
//                                                  #IMPLIED
//  meter.count    	CDATA 	                    #IMPLIED
//  meter.sym    	common|cut                  #IMPLIED
//  meter.unit    	CDATA 	                    #IMPLIED
//  modusmaior    	[2-3] 	                    #IMPLIED
//  modusminor    	[2-3] 	                    #IMPLIED
//  n    	        NMTOKEN                     #IMPLIED
//  num    	        CDATA 	                    #IMPLIED
//  numbase            	CDATA 	                    #IMPLIED
//  octave.default    	[0-9] 	                    #IMPLIED
//  prolatio    	[2-3] 	                    #IMPLIED
//  proport.num    	CDATA 	                    #IMPLIED
//  proport.numbase    	CDATA 	                    #IMPLIED
//  source            	IDREFS 	                    #IMPLIED
//  tempus    	        [2-3] 	                    #IMPLIED
//  trans.diat    	CDATA 	                    #IMPLIED
//  trans.semi    	CDATA 	                    #IMPLIED
//  

void printMdivScoreScoredef(int indent, SSTREAM& out, HumdrumFile& infile) {
   Indent(out, indent);
   out << "<scoreDef";

   // @key.sig
   printInitialKeySignature(out, infile);

   // @meter.count
   // @meter.unit
   // @meter.sym
   printInitialMeterSignature(out, infile);

   // @midi.div
   MIDITPQ = getMidiTicksPerQuarterNote(infile);
   if (verboseQ) {
      out << " midi.div=\"" << MIDITPQ << "\"";
   }
   
   out << ">\n";

   printScoredefStaffgrp(indent+1, out, infile);

   printTempExpandRules(indent+1, out, infile);

   Indent(out, indent);
   out << "</scoreDef>\n";
}



///////////////////////////////
//
// printTempExpandRules -- print section expansion rules
//

void printTempExpandRules(int indent, SSTREAM& out, HumdrumFile& infile) {
   int i;
   PerlRegularExpression pre;
   Array<char> buffer;
   int len;

   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isInterpretation()) {
         continue;
      }
      if (pre.search(infile[i][0], "^\\*>([^\\[]*)\\[([^\\]]+)\\]$")) {
         Indent(out, indent);
         out << "<!-- " << "<secexpan";
         if (strlen(pre.getSubmatch(1)) == 0) {
            out << " repeat=\"true\"";
            out << " label=\"default\"";
         } else if (strcmp(pre.getSubmatch(), "norep") == 0) {
            out << " repeat=\"false\"";
            out << " label=\"norep\"";
         } else {
            out << " label=\"";
            printAsCdata(out, pre.getSubmatch());
            out << "\"";
         }
	 len = strlen(pre.getSubmatch(2));
	 buffer.setSize(len+1);
	 strcpy(buffer.getBase(), pre.getSubmatch());
         pre.sar(buffer, "\\s*,\\s*", " ", "g");
         if (buffer.getSize() > 1) {
            out << " ids=\"";
            printAsCdata(out, buffer.getBase());
            out << "\"";
         }

         out << "/> -->\n";
      }
   }
}



//////////////////////////////
//
// printScoredefStaffgrp -- list of staves found in the music.
//   staffGrp/@barthru    = true if a barline goes through all staves
//   staffGrp/@symbol     = brace
//   staffGrp/@lable.full = string to place to left of first staff on
//       first system.
//

void printScoredefStaffgrp(int indent, SSTREAM& out, HumdrumFile& infile) {

   if (PTRACK.getSize() < 1) {
      return;
   }

   Indent(out, indent);
   out << "<staffGrp";
   out << ">\n";

   int i;
   for (i=0; i<PTRACK.getSize(); i++) {
      printStaffgrpStaffdef(indent+1, out, infile, PTRACK, i);
   }

   Indent(out, indent);
   out << "</staffGrp>\n";
}



//////////////////////////////
//
// getStaffCount -- count the number of **kern spines on
//     the first instance of exclusive interpretations.
// 

void getStaffCount(Array<int>& primarytracks, HumdrumFile& infile) {
   primarytracks.setSize(100);
   primarytracks.setSize(0);
   primarytracks.setGrowth(100);
   int i, j;
   int track;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isInterpretation()) {
         for (j=0; j<infile[i].getFieldCount(); j++) {
            if (strcmp(infile[i][j], "**kern") == 0) {
               track = infile[i].getPrimaryTrack(j);
               primarytracks.append(track);
            }
         }
      }
   }

   // reverse the order of the staves to enumerate from higher instruments
   // to lower instruments.
   Array<int>& pt = primarytracks;
   int tempval;
   for (i=0; i<primarytracks.getSize()/2; i++) {
      tempval = primarytracks[i];
      primarytracks[i] = primarytracks[pt.getSize()-1-i];
      primarytracks[pt.getSize()-1-i] = tempval;
   }
}



//////////////////////////////
//
// printStaffgrpStaffdef --
//

void printStaffgrpStaffdef(int indent, SSTREAM& out, HumdrumFile& infile, 
      Array<int>& tracks, int index) {
   Indent(out, indent);
   out << "<staffDef";

   out << " n=\"" << index+1 << "\"";

   printInitialClef(out, infile, tracks[index]);

   out << "/>\n";
}



//////////////////////////////
//
// printIntialClef -- print the initial clef of the
//     specified primary track spine.
//  clef.line = what staffline the cleff is on
//  clef.shape = G, GG, F, C, perc, TAB
//  clef.trans = 8va, 8vb, 15va
// 
// Types of **kern *clef's:
// G	G clef (as used, for example in the treble staff)
// F	F clef (as used, for example in the bass staff)
// C	C clef (as used, for example in the alto staff)
// X	percussion clef
// -	explicit indication that clef is absent
// 1	first (lowest) line is designated by the pitch of the clef
// 2	second (from bottom) line is designated by the pitch of the clef
// 3	third (from bottom) line is designated by the pitch of the clef
// 4	fourth (from bottom) line is designated by the pitch of the clef
// 5	fifth (highest) line is designated by the pitch of the clef
// v =  8va bassa (played one octave higher)
// ^ =  8va treble (played one octave lower)
// ^^ double octave treble (played two octaves higher)
// vv double octave bass (played two octaves lower)
//

void printInitialClef(SSTREAM& out, HumdrumFile& infile, int ptrack) {
   int i, j;
   const char* clef = NULL;
   PerlRegularExpression pre;

   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         break;
      }
      if (infile[i].isMeasure()) {
         break;
      }
      if (infile[i].isInterpretation()) {
         for (j=0; j<infile[i].getFieldCount(); j++) {
            if (infile[i].getPrimaryTrack(j) == ptrack) {
               if (pre.search(infile[i][j], "^\\*clef(.*)")) {
		  clef = infile[i][j];
                  break;
               }
            }
         }
      }
      if (clef != NULL) {
         break;
      }
   }

   if (clef == NULL) {
      return;
   }
	     
   char buffer[128] = {0};
   strncpy(buffer, pre.getSubmatch(1), 32);


   pre.search(buffer, "([CFGX-])");
   if (strcmp(pre.getSubmatch(1), "-") == 0) {
      // explicitly no clef present.
      return;
   } else if (strcmp(pre.getSubmatch(), "C") == 0) {
      out << " clef.shape=\"C\"";
   } else if (strcmp(pre.getSubmatch(), "F") == 0) {
      out << " clef.shape=\"F\"";
   } else if (strcmp(pre.getSubmatch(), "G") == 0) {
      out << " clef.shape=\"G\"";
   } else if (strcmp(pre.getSubmatch(), "G") == 0) {
      out << " clef.shape=\"perc\"";
   }
   
   if (pre.search(buffer, "(\\d)")) {
      out << " clef.line=\"";
      out << pre.getSubmatch(1);
      out << "\"";
   }

   if (strstr(buffer, "vv") != NULL) {
      // transpose two octaves lower
   } else if (strchr(buffer, 'v') != NULL) {
      // transpose one octave lower
      out << " clef.trans=\"8vb\"";
   } else if (strstr(buffer, "^^") != NULL) {
      // transpose two octaves higher
   } else if (strchr(buffer, '^') != NULL) {
      // transpose one octave higher
      out << " clef.trans=\"8va\"";
   }
   
}



//////////////////////////////
//
// getMidiTicksPerQuarterNote --
//

int getMidiTicksPerQuarterNote(HumdrumFile& infile) {
   int returnval = infile.getMinTimeBase();
   if (returnval % 4 != 0) {
      returnval = returnval * 4;
   }

   if (returnval % 4 != 0) {
      cerr << "Error: timebase is not divisible by 4: " << returnval << endl;
      exit(1);
   }
	
   return returnval/4;
}



//////////////////////////////
//
// printInitialKeySignature --
//
// score/scoreDef/@key.sig, @key.mode
//
// @key.sig:  1f = 1 flat, 0 = no sharp/flat, 2s = two sharps
// @key.mode: major|minor|dorian|phrygian|lydian|mixolydian|aeolian|locrian
//            #IMPLIED
// @key.pname a|b|c|d|e|f|g
// @key.accid s|f|ss|ff|n
//

void printInitialKeySignature(SSTREAM& out, HumdrumFile& infile) {
   int i, j;
   const char* keysig = NULL;
   const char* key    = NULL;

   PerlRegularExpression pre;

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
         if (pre.search(infile[i][j], "^\\*k\\[.*\\]$")) {
            keysig = infile[i][j];
         }
         if (pre.search(infile[i][j], "^\\*[A-G][#-]?:", "i")) {
            key = infile[i][j];
         }
         break;
      }
   }
   
   printMeiKeySignatureAttributes(out, keysig);
   printMeiKeyAttributes(out, key);
}



//////////////////////////////
//
// printMeiKeyAttributes -- prints the key of the music
//   (such as whether the music is in C major, or D minor, etc.
//   This is not the key signature which indicates which notes
//   in the music are to be played with sharps or flats.
//   These attributes are stored in <scoreDef>.
//   Attributes which describe the musical key are:
//      @pname = diatonic pitch name
//      @accid = accidental to apply to pitch name
//      @mode  = mode of key: major, minor, other mode
//

void printMeiKeyAttributes(SSTREAM& out, const char* key) {

   if (key != NULL) {
      PerlRegularExpression pre;
      pre.search(key, "^\\*([A-G])([#-]?):(.*)", "i");
      char pname = pre.getSubmatch(1)[0];
      
      out << " key.pname=\"" << (char)tolower(pname) << "\"";

      char accid = pre.getSubmatch(2)[0];
      switch (accid) {
         case '#':  accid = 's';  break;
         case '-':  accid = 'f';  break;
         default:   accid = 'n';
      }
      out << " key.accid=\"" << accid << "\"";

      char mode[32] = {0};
      if (strlen(pre.getSubmatch(3)) == 3) {
         if (strcmp(pre.getSubmatch(), "mix") == 0) {
            strcpy(mode, "mixolydian");
         } else if (strcmp(pre.getSubmatch(), "dor") == 0) {
            strcpy(mode, "dorian");
         } else if (strcmp(pre.getSubmatch(), "phr") == 0) {
            strcpy(mode, "phrygian");
         } else if (strcmp(pre.getSubmatch(), "lyd") == 0) {
            strcpy(mode, "lydian");
         } else if (strcmp(pre.getSubmatch(), "aeo") == 0) {
            strcpy(mode, "aeolian");
         } else if (strcmp(pre.getSubmatch(), "loc") == 0) {
            strcpy(mode, "locrian");
         } else if (std::isupper(pname)) {
            strcpy(mode, "major");
         } else {
            strcpy(mode, "minor");
         }
      } else {
         if (std::isupper(pname)) {
            strcpy(mode, "major");
         } else {
            strcpy(mode, "minor");
         }
      }
      out << " key.mode=\"" << mode << "\"";
   }
}



//////////////////////////////
//
// printMeiKeySignatureAttributes -- convert **kern signature into
//   and MEI key signature.  Assumes that the order of the pitches 
//   in *k[] are standard and consecutive.
//

void printMeiKeySignatureAttributes(SSTREAM& out, const char* keysig) {
   if (keysig != NULL) {
      int count = 0;
      int len = strlen(keysig);
      int i;
      for (i=0; i<len; i++) {
         if (keysig[i] == '#') {
            count++;
         } else if (keysig[i] == '-') {
            count--;
         }
      }
      out << " key.sig=\"" << abs(count);
      if (count > 0) {
         out << "s";
      } else if (count < 0) {
         out << "f";
      }
      out << "\"";
   }
}



//////////////////////////////
//
// printInitialMeterSignature --
//    @meter.count == numerator of key signature
//    @meter.unit  == demoninator of key signature
//    @meter.sym   == cut time or common time symbol
//

void printInitialMeterSignature(SSTREAM& out, HumdrumFile& infile) {
   int i, j;
   const char* metersig   = NULL;
   const char* metstring  = NULL;

   PerlRegularExpression pre;

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
         if (pre.search(infile[i][j], "^\\*M(\\d+)/(\\d+)$")) {
            metersig = infile[i][j];
         }
         if (pre.search(infile[i][j], "^\\*met\\(.+\\)$", "i")) {
            metstring = infile[i][j];
         }
         break;
      }
   }

   printMeiMeterSignatureAttributes(out, metersig);
   printMeiMeterSymbolAttributes(out, metstring);
}



//////////////////////////////
//
// printMeiMeterSignatureAttributes -- time signature such as 4/4;
//

void printMeiMeterSignatureAttributes(SSTREAM& out, const char* metersig) {
   if (metersig != NULL) {
      PerlRegularExpression pre;
      pre.search(metersig, "^\\*M(\\d+)/(\\d+)$");
      out << " meter.count=\"";
      out << pre.getSubmatch(1);
      out << "\"";

      out << " meter.unit=\"";
      out << pre.getSubmatch(2);
      out << "\"";
   }
}



//////////////////////////////
//
// printMeiMeterSymbolAttributes -- such as "C" for common time.
//

void printMeiMeterSymbolAttributes(SSTREAM& out, const char* metstring) {
   if (metstring != NULL) {
      PerlRegularExpression pre;
      if (pre.search(metstring, "\\*met\\(C\\)", "i")) {
         out << " meter.sym=\"common\"";
      } else if (pre.search(metstring, "\\*met\\(C|\\)", "i")) {
         out << " meter.sym=\"cut\"";
      } 
   }
}



//////////////////////////////
//
// checkOptions -- 
//

void checkOptions(Options& opts, int argc, char** argv) {
   opts.define("v|verbose=b",   "Verbose output of data");
   opts.define("I|indent=s:\t", "Indenting string");
   opts.define("X|noxmlscope=s:\t","Do not prepend xml: scope to XML attributes");

   opts.define("d|debug=b");                    // used for debuging info
   opts.define("author=b");                     // author of program
   opts.define("version=b");                    // compilation info
   opts.define("example=b");                    // example usages
   opts.define("h|help=b");                     // short description
   opts.process(argc, argv);

   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, August 2009" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 7 August 2009" << endl;
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

   debugQ   = opts.getBoolean("debug");
   verboseQ = opts.getBoolean("verbose");
   INDENT   = opts.getString("indent");

   // tab character in option is being eaten (probably by the 
   // option parser, so if the INDENT string is empty, force it
   // to be a tab character.
   if (INDENT.size() == 0) {
      INDENT = "\t";
   }
   if (opts.getBoolean("noxmlscope")) {
      xmlscope = "";
   } 
}



//////////////////////////////
//
// Indent --
//
// default value: string = NULL
//

void Indent(SSTREAM& out, int indent, const char* string) {
   int i;
   int limit = 100;
   const char* istring = string;
   if (istring == NULL) {
      istring = INDENT.c_str();
   }
	    
   if (indent > limit) {
      return;
   }
   for (i=0; i<indent; i++) {
      out << istring;
   }
}



//////////////////////////////
//
// printAsCdata -- print text data with special escaping for XML data.
//

void printAsCdata(SSTREAM& out, const char* string) {
   int i = 0;
   while (string[i] != '\0') {
      switch(string[i]) {
         case '>':   out << "&gt;";     break;
         case '<':   out << "&lt;";     break;
         case '\"':  out << "&quot;";   break;
         // case '\'':  out << "&apos;";   break;
         default:    out << string[i];
      }
      i++;
   }
}



//////////////////////////////
//
// example -- example function calls to the program.
//

void example(void) {


}



//////////////////////////////
//
// usage -- command-line usage description and brief summary
//

void usage(const char* command) {

}



