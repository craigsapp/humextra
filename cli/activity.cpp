//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Oct  4 21:57:39 PDT 2011
// Last Modified: Fri Oct 24 16:49:55 PDT 2014 Do not count non-**kern data.
// Filename:      ...museinfo/examples/all/activity.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/activity.cpp
// Syntax:        C++; museinfo
//
// Description:   Cumulative articulation plot generator.  Counts the number
//                of notes within each measure of the score and then outputs
//                either raw data or instructions for creating a plot.
//
// Interesting webpages:
//    http://gnuplot-tricks.blogspot.com

#include "humdrum.h"
#include "PerlRegularExpression.h"

#include <string>

using namespace std;

///////////////////////////////////////////////////////////////////////////

// function declarations
void      checkOptions          (Options& opts, int argc, char* argv[]);
void      example               (void);
void      usage                 (const string& command);
void      processFile           (HumdrumFile& infile);
void      processFileSeparate   (HumdrumFile& infile);
int       countObject           (const string& buffer);
ostream&  drawArrows            (HumdrumFile& infile, ostream& out,
                                 vector<int>& barvals);
int       getTrackMap           (vector<int>& trackmap, HumdrumFile& infile);
int       allEqual              (vector<int>& array, int value);
double    getMaximum            (vector<vector<double> >& array);
void      printTickLabels       (ostream& out, int maxval);
void      printMensurations     (ostream& out, HumdrumFile& infile);
void      printSeparateMensurations(ostream& out, HumdrumFile& infile);
void      printTitle            (ostream& out, HumdrumFile& infile);
ostream&  encodeText            (ostream& out, const string& string);
ostream&  printPartAbbreviations(ostream& out, HumdrumFile& infile);
ostream&  placeMensuration      (ostream& out, const string& mensur, double xpos,
                                 double ypos, const string& color,
                                 const string& graph);
int       checkSounding         (HumdrumFile& infile, int row, int col);

// global variables
Options   options;             // database for command-line arguments
int       gnuplotQ   = 0;      // used with --gnuplot option
int       separateQ  = 0;      // used with -s option
int       LabelCount = 0;      // used for printing gnuplot labels
int       ciconiaQ   = 1;
int       Scale      = 2;
string    yrange     = "";     // used with --yrange option
string    titleFont  = "Times New Roman";
// string    titleFont  = "Vera";


///////////////////////////////////////////////////////////////////////////


int main(int argc, char** argv) {
	checkOptions(options, argc, argv);
	HumdrumFile infile;

	int i;
	// figure out the number of input files to process
	int numinputs = options.getArgCount();

	for (i=0; i<numinputs || i==0; i++) {
		infile.clear();

		// if no command-line arguments read data file from standard input
		if (numinputs < 1) {
			infile.read(cin);
		} else {
			infile.read(options.getArg(i+1));
		}
		if (separateQ) {
			processFileSeparate(infile);
		} else {
			processFile(infile);
		}
	}
}



///////////////////////////////////////////////////////////////////////////

//////////////////////////////
//
// allEqual -- true if all are equal to value.
//

int allEqual(vector<int>& array, int value) {
	int i;
	if (array.size() <= 1) {
		return 1;
	}
	for (i=1; i<(int)array.size(); i++) {
		if (array[i] != array[i-1]) {
			return 0;
		}
	}

	return 1;
}



//////////////////////////////
//
// processFileSeparate --
//

