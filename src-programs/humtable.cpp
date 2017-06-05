//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Mar  8 17:05:00 PST 2011
// Last Modified: Sun Mar 27 13:36:19 PDT 2011 added label hyperlinks
// Last Modified: Sun Mar 27 17:34:54 PDT 2011 added ref record tooltips
// Last Modified: Thu Jun 26 16:07:42 PDT 2014 generalized CSS class markup
// Filename:      ...sig/examples/all/humtable.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/humtable.cpp
// Syntax:        C++; museinfo
//
// Description:   Print Humdrum file as an HTML table.
//

#include "humdrum.h"
#include "PerlRegularExpression.h"

#include <stdlib.h>
#include <string.h>

#ifndef OLDCPP
   #include <iostream>
   #include <sstream>
#else
   #include <iostream.h>
   #include <strstream.h>
#endif

// function declarations
void     checkOptions         (Options& opts, int argc, char* argv[]);
void     example              (void);
void     usage                (const char* command);
void     createTable          (ostream& out, HumdrumFile& infile);
void     printGlobalComment   (ostream& out, HumdrumFile& infile, int line);
void     printReferenceRecord (ostream& out, HumdrumFile& infile, int line);
void     printFields          (ostream& out, HumdrumFile& infile, int line);
int      getSubTracks         (HumdrumFile& infile, int line, int track);
void     addLabelHyperlinks   (Array<char>& strang);
void     addLabelHyperlinkName(Array<char>& strang);
ostream& printHtmlPageHeader  (ostream& out);
ostream& printHtmlPageFooter  (ostream& out);
ostream& printHtmlPageFooter  (ostream& out);
ostream& printCss             (ostream& out);
ostream& printClassInfo       (ostream& out, HumdrumFile& infile, int line);
void     getMarks             (HumdrumFile& infile, Array<char>& chars, 
                               Array<SigString>& colors);
ostream& printMarkStyle       (ostream& out, HumdrumFile& infile, int line, 
                               int track, Array<char>& chars, 
                               Array<SigString>& colors);
void     markupKernToken      (Array<char>& strang, HumdrumFile& infile, 
                               int line, int field);
void     printHtmlCharacter   (ostream& out, char ch);
void     markNullToken        (Array<char>& strang);

// global variables
Options      options;            // database for command-line arguments
int          fullQ     = 1;      // used with --page option
int          classQ    = 1;      // used with -C option
int          cssQ      = 0;      // used with --css option
int          textareaQ = 0;      // used with --textarea option
int          markQ     = 1;	 // used with -M option
int          kernQ     = 0;      // used with -k option
const char*  prefix    = "hum";  // used with --prefix option

Array<char>      Markchar;
Array<SigString> Markcolor;