void processFileSeparate(HumdrumFile& infile) {
	int i, j, k;
	char buffer[1024] = {0};
	int tcount;
	int measure = 0;

	int firstQ  = 0;
	PerlRegularExpression pre;
	vector<int> barvals;
	vector<vector<double> > cumvals;
	barvals.reserve(100000);
	cumvals.reserve(100000);

	vector<int> trackmap;
	int vcount = getTrackMap(trackmap, infile);
	vector<int> cumsum;
	cumsum.resize(vcount);
	std::fill(cumsum.begin(), cumsum.end(), 0);
	vector<int> sounding;
	sounding.resize(vcount);
	std::fill(sounding.begin(), sounding.end(), 0);
	int vindex;

	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].isMeasure()) {
			if ((firstQ == 0) && (allEqual(cumsum, 0))) {
				firstQ++;
				if (pre.search(infile[i][0], "(\\d+)")) {
					measure = atoi(pre.getSubmatch(1));
				}
				continue;
			}
			firstQ++;
			if ((!barvals.empty()) && (barvals.back() == measure)) {
				for (j=0; j<(int)cumsum.size(); j++) {
					cumvals.back().at(j) += cumsum.at(j);
				}
			} else {
				barvals.push_back(measure);
				cumvals.resize((int)cumvals.size()+1);
				cumvals.back().resize(vcount);
				std::fill(cumvals.back().begin(), cumvals.back().end(), 0);
				for (j=0; j<(int)cumsum.size(); j++) {
					cumvals.back().at(j) = cumsum.at(j);
					if ((cumsum.at(j) == 0) && (sounding.at(j) > 0)) {
						// no notes attacked in the current measure
						// but notes are sounding from the previous
						// measure, so mark with special 0.5 code:
						cumvals.back()[j] = 0.5;
					}
				}
				std::fill(sounding.begin(), sounding.end(), 0);
			}
			if (pre.search(infile[i][0], "(\\d+)")) {
				measure = atoi(pre.getSubmatch(1));
			}
			std::fill(cumsum.begin(), cumsum.end(), 0);
			continue;
		}
		if (!infile[i].isData()) {
			continue;
		}

		for (j=0; j<infile[i].getFieldCount(); j++) {
			if (!infile[i].isExInterp(j, "**kern")) {
				continue;
			}
			tcount = infile[i].getTokenCount(j);
			vindex = trackmap[infile[i].getPrimaryTrack(j)];
			if (vindex == -1) {
				cerr << "Funny negative track index" << endl;
				cerr << "Line :   " << i+1 << endl;
				cerr << "Column : " << j+1 << endl;
				cerr << "Track :  " << infile[i].getPrimaryTrack(j);
				exit(1);
			}
			if (tcount == 1) {
				int count = countObject(infile[i][j]);
				cumsum.at(vindex) += count;
				sounding.at(vindex) += checkSounding(infile, i, j);
			} else {
				for (k=0; k<tcount; k++) {
					infile[i].getToken(buffer, j, k);
					int ocount = countObject(buffer);
					cumsum.at(vindex) += ocount;
					sounding.at(vindex) += checkSounding(infile, i, j);
				}
			}
		}
	}


	if (gnuplotQ) {
		// style template:
		// http://commons.wikimedia.org/wiki/File:Berlin_population2.svg
		cout << "set term png size " << Scale * 800 << "," << Scale * 250
			  << " enhanced font \"" << titleFont << ","
			  << 10 * Scale << "\"\n";
		cout << "set o\n";      // send to standard output
		cout << "unset key\n";  // do not display legend
		cout << "set xlabel \"measure\" offset 0,0.75\n";
		cout << "set cbrange [0:1]\n";
		// cout << "set encoding iso_8859_1\n";
		// cout << "set encoding iso10646\n";

		printTitle(cout, infile);
		drawArrows(infile, cout, barvals);

		//cout << "set ylabel \"voice\" offset 1.75,0\n";
		cout << "set ylabel \"voice\"\n";
		// cout << "set ytics 1\n";
		printPartAbbreviations(cout, infile);
		//cout << "set mxtics 5\n";
		printTickLabels(cout, barvals.back());
		printSeparateMensurations(cout, infile);

		cout << "plot '-' using 1:2:($3 * 255):($4 * 255):($5 * 255) with rgbimage" << endl;

	}


	if (gnuplotQ) {

		double maxval = getMaximum(cumvals);
		double color = 0.0;

		for (i=0; i<(int)barvals.size(); i++) {
			for (j=0; j<(int)cumvals[i].size(); j++) {
				// color = 1.0 - (cumvals[i][j]+1)/maxval;
				color = 1.0 - cumvals[i][j]/maxval;
				if (cumvals[i][j] == 0) {
					color = 1.0;
				}
				// if (color > 0.0) {
					cout << barvals[i];
					cout << "\t" << j;
					cout << "\t" << color;
					cout << "\t" << color;
					cout << "\t" << color;
					cout << endl;
				//  }
			}
		}



	} else {

		// print the results
		int sum = 0;
		for (i=0; i<(int)barvals.size(); i++) {
			cout << barvals[i];
			sum = 0;
			for (j=0; j<(int)cumvals[i].size(); j++) {
				sum += cumvals[i][j];
			}
			cout << "\t" << sum;
			for (j=0; j<(int)cumvals[i].size(); j++) {
				cout << "\t" << cumvals[i][j];
			}
			cout << "\n";
		}
	}


	if (gnuplotQ) {
		cout << "e\n";
	}

}



//////////////////////////////
//
// checkSounding -- returns true if there is a note sounding.
//     (not just if an attack).
//

int checkSounding(HumdrumFile& infile, int row, int col) {
	int& i = row;
	int& j = col;
	int ii;
	int jj;

	if (!infile[i].isExInterp(j, "**kern")) {
		return 0;
	}

	ii = i;
	jj = j;
	if (strcmp(infile[i][j], ".") == 0) {
		ii = infile[i].getDotLine(j);
		jj = infile[i].getDotSpine(j);
	}

	if (strchr(infile[ii][jj], 'r') != NULL) {
		return 0;
	}

	return 1;
}



//////////////////////////////
//
// printPartAbbreviations --  search for the abbreviations line
//    in the file.  The abbreviations. start with *I' and then the
//    part's abbreviation name.  Assuming no spine splits
//    before abbreviation name.  Also assuming abbreviations occurs
//    before any data.
//

ostream& printPartAbbreviations(ostream& out, HumdrumFile& infile) {
	vector<string> names(infile.getMaxTracks());
	int kmax = 0;
	int kindex = 0;
	for (int i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isInterpretation()) {
			continue;
		}
		kindex = -1;
		for (int j=0; j<infile[i].getFieldCount(); j++) {
			if (!infile[i].isExInterp(j, "**kern")) {
				continue;
			}
			kindex++;
			if (kindex + 1 > kmax) {
				kmax = kindex + 1;
			}
			if (strncmp("*I'", infile[i][j], 3) == 0) {
				names[kindex] = infile[i][j]+3;
			}
		}
	}

	names.resize(kmax);
	if (kmax > 0) {
		out << "set ytics (";
		for (int i=0; i<kmax; i++) {
			out << "\"" << names[i] << "\" " << i;
			if (i < kmax - 1) {
				out << ", ";
			}
		}
		out << ")\n";
	}

	if (kmax < 6) {
		out << "set yrange [-1:" << kmax << "]\n";
	} else {
		out << "set yrange [-1:" << kmax+0.5 << "]\n";
	}

	return out;
}



//////////////////////////////
//
// printTitle --
// http://lavica.fesb.hr/cgi-bin/info2html?%28gnuplot%29title_
//   [OPR ]OTL (SCT)
//

void printTitle(ostream& out, HumdrumFile& infile) {
	char OPR[1024] = {0};
	char OTL[1024] = {0};
	char SCT[1024] = {0};

	infile.getBibValue(OPR, "OPR");
	infile.getBibValue(OTL, "OTL");
	infile.getBibValue(SCT, "SCT");

	if ((strlen(OPR) == 0) && (strlen(OTL) == 0) && (strlen(SCT) == 0)) {
		// no title to print.
		return;
	}

	out << "set title \"";
	if (strlen(OPR) > 0) {
		encodeText(out, OPR);
		out << "  ";
	}
	encodeText(out, OTL);
	if (strlen(SCT) > 0) {
		out << " (";
		encodeText(out, SCT);
		out << ")";
	}
	// out << "\" offset 0,-1 font \"VeraSe," << Scale * 12 << "\"\n";
	// out << "\" offset 0,-0.75 font \"tt1095m_.ttf," << Scale * 16 << "\"\n";
	out << "\" offset 0,-0.75 font \"" << titleFont << "," << Scale * 16 << "\"\n";
	// tt1095m_.ttf  roman
	// tt1096m_.ttf  italic
	// tt1099m_.ttf  bold
	// tt1100m_.ttf  bold-italic
}



//////////////////////////////
//
// encodeText -- convert HTML style accented characters into printable form.
//    http://www.pjb.com.au/comp/diacritics.html
//