///////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[]) {
   checkOptions(options, argc, argv);
   HumdrumFileSet infiles;
   infiles.read(options);

   if (fullQ) {
      printHtmlPageHeader(cout);
   } else if (cssQ) {
      printCss(cout);
      exit(0);
   }

   for (int i=0; i<infiles.getCount(); i++) {
      createTable(cout, infiles[i]);
   }

   if (fullQ) {
      printHtmlPageFooter(cout);
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// printCss -- print an example Cascading Style Sheet.
//

ostream& printCss(ostream& out) {
   out << "." << prefix << "Div        { width:100%; padding:10px; background:#ffffff; align:center; border: 1px solid #e1e4e5; }\n";
   out << "." << prefix << "Table      { padding:0; border-spacing:0; }\n";
   out << "." << prefix << "Subtable   { padding:0; border-spacing:0; }\n";
   out << "." << prefix << "Line:hover { background:#d0d0ff; }\n";
   out << "." << prefix << "Null       { color:#aaaaaa; }\n";
   out << "." << prefix << "KernToken  { color:#aaaaaa; }\n";
   out << "." << prefix << "KernPitch  { color:#000000; }\n";
   out << "." << prefix << "KernRhythm { color:#000000; }\n";
   out << "." << prefix << "Trackpad   { padding-right:30px; }\n";
   out << "." << prefix << "Subtrackpad{ padding-right:20px; }\n";
   out << "." << prefix << "Bar        { color:#555555; background-color:#eeeeee; }\n";
   out << "." << prefix << "Exinterp   { color:magenta; }\n";
   out << "." << prefix << "Manip      { color:#f62217; }\n";
   out << "." << prefix << "Interp     { color:#9f000f; }\n";
   out << "." << prefix << "Ref        { color:#007700; }\n";
   out << "." << prefix << "Gcom       { color:#1100aa; }\n";
   out << "." << prefix << "Lcom       { color:#7d1b7e; }\n";

   return out;
}



//////////////////////////////
//
// printHtmlPageFooter -- 
//

ostream& printHtmlPageFooter(ostream& out) {
   out << "</body></html>\n";
   return out;
}



//////////////////////////////
//
// printHtmlPageHeader --
//

ostream& printHtmlPageHeader(ostream& out) {
   out << "<html>\n<head>\n<title>full page</title>\n";

   if (cssQ) {
      out << "\n<style type=\"text/css\">\n";
      out << "a {text-decoration:none}\n";
      printCss(out);
      out << "</style>\n";
   }
   out << "\n</head>\n<body>\n";
   return out;
}



//////////////////////////////
//
// createTable --
//

void createTable(ostream& out, HumdrumFile& infile) {
   if (markQ) {
      getMarks(infile, Markchar, Markcolor);
   } else {
      Markchar.setSize(0);;
      Markcolor.setSize(0);;
   }

   out << "<div class=\"" << prefix << "Div\">\n";
   out << "<table class=\"" << prefix << "Table\">\n";

   int i;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         printFields(out, infile, i);
         continue;
      }
      if (infile[i].isMeasure()) {
         printFields(out, infile, i);
         continue;
      }
      if (infile[i].isInterpretation()) {
         printFields(out, infile, i);
         continue;
      }
      if (infile[i].isLocalComment()) {
         printFields(out, infile, i);
         continue;
      }
      if (infile[i].isGlobalComment()) {
         printGlobalComment(out, infile, i);
         continue;
      }
      if (infile[i].isBibliographic()) {
         printReferenceRecord(out, infile, i);
         continue;
      }
   }

   out << "</table>\n";
   out << "</div>\n";

   if (textareaQ) {
      out << "<textarea wrap=off rows=10 cols=70>";
      out << infile;
      out << "</textarea>\n";
   }

}



//////////////////////////////
//
// getMarks -- Return a list of mark characters in **kern data.  Only
//     considering **kern markers for the moment.
//

void getMarks(HumdrumFile& infile, Array<char>& chars, 
      Array<SigString>& colors) {
   const char* defaultcolor = "#dd0000";
   const char* color = defaultcolor;
   PerlRegularExpression pre;
   char marker;
   int i;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isReferenceRecord()) {
         continue;
      }
      if (pre.search(infile[i][0], 
            "!!!RDF\\*\\*kern:\\s*([^\\s])\\s*=.*(match|mark).*note")) {
         marker = pre.getSubmatch(1)[0];
         if (pre.search(infile[i][0], "color\\s*=\\s*[\"']([^\"']+)[\"']")) {
            color = pre.getSubmatch(1);
         } else if (pre.search(infile[i][0], "color\\s*=\\s*([^\\s]+)")) {
            color = pre.getSubmatch(1);
         } else {
            color = defaultcolor;
         }
         chars.append(marker);
         colors.increase(1);
         colors.last() = color;
      }
   }
}



//////////////////////////////
//
// getSubTracks --
//

int getSubTracks(HumdrumFile& infile, int line, int track) {
   int& i = line;
   int j;
   int output = 0;
   for (j=0; j<infile[i].getFieldCount(); j++) {
      if (infile[i].getPrimaryTrack(j) == track) {
         output++;
      }
   }

   return output;
}



//////////////////////////////
//
// printClassInfo --
//

ostream& printClassInfo(ostream& out, HumdrumFile& infile, int line) {
   if (infile[line].isBarline()) {
      out << " class=\"" << prefix << "Line " << prefix << "Bar\"";
   } else if (infile[line].isData()) {
      out << " class=\"" << prefix << "Line " << prefix << "Dat\"";
   } else if (infile[line].isInterpretation()) {
      if (strncmp(infile[line][0], "**", 2) == 0) {
         out << " class=\"" << prefix << "Line " << prefix << "Exinterp\"";
      } else if (infile[line].isSpineManipulator()) {
         out << " class=\"" << prefix << "Line " << prefix << "Manip\"";
      } else {
         out << " class=\"" << prefix << "Line " << prefix << "Interp\"";
      }
   } else if (infile[line].isLocalComment()) {
      out << " class=\"" << prefix << "Line " << prefix << "Lcom\"";
   }
   return out;
}