ostream& encodeText(ostream& out, const string& astring) {
	string newstring = astring;
	PerlRegularExpression pre;
	pre.sar(newstring, "&agrave;", "\340", "g");
	pre.sar(newstring, "&aacute;", "\341", "g");
	pre.sar(newstring, "&acirc;",  "\342", "g");
	pre.sar(newstring, "&atilde;", "\343", "g");
	pre.sar(newstring, "&auml;",   "\344", "g");
	pre.sar(newstring, "&aring;",  "\345", "g");
	pre.sar(newstring, "&aelig;",  "\346", "g");
	pre.sar(newstring, "&ccedil;", "\347", "g");
	pre.sar(newstring, "&egrave;", "\350", "g");
	pre.sar(newstring, "&eacute;", "\351", "g");
	out << newstring;
	return out;
}



//////////////////////////////
//
// getMaximum -- return the largest value in the matrix.
//

double getMaximum(vector<vector<double> >& array) {
	int i, j;
	int output = 0;
	for (i=0; i<(int)array.size(); i++) {
		for (j=0; j<(int)array[i].size(); j++) {
			if (output < array[i][j]) {
				output = array[i][j];
			}
		}
	}
	return output;
}



///////////////////////////////
//
// printMensurations --
//

void printMensurations(ostream& out, HumdrumFile& infile) {
	int i, j;
	int barnum = 1;
	PerlRegularExpression pre;
	int first = 0;
	int pfirst = 0;
	char lastbuffer[1024] = {0};
	char buffer[1024] = {0};
	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].isMeasure() && pre.search(infile[i][0], "(\\d+)")) {
			barnum = atoi(pre.getSubmatch(1));
		}
		buffer[0] = '\0';
		if (infile[i].isGlobalComment()) {
			// check for a primary-mensuration marker in the case that
			// the mensuration in different voices are different.
			if (pre.search(infile[i][0],
					"!!primary-mensuration:\\s*met\\((.*)\\)\\s*$", "i")) {
				strcpy(buffer, pre.getSubmatch(1));
			}
		} else {
			if (!infile[i].isInterpretation()) {
				continue;
			}
			first = 0;
			for (j=0; j<infile[i].getFieldCount(); j++) {
				if (!infile[i].isExInterp(j, "**kern")) {
					continue;
				}
				if (!pre.search(infile[i][j], "^\\*met\\((.*?)\\)")) {
					// all mensurations must be the same; otherwise ignore
					buffer[0] = '\0';
					break;
				}
				if (first == 0) {
					strcpy(buffer, pre.getSubmatch(1));
					first++;
				} else {
					if (strcmp(buffer, pre.getSubmatch(1)) != 0) {
						buffer[0] = '\0';
						break;
					}
				}
			}
		}

		if (pre.search(buffer, "^\\s*$")) {
			continue;
		}

		if (strcmp(buffer, lastbuffer) == 0) {
			// don't repeat a mensuration display
			continue;
		}

		strcpy(lastbuffer, buffer);


		double vpos    = 0.04;
		double offset  = 0.00;
		double poffset = 0.00;

		if (pfirst++ == 0) {
			poffset = 0.5;
		} else {
			poffset = 0.0;
		}
		if (separateQ) {
			poffset = 0;
		}

		placeMensuration(out, buffer, barnum+poffset, vpos + offset,
				"dark-blue", "graph ");
	}
}



///////////////////////////////
//
// printSeparateMensurations --
//