//////////////////////////////
//
// printFields --
//

void printFields(ostream& out, HumdrumFile& infile, int line) {
   out << "<tr valign=\"baseline\"";
   if (classQ) {
      printClassInfo(out, infile, line);
   }

   out << ">";
   int& i = line;
   int j;
   int track;
   int counter;
   int subtracks;
   PerlRegularExpression pre;
   Array<char> strang;

   for (track=1; track<=infile.getMaxTracks(); track++) {
      subtracks = getSubTracks(infile, line, track);
      out << "<td";
      if (markQ && (subtracks == 1)) {
         printMarkStyle(out, infile, line, track, Markchar, Markcolor);
      }
      if (classQ) {
         out << " class=\"";
         if (subtracks == 1) {
            out << " " << prefix << "Tok ";
            out << " " << prefix << "SS";
            out << infile.getTrackExInterp(track).c_str()+2;
         }
         out << " " << prefix << "Tr" << track;
         if (track < infile.getMaxTracks()) {
            out << " " << prefix << "Trackpad";
         }
         out << "\"";
      }
      out << ">";
      if (subtracks > 1) {
         out << "<table width=\"100%\" class=\"" << prefix << "Subtable\">";
         out << "<tr valign=baseline";
         if (classQ) {
            printClassInfo(out, infile, line);
         }
         out << ">";
         counter = 0;
         for (j=0; j<infile[i].getFieldCount(); j++) {
            if (infile[i].getPrimaryTrack(j) == track) {

               if (strcmp(infile[i][j], ".") == 0) {
                  markNullToken(strang);
               } else if (kernQ && infile[i].isData() && 
                   (infile.getTrackExInterp(track) == "**kern")) {
                 markupKernToken(strang, infile, i, j);
               } else {
                  strang.setSize(strlen(infile[i][j])+1);
                  strcpy(strang.getBase(), infile[i][j]); 
                  pre.sar(strang, " ", "&nbsp;", "g");
                  // deal with HTML entities, just < and > for now:
                  pre.sar(strang, ">", "&gt;", "g");
                  pre.sar(strang, "<", "&lt;", "g");
                  if (pre.search(strang, "^\\*>.*\\[", "")) {
                     addLabelHyperlinks(strang);
                  } else if (pre.search(strang, "^\\*>", "")) {
                     addLabelHyperlinkName(strang);
                  }
               }

               out << "<td width=\"" << 100.0/subtracks << "%\"";
               if (classQ) {
                  out << " class=\"";
                  out << " " << prefix << "Tok ";
                  out << " " << prefix << "SS";
                  out << infile.getTrackExInterp(track).c_str()+2;
                  if (counter < subtracks - 1) {
                     out << " " << prefix << "Subtrackpad";
                  }
                  out << "\"";
               }
               if (markQ && (subtracks == 1)) {
                  printMarkStyle(out, infile, line, track, Markchar, Markcolor);
               }
               out << ">" << strang << "</td>";
               counter++;
            }
         }
         out << "</tr></table>";
      } else {
         for (j=0; j<infile[i].getFieldCount(); j++) {
            if (infile[i].getPrimaryTrack(j) == track) {

               if (strcmp(infile[i][j], ".") == 0) {
                  markNullToken(strang);
               } else if (kernQ && infile[i].isData() && 
                   (infile.getTrackExInterp(track) == "**kern")) {
                 markupKernToken(strang, infile, i, j);
               } else {
                  strang.setSize(strlen(infile[i][j])+1);
                  strcpy(strang.getBase(), infile[i][j]); 
                  pre.sar(strang, " ", "&nbsp;", "g");
                  // deal with HTML entities, just < and > for now:
                  pre.sar(strang, ">", "&gt;", "g");
                  pre.sar(strang, "<", "&lt;", "g");
                  if (pre.search(strang, "^\\*>.*\\[", "")) {
                     addLabelHyperlinks(strang);
                  } else if (pre.search(strang, "^\\*>", "")) {
                     addLabelHyperlinkName(strang);
                  }
               }

               out << strang;
            }
         }
      }
      out << "</td>";
   }
   out << "</tr>\n";
}



//////////////////////////////
//
// markNullToken --
//