void printSeparateMensurations(ostream& out, HumdrumFile& infile) {
	int i, j;
	int barnum = 1;
	PerlRegularExpression pre;
	// char buffer[1024] = {0};

	vector<string> lastmensur;

	lastmensur.resize(infile.getMaxTracks());
	for (i=0; i<(int)lastmensur.size(); i++) {
		lastmensur[i][0] = '\0';
	}

	vector<int> track2part(infile.getMaxTracks()+1, -1);
	int kindex = -1;

	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isInterpretation()) {
			continue;
		}
		for (j=0; j<infile[i].getFieldCount(); j++) {
			if (!infile[i].isExInterp(j, "**kern")) {
				continue;
			}
			kindex++;
			track2part[infile[i].getPrimaryTrack(j)] = kindex;
		}
		break;
	}

	int k;
	// double vpos    = 0.04;
	//double offset  = 0.00;
	// double poffset = 0.00;

	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].isMeasure() && pre.search(infile[i][0], "(\\d+)")) {
			barnum = atoi(pre.getSubmatch(1));
		}
		// buffer[0] = '\0';
		if (infile[i].isGlobalComment()) {
			// check for a primary-mensuration marker in the case that
			// the mensuration in different voices are different.
			//if (pre.search(infile[i][0],
			//      "!!primary-mensuration:\\s*met\\((.*)\\)\\s*$", "i")) {
			//   strcpy(buffer, pre.getSubmatch(1));
			//}
		} else {
			if (!infile[i].isInterpretation()) {
				continue;
			}
			for (j=0; j<infile[i].getFieldCount(); j++) {
				if (!infile[i].isExInterp(j, "**kern")) {
					continue;
				}

				if (pre.search(infile[i][j], "^\\*met\\((.*?)\\)")) {
					k = track2part[infile[i].getPrimaryTrack(j)];
					if (lastmensur[k] != pre.getSubmatch(1)) {
						lastmensur[k] = pre.getSubmatch(1);
						placeMensuration(out, pre.getSubmatch(1), barnum, k-0.1,
								"red", "");
					}
				}
			}
		}
	}
}



//////////////////////////////
//
// placeMensuration --
//

ostream& placeMensuration(ostream& out, const string& mensur, double xpos,
		double ypos, const string& color, const string& graph) {

	// found a mensuration, so print it at the current measure position
	// http://lavica.fesb.hr/cgi-bin/info2html?%28gnuplot%29label

	if (mensur == "C|") {
		if (ciconiaQ)  {
			out << "set label " << ++LabelCount << " \"Z\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front font \"ciconia_.pfb,"
				 << Scale * 20 << "\" "
				 << "textcolor rgb \"" << color << "\"\n";
		} else {
			out << "set label " << ++LabelCount << " \"C\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front font \"" << titleFont << "," << Scale * 10
				 << "\" textcolor rgb "
				 << "\"" << color << "\"\n";
			out << "set label " << ++LabelCount << " \"|\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front font \"" << titleFont << "," << Scale * 10
				 << "\" textcolor rgb "
				 << "\"" << color << "\n";
		}

	} else if (mensur == "C|2") {
		if (ciconiaQ)  {
			out << "set label " << ++LabelCount << " \"Z\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front font \"ciconia_.pfb,"
				 << Scale * 20 << "\" "
				 << "textcolor rgb \"" << color << "\"\n";
			out << "set label " << ++LabelCount << " \"     2\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front font \"" << titleFont << ","
				 << Scale * 10 << "\" "
				 << "textcolor rgb \"" << color << "\"\n";
		} else {
			out << "set label " << ++LabelCount << " \"" << mensur << "\" "
				 << "at first " << xpos+.5 << "," << graph
				 << ypos << " center offset 0.3,0 front font \"" << titleFont << ","
				 << Scale * 10 << "\""
				 << " textcolor rgb " << "\"" << color << "\"\n";
		}

	} else if (mensur == "C|3") {
		if (ciconiaQ)  {
			out << "set label " << ++LabelCount << " \"Z\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front font \"ciconia_.pfb,"
				 << Scale * 20 << "\" "
				 << "textcolor rgb \"" << color << "\"\n";
			out << "set label " << ++LabelCount << " \"     3\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front font \"" << titleFont << ","
				 << Scale * 10 << "\" "
				 << "textcolor rgb \"" << color << "\"\n";
		} else {
			out << "set label " << ++LabelCount << " \"" << mensur << "\" "
				 << "at first " << xpos+.5 << "," << graph
				 << ypos << " center offset 0.3,0 front font \"" << titleFont << ","
				 << Scale * 10 << "\""
				 << " textcolor rgb " << "\"" << color << "\"\n";
		}

	} else if (mensur == "O|") {
		if (ciconiaQ) {
			out << "set label " << ++LabelCount << " \"o\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front font \"ciconia_.pfb,"
				 << Scale * 20 << "\" "
				 << "textcolor rgb \"" << color << "\"\n";
		} else {
			out << "set label " << ++LabelCount << " \"O\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front font \"" << titleFont << ","
				 << Scale * 10 << "\" textcolor rgb "
				 << "\"" << color << "\"\n";
			out << "set label " << ++LabelCount << " \"|\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front font \"" << titleFont << "," << Scale * 10
				 << "\" textcolor rgb "
				 << "\"" << color << "\"\n";
		}

	} else if (mensur == "O.") {
		if (ciconiaQ) {
			out << "set label " << ++LabelCount << " \"P\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front "
				 << "font \"ciconia_.pfb," << Scale * 20 << "\" textcolor rgb "
				 << "\"" << color << "\"\n";
		} else {
			out << "set label " << ++LabelCount << " \"O\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front font \"" << titleFont << ","
				 << Scale * 10 << "\" textcolor rgb "
				 << "\"" << color << "\"\n";
			out << "set label " << ++LabelCount << " \"\267\" "
				 << "at first " << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front font \"symbol.ttf,"
				 << Scale * 9 << "\" "
				 << "textcolor rgb \"" << color << "\"\n";
		}

	} else if (mensur == "C.") {
		if (ciconiaQ) {
			out << "set label " << ++LabelCount << " \"c\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front "
				 << "font \"ciconia_.pfb," << Scale * 20 << "\" textcolor rgb "
				 << "\"" << color << "\"\n";
		} else {
			out << "set label " << ++LabelCount << " \"C\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front font \"" << titleFont << ","
				 << Scale * 10 << "\" textcolor rgb "
				 << "\"" << color << "\"\n";
			out << "set label " << ++LabelCount << " \"\267\" "
				 << "at first " << xpos+.5 << "," << graph << ypos
				 << " center offset 0.5,0 front font \"symbol.ttf,"
				 << Scale * 9 << "\" "
				 << "textcolor rgb \"" << color << "\"\n";
		}

	} else if (mensur == "O") {
		if (ciconiaQ) {
			out << "set label " << ++LabelCount << " \"O\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front "
				 << "font \"ciconia_.pfb," << Scale * 20 << "\" textcolor rgb "
				 << "\"" << color << "\"\n";
		} else {
			out << "set label " << ++LabelCount << " \"O\" "  << "at first "
				 << xpos+.5 << "," << graph << ypos
				 << " center offset 0.3,0 front font \"" << titleFont << ","
				 << Scale * 10 << "\" textcolor rgb "
				 << "\"" << color << "\"\n";
		}

	} else if (mensur == "Cr") {
		out << "set label " << ++LabelCount << " \"" << "C" << "\" "
			 << "at first " << xpos+.5 << "," << graph
			 << ypos << " center rotate by 180 offset 0.3,graph 0.04 front font \"" << titleFont << ","
			 << Scale * 10 << "\""
			 << " textcolor rgb " << "\"" << color << "\"\n";

	} else {
		out << "set label " << ++LabelCount << " \"" << mensur << "\" "
			 << "at first " << xpos+.5 << "," << graph
			 << ypos << " center offset 0.3,0 front font \"" << titleFont << ","
			 << Scale * 10 << "\""
			 << " textcolor rgb " << "\"" << color << "\"\n";
	}

	return out;
}



//////////////////////////////
//
// getTrackMap --
//

int getTrackMap(vector<int>& trackmap, HumdrumFile& infile) {
	trackmap.resize(infile.getMaxTracks()+1);
	std::fill(trackmap.begin(), trackmap.end(), -1);
	int counter = 0;
	for (int i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isInterpretation()) {
			continue;
		}
		for (int j=0; j<infile[i].getFieldCount(); j++) {
			if (infile[i].isExInterp(j, "**kern")) {
				int track = infile[i].getPrimaryTrack(j);
				trackmap[track] = counter++;
			}
		}
		break;
	}
	return counter;
}



//////////////////////////////
//
// processFile --
//

void processFile(HumdrumFile& infile) {
	int i, j, k;
	char buffer[1024] = {0};
	int tcount;
	int measure = 0;
	int cumsum  = 0;
	int firstQ  = 0;
	// int counter = 0;
	PerlRegularExpression pre;
	vector<int> barvals(100000);
	vector<int> cumvals(100000);
	barvals.resize(0);
	cumvals.resize(0);

	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].isMeasure()) {
			if ((firstQ == 0) && (cumsum == 0)) {
				firstQ++;
				if (pre.search(infile[i][0], "(\\d+)")) {
					measure = atoi(pre.getSubmatch(1));
				}
				continue;
			}
			firstQ++;
			if (barvals.back() == measure) {
				cumvals.back() += cumsum;
			} else {
				barvals.push_back(measure);
				cumvals.push_back(cumsum);
			}
			if (pre.search(infile[i][0], "(\\d+)")) {
				measure = atoi(pre.getSubmatch(1));
			}
			cumsum = 0;
			continue;
		}
		if (!infile[i].isData()) {
			continue;
		}

		for (j=0; j<infile[i].getFieldCount(); j++) {
			if (!infile[i].isExInterp(j, "**kern")) {
				continue;
			}
			tcount = infile[i].getTokenCount(j);
			if (tcount == 1) {
				cumsum += countObject(infile[i][j]);
			} else {
				for (k=0; k<tcount; k++) {
					infile[i].getToken(buffer, j, k);
					// counter = countObject(buffer);
					cumsum += countObject(buffer);
				}
			}
		}
	}


	if (gnuplotQ) {
		// style template:
		// http://commons.wikimedia.org/wiki/File:Berlin_population2.svg
		cout << "set term png size " << Scale * 800 << ","
			  << Scale* 250 << " enhanced font \"" << titleFont << ","
			  << Scale * 10 << "\"\n";
		cout << "set o\n";      // send to standard output
		cout << "unset key\n";  // do not display legend
		cout << "set xlabel \"measure\" offset 0,0.75\n";
		if (pre.search(yrange, "[\\d.+-]+:[\\d.+-]")) {
			if (pre.search(yrange, "^\\s*\\[.*\\]\\s*$")) {
				cout << "set yrange " << yrange << "\n";
			} else {
				cout << "set yrange [" << yrange << "]\n";
			}
		}
		cout << "set grid linewidth " << Scale * 0.5 << "\n";
		// cout << "set encoding iso_8859_1\n";
		// cout << "set encoding iso10646\n";

		printTitle(cout, infile);
		drawArrows(infile, cout, barvals);

		cout << "set ylabel \"attacks/measure\" offset 1.75,0\n";
		cout << "set style fill solid 0.25 transparent\n";

		printTickLabels(cout, barvals.back());
		printMensurations(cout, infile);

		cout << "plot '-' u 1:2 w filledcurves below x1 lt rgb 'dark-blue' lw 1\n";
	}


	// print the results
	for (i=0; i<(int)barvals.size(); i++) {
		cout << barvals[i] << "\t" << cumvals[i] << endl;
	}

	if (gnuplotQ) {
		cout << "e\n";
	}

}