void markNullToken(Array<char>& strang) {
   stringstream buffer;
   buffer << "<span class=\"" << prefix << "Null\">";
   buffer << ".";
   buffer << "</span>";
   buffer << ends;
   int len = strlen(buffer.str().c_str());
   strang.setSize(len+1);
   strcpy(strang.getBase(), buffer.str().c_str());
}



//////////////////////////////
//
// markupKernToken --
//

void markupKernToken(Array<char>& strang, HumdrumFile& infile, 
      int line, int field) {

   stringstream buffer;
   const char* token = infile[line][field];
   int len = strlen(token);
   Array<int> states(len);
   states.setAll(0);
   char ch;
   int i;
   for (i=0; i<len; i++) {
      if (std::isdigit(token[i])) {
         states[i] = 2;  // part of rhythm
         continue;
      }
      ch = std::tolower(token[i]);
      if ((ch >= 'a') && (ch <= 'g')) {
         states[i] = 1;  // part of pitch
         continue;
      }
      switch (ch) {
         case '#': 
         case '-': 
         case 'n': 
         case 'r': 
            states[i] = 1;   // pitch
            break;
         case '%': 
         case '.': 
            states[i] = 2;   // rhythm
            break;
      }
   }

   int lstate = 0;
   buffer << "<span class=\""  << prefix << "KernToken\">";
   if (strcmp(token, ".") == 0) {
      buffer << ".";
   } else {
      for (i=0; i<len; i++) {
         if (states[i] == lstate) {
            // nothing new to markup.
            printHtmlCharacter(buffer, token[i]);
            continue;
         }
         if ((i > 0) && (lstate > 0)) {
            // turn off old span
            buffer << "</span>";
         }
         if (states[i] == 0) {
            printHtmlCharacter(buffer, token[i]);
            lstate = states[i];
            continue;
         }
         // turn on new span:
         buffer << "<span class=\"";
         switch (states[i]) {
            case 1:    // pitch
               buffer << prefix << "KernPitch";
               break;
            case 2:    // rhythm
               buffer << prefix << "KernRhythm";
               break;
         }
         buffer << "\">";
         printHtmlCharacter(buffer, token[i]);
         lstate = states[i];
      }
   }
   buffer << "</span>";
   buffer << ends;

   int len2 = strlen(buffer.str().c_str());
   strang.setSize(len2+1);
   strcpy(strang.getBase(), buffer.str().c_str());
}



//////////////////////////////
//
// printHtmlCharacter --
//

void printHtmlCharacter(ostream& out, char ch) {
   switch (ch) {
      case '>' : out << "&gt;"; break;
      case '<' : out << "&lt;"; break;
      case '&' : out << "&lt;"; break;
      case ' ' : out << "&nbsp;"; break;
      case '\'': out << "&apos;"; break;
      case '"' : out << "&quot;"; break;
      default  : out << ch;
   }
}
      


//////////////////////////////
//
// printMarkStyle --  Have to do spine split marks as well.
//

ostream& printMarkStyle(ostream& out, HumdrumFile& infile, int line, 
      int track, Array<char>& chars, Array<SigString>& colors) {
   if (!infile[line].isData()) {
      return out;
   }
   int ii, jj;
   int j;
   int k;
   int ttrack;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      ttrack = infile[line].getPrimaryTrack(j);
      if (ttrack != track) {
         continue;
      }
      
      ii = line;
      jj = j;
      if (strcmp(infile[line][j], ".") == 0) {
         ii = infile[line].getDotLine(j);
         jj = infile[line].getDotSpine(j);
      }

      for (k=0; k<chars.getSize(); k++) {
         if (strchr(infile[ii][jj], chars[k]) == NULL) {
            continue;
         }
         // found a mark so set background color of cell
         out << " style=\"background-color:" << colors[k] << "\"";
         return out;
      }
   }

   return out;
}



//////////////////////////////
//
// addLabelHyperlinkName --
//

void addLabelHyperlinkName(Array<char>& strang) {
   PerlRegularExpression pre;
   if (!pre.search(strang, "^\\*>([^\\[]+)$", "")) {
      return;
   }

   char buffer[1024] = {0};
   strcpy(buffer, "<a name=\"");
   strcat(buffer, pre.getSubmatch(1));
   strcat(buffer, "\"></a>");
   strcat(buffer, strang.getBase());
   strang.setSize(strlen(buffer)+1);
   strcpy(strang.getBase(), buffer);
}