//////////////////////////////
//
// printTickLabels -- print tick labels based on how much space is
//      available.
//

void printTickLabels(ostream& out, int maxval) {

	//cout << "set mxtics 5\n";
	if (maxval > 271) {
		cout << "set xtics 25\n";  // horizontal ticks every 25 measures
	} else if (maxval < 139) {
		cout << "set xtics 5\n";  // horizontal ticks every 5 measures
	} else {
		cout << "set xtics 10\n";  // horizontal ticks every 10 measures
	}

}



//////////////////////////////
//
// drawArrows -- draw arrows at double barlines
//

ostream& drawArrows(HumdrumFile& infile, ostream& out, vector<int>& barvals) {
	int count = 0;
	PerlRegularExpression pre;
	int i, j;
	double barnum = 0;
	string omd;
	// char buffer[1024] = {0};

	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isMeasure()) {
			continue;
		}
		// found a measure, check to see if a double barline
		if ((strstr(infile[i][0], "||") == NULL) &&
				(strstr(infile[i][0], "|!") == NULL)) {
			continue;
		}
		// continue if no measure number on double barline
		if (!pre.search(infile[i][0], "(\\d+)")) {
			continue;
		}
		barnum = atoi(pre.getSubmatch(1)) - 0.5;

/*
		// search for an !!!OMD: record after the double barline, but
		// before any data
		omd = "";

		for (j=i+1; j<infile.getNumLines(); j++) {
			if (infile[j].isData()) {
				break;
			}
			if (infile[j].isBibliographic()) {
				if (strcmp(infile[j].getBibKey(buffer, 1000), "OMD") != 0) {
					continue;
				}
				omd = infile[j].getBibValue(buffer, 1000);
			}
		}
*/

		if (count == 0) {
			out << "set style line 10"
				 << " linecolor rgb \"red\" linewidth " << Scale * 2.05 << "\n";
		}
		count++;
		// arrow parameters
		// http://lavica.fesb.hr/cgi-bin/info2html?%28gnuplot%29arrow
		out << "set arrow " << count << " from " << barnum << ",graph 0.01 to "
			 << barnum << ",graph 0.99 nohead linestyle 10\n";
	}

	int toggle = 0;
	double lastfrac = -100;
	double offset = 0.0;
	double curfrac = -100;
	// int length;
	barnum = 1;
	for (j=0; j<infile.getNumLines(); j++) {
		if (infile[j].isMeasure()) {
			if (pre.search(infile[j][0], "(\\d+)")) {
				barnum = atoi(pre.getSubmatch(1)) + 0.5;
			}
		}
		if (!pre.search(infile[j][0], "section:\\s*(.*)\\s*$")) {
			continue;
		}
		omd = pre.getSubmatch(1);

/*
		if (!infile[j].isBibliographic()) {
			continue;
		}
		if (strcmp(infile[j].getBibKey(buffer, 1000), "OMD") != 0) {
			continue;
		}
		omd = infile[j].getBibValue(buffer, 1000);
*/

		// length = strlen(omd);
		if (!pre.search(omd, "^\\s*$")) {
			//if (count == 0) {
			//   out << "set style line 10"
			//       << " linecolor rgb \"red\" linewidth 2\n";
			//}
			count++;
			curfrac = (double)barnum/barvals.back();
			if ((curfrac - lastfrac) < 0.14) {
				toggle = !toggle;
				if (toggle == 0) {
					offset = -0.00;
				} else {
					offset = -0.05;
				}
			} else {
				toggle = 0;
			}
			lastfrac = curfrac;
			double vpos = 0.93;
			// label parameters
			// http://lavica.fesb.hr/cgi-bin/info2html?%28gnuplot%29label
			if (pre.search(omd, ".*pars:\\s(.*)$", "i")) {
				out << "set label " << ++LabelCount << " \"" << pre.getSubmatch(1)
					 << "\" at first " << barnum << ",graph " << vpos + offset
					 << " left front font \"," << Scale * 10 << "\" textcolor rgb "
					 << "\"orange\"\n";
			} else {
				out << "set label " << ++LabelCount << " \"" << omd
					 << "\" at first " << barnum << ",graph " << vpos + offset
					 << " left front font \"," << Scale * 10 << "\" textcolor rgb "
					 << "\"orange\"\n";
			}
		}
	}
	return out;
}



//////////////////////////////
//
// countObject -- count a note if it is not part of a sustain or a rest
//    or a null token.
//

int countObject(const string& buffer) {
	if (buffer == ".") {
		return 0;
	}
	if (buffer.find('r') != std::string::npos) {  // rests
		return 0;
	}
	if (buffer.find('_') != std::string::npos) {  // middle of sustained note
		return 0;
	}
	if (buffer.find(']') != std::string::npos) {  // end of sustained note
		return 0;
	}

	return 1;
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
	opts.define("g|gnuplot=b", "create gnuplot script");
	opts.define("s|separate=b", "separate each voices values");
	opts.define("yrange=s:", "set the vertical axis numeric range");

	opts.define("author=b", "author of the program");
	opts.define("version=b", "compilation info");
	opts.define("example=b", "example usages");
	opts.define("h|help=b", "short description");
	opts.process(argc, argv);

	// handle basic options:
	if (opts.getBoolean("author")) {
		cout << "Written by Craig Stuart Sapp, "
			  << "craig@ccrma.stanford.edu, October 2011" << endl;
		exit(0);
	} else if (opts.getBoolean("version")) {
		cout << argv[0] << ", version: 4 October 2010" << endl;
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

	if (opts.getBoolean("gnuplot")) {
		gnuplotQ = 1;
	}

	separateQ = opts.getBoolean("separate");
	if (opts.getBoolean("yrange")) {
		yrange = opts.getString("yrange");
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

void usage(const string& command) {
	cout <<
	"                                                                         \n"
	<< endl;
}


// md5sum: 6ae48ceb91ea80f8b5e5effd1074c066 activity.cpp [20160305]