//////////////////////////////
//
// addLabelHyperlinks --
//

void addLabelHyperlinks(Array<char>& strang) {
   PerlRegularExpression pre;
   if (!pre.search(strang, "^\\*>(.*)\\[(.*)\\]", "")) {
      return;
   }

   char buffer[10123] = {0};
   strcpy(buffer, "*>");
   strcat(buffer, pre.getSubmatch(1));
   strcat(buffer, "[");

   PerlRegularExpression pre2;
   Array<Array<char> > tokens;
   pre2.getTokens(tokens, ",", pre.getSubmatch(2));
   
   int i;
   for (i=0; i<tokens.getSize(); i++) {
      strcat(buffer, "<a href=\"#");
      strcat(buffer, tokens[i].getBase());
      strcat(buffer, "\">");
      strcat(buffer, tokens[i].getBase());
      strcat(buffer, "</a>");
      if (i < tokens.getSize() - 1) {
         strcat(buffer, ",");
      }
   }
   strcat(buffer, "]");
   strang.setSize(strlen(buffer) + 1);
   strcpy(strang.getBase(), buffer);
}



//////////////////////////////
//
// printReferenceRecord --
//

void printReferenceRecord(ostream& out, HumdrumFile& infile, int line) {
   int& i = line;

   PerlRegularExpression pre;
   if (!pre.search(infile[line][0], "^!!!([^!:]+):(.*)$", "")) {
      out << "<tr valign=\"baseline\"";
      if (classQ) {
         out << " class=\"" << prefix << "Line " << prefix << "Gcom\"";
      }
      out << "><td colspan=" << infile.getMaxTracks() << ">";
      out << infile[i] << "</font></td></tr>\n";
      return;
   } 

   Array<char> description;
   
   infile[line].getBibliographicMeaning(description, pre.getSubmatch(1));
   PerlRegularExpression pre2;
   pre2.sar(description, "\"", "", "g");

   out << "<tr valign=\"baseline\"";
   if (classQ) {
      out << " class=\"" << prefix << "Line " << prefix  << "Ref\"";
   }
   out << "><td colspan=" << infile.getMaxTracks() << ">";
   out << "<span class=\"" << prefix << "Key\" title=\"";
   out << description << "\">";
   out << "!!!";
   out << pre.getSubmatch(1);
   out << ":";
   out << pre.getSubmatch(2);
   out << "</span>";
   out << "</td></tr>\n";

}



//////////////////////////////
//
// printGlobalComment --
//

void printGlobalComment(ostream& out, HumdrumFile& infile, int line) {
   int& i = line;
   out << "<tr valign=\"baseline\"";
   if (classQ) {
      out << " class=\"" << prefix << "Line " << prefix << "Gcom\"";
   }
   out << "><td colspan=" << infile.getMaxTracks() << ">";
   out << infile[i] << "</font></td></tr>\n";
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("p|page|full=b",   "print a full page instead of just table");
   opts.define("C|no-class=b",    "don't label elements with classes");
   opts.define("css=b",           "Print an example CSS ");
   opts.define("textarea|ta|t=b", "print data in a textarea after main table");
   opts.define("M|no-marks=b",    "don't highlight marked notes");
   opts.define("P|prefix=s:hum",  "prefix for CSS classes");
   opts.define("k|kern=b",        "markup kern data with spans");

   opts.define("author=b",          "author of program");
   opts.define("version=b",         "compilation info");
   opts.define("example=b",         "example usages");
   opts.define("h|help=b",          "short description");
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, March 2011" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: June 2014" << endl;
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

   fullQ     =  opts.getBoolean("page");
   classQ    = !opts.getBoolean("no-class");
   cssQ      =  opts.getBoolean("css");
   textareaQ =  opts.getBoolean("textarea");
   markQ     = !opts.getBoolean("no-marks");
   kernQ     =  opts.getBoolean("kern");
   prefix    =  opts.getString("prefix").data();

   Markchar.setSize(0);;
   Markcolor.setSize(0);;
}



//////////////////////////////
//
// example -- example usage of the humtable program
//

void example(void) {
   cout <<
   "                                                                         \n"
   << endl;
}





//////////////////////////////
//
// usage -- gives the usage statement for the humtable program
//

void usage(const char* command) {
   cout <<
   "                                                                         \n"
   << endl;
}



// md5sum: 1790292d3c46c0b1f40462824067fad0 humtable.cpp [20170605]
