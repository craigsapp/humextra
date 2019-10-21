//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Dec 26 03:28:25 PST 2010
// Last Modified: Mon Feb  7 06:22:42 PST 2011 (added beam directions)
// Filename:      ...sig/examples/all/autostem.cpp 
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/autostem.cpp
// Syntax:        C++; museinfo
//
// Description:   Converts Humdrum files into MuseData files.
//

#include "humdrum.h"
#include "PerlRegularExpression.h"
#include "MuseData.h"

#include <string.h>
#include <math.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

//////////////////////////////////////////////////////////////////////////

class Coord {
	public:
		Coord(void) { clear(); }
		void clear(void) { i = j = -1; }
		int i;
		int j;
};

//////////////////////////////////////////////////////////////////////////

// function declarations:
void      checkOptions         (Options& opts, int argc, char** argv);
void      example              (void);
void      usage                (const string& command);
void      autostem             (HumdrumFile& infile);
void      getClefInfo          (vector<vector<int> >& baseline, 
                                HumdrumFile& infile);
void      addStem              (char* output, const string& input, 
                                const string& piece);
void      processKernTokenStemsSimpleModel(HumdrumFile& infile, 
                                vector<vector<int> >& baseline, 
                                int row, int col);
void      removeStems          (ostream& out, HumdrumFile& infile);
void      removeStem2          (HumdrumFile& infile, int row, int col);
int       getVoice             (HumdrumFile& infile, int row, int col);
void      getNotePositions     (vector<vector<vector<int> > >& notepos, 
                                vector<vector<int> >& baseline, 
                                HumdrumFile& infile);
void      printNotePositions   (HumdrumFile& infile, 
                                vector<vector<vector<int> > >& notepos);
void      getVoiceInfo         (vector<vector<int> >& voice, HumdrumFile& infile);
void      printVoiceInfo       (HumdrumFile& infile, vector<vector<int> >& voice);
void      processKernTokenStems(HumdrumFile& infile, 
                                vector<vector<int> >& baseline, int row, int col);
void      getMaxLayers         (vector<int>& maxlayer, vector<vector<int> >& voice,
                                HumdrumFile& infile);
void      assignStemDirections (vector<vector<int> >& stemdir, 
                                vector<vector<int> > & voice, 
                                vector<vector<vector<int> > >& notepos, 
                                HumdrumFile& infile);
void      assignBasicStemDirections(vector<vector<int> >& stemdir, 
                                vector<vector<int> >& voice, 
                                vector<vector<vector<int> > >& notepos, 
                                HumdrumFile& infile);
int       determineChordStem   (vector<vector<int> >& voice, 
                                vector<vector<vector<int> > >& notepos, 
                                HumdrumFile& infile, int row, int col);
void      insertStems          (HumdrumFile& infile, 
                                vector<vector<int> >& stemdir);
void      setStemDirection     (HumdrumFile& infile, int row, int col, 
                                int direction);
void      getBeamState         (vector<vector<string > >& beams, 
                                HumdrumFile& infile);
void      countBeamStuff       (const string& token, int& start, int& stop, 
                                int& flagr, int& flagl);
void      getBeamSegments      (vector<vector<Coord> >& beamednotes, 
                                vector<vector<string > >& beamstates, 
                                HumdrumFile& infile, vector<int> maxlayer);
int       getBeamDirection     (vector<Coord>& coords,  
                                vector<vector<int> >& voice, 
                                vector<vector<vector<int> > >& notepos);
void      setBeamDirection     (vector<vector<int> >& stemdir, 
                                vector<Coord>& bnote, int direction);


// User interface variables:
Options options;
int    debugQ        = 0;             // used with --debug option
int    removeQ       = 0;             // used with -r option
int    noteposQ      = 0;             // used with -p option
int    voiceQ        = 0;             // used with --voice option
int    removeallQ    = 0;             // used with -R option
int    overwriteQ    = 0;             // used with -o option
int    overwriteallQ = 0;             // used with -O option
int    Middle        = 4;             // used with -u option
int    Borderline    = 0;             // really used with -u option
int    notlongQ      = 0;             // used with -L option


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

	if (removeQ || overwriteQ) {
		stringstream tempstream;
		removeStems(tempstream, infile);
		infile.clear();
		infile.read(tempstream);
		if (removeQ) {
			cout << infile;
			exit(0);
		}
	} 

	autostem(infile);
	cout << infile;

	return 0;
}


//////////////////////////////////////////////////////////////////////////



//////////////////////////////
//
// removeStems --
//

void removeStems(ostream& out, HumdrumFile& infile) {
	int i, j;
	PerlRegularExpression pre;
	string buffer;
	buffer.resize(1024);

	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isData()) {
			out << infile[i] << "\n";
			continue;
		}
		for (j=0; j<infile[i].getFieldCount(); j++) {
			if (!infile[i].isExInterp(j, "**kern")) {
				out << infile[i][j];
				if (j < infile[i].getFieldCount()-1) {
					out << "\t";
				}
				continue;
			}
			buffer = infile[i][j];
			if (removeallQ || overwriteallQ) {
				pre.sar(buffer, "[\\\\/]x(?!x)", "", "g");
				pre.sar(buffer, "[\\\\/](?!x)", "", "g");
			} else {
				pre.sar(buffer, "[\\\\/](?!x)", "", "g");
			}
			out << buffer;
	
			if (j < infile[i].getFieldCount()-1) {
				out << "\t";
			}
		}
		out << "\n";
	}
}



//////////////////////////////
//
// autostem -- add an up/down stem on notes in **kern data which do not
//     already have stem information.
//

void autostem(HumdrumFile& infile) {
	vector<vector<int> > baseline;
	getClefInfo(baseline, infile);
	
	vector<vector<vector<int> > > notepos;     // staff line position of all notes
	getNotePositions(notepos, baseline, infile);

	if (noteposQ) {
		printNotePositions(infile, notepos);
		exit(0);
	}

	vector<vector<int> > voice;     // voice/layer number in track
	getVoiceInfo(voice, infile);

	if (voiceQ) {
		printVoiceInfo(infile, voice);
		exit(0);
	}

	vector<vector<int> > stemdir;   // stem directions;
	stemdir.resize(infile.getNumLines());
	for (int i=0; i<(int)stemdir.size(); i++) {
		stemdir[i].resize(infile[i].getFieldCount());
		std::fill(stemdir[i].begin(), stemdir[i].end(), 0);
	}
	// later add stems presently in data.
	assignStemDirections(stemdir, voice, notepos, infile);

	insertStems(infile, stemdir);

/*
	// simple model which only considers notes in isolation from any
	// beaming to other notes...
	int i, j;
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
				processKernTokenStems(infile, stemdir, i, j);
		}
	}
*/

}



//////////////////////////////
//
// insertStems -- put stem directions into the data.
//

void insertStems(HumdrumFile& infile, vector<vector<int> >& stemdir) {
	int i, j;

	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isData()) {
				continue;
		}
		for (j=0; j<infile[i].getFieldCount(); j++) {
				if (!infile[i].isExInterp(j, "**kern")) {
				continue;
				}
				setStemDirection(infile, i, j, stemdir[i][j]);
		}
	}
}



//////////////////////////////
//
// setStemDirection -- don't change stem direction if there is already
//    a stem on the note.
//

void setStemDirection(HumdrumFile& infile, int row, int col, int direction) {
	int& i = row;
	int& j = col;
	int k;

	if (strcmp(infile[i][j], ".") == 0) {
		return;
	}
	if (strchr(infile[i][j], 'r') != NULL) {
		return;
	}

	int tokencount = infile[i].getTokenCount(j);
	char buffer[128] = {0};
	char buffer2[128] = {0};
	int len = strlen(infile[i][j]);
	char output[len*2+tokencount];
	output[0] = '\0';

	double duration;
	if (notlongQ) {
		// Don't print stems on whole notes and breves.
		// Duration units are in quarter notes.
		duration = Convert::kernToDuration(infile[row][col]);
		if ((duration >= 4.0) && (duration < 16.0)) {
				return;
		}
	}

	for (k=0; k<tokencount; k++) {
		infile[i].getToken(buffer, j, k, 30);
		if ((strchr(buffer, '/') == NULL) & (strchr(buffer, '\\') == NULL)) {
				if (direction > 0) {
				addStem(buffer2, buffer, "/");
				} else if (direction < 0) {
				addStem(buffer2, buffer, "\\");
				} else {
				strcpy(buffer2, buffer);
				}
		} else {
				strcpy(buffer2, buffer);
		}
		strcat(output, buffer2);
		if (k < tokencount-1) {
				strcat(output, " ");
		}
	}
	infile[i].setToken(j, output);
}



//////////////////////////////
//
// assignStemDirections --
//

void assignStemDirections(vector<vector<int> >& stemdir, 
		vector<vector<int> > & voice, 
		vector<vector<vector<int> > >& notepos, HumdrumFile& infile) {

	vector<int> maxlayer;
	getMaxLayers(maxlayer, voice, infile);

	assignBasicStemDirections(stemdir, voice, notepos, infile);

	vector<vector<string > > beamstates;
	getBeamState(beamstates, infile);

	vector<vector<Coord> > beamednotes;
	getBeamSegments(beamednotes, beamstates, infile, maxlayer);

	// print notes which are beamed together for debugging:

	int i, j;
	if (debugQ) {
		for (i=0; i<(int)beamednotes.size(); i++) {
				cout << "!! ";
				for (j=0; j<(int)beamednotes[i].size(); j++) {
				cout << infile[beamednotes[i][j].i][beamednotes[i][j].j] << "\t";
				}
				cout << "\n";
		}
	}

	int direction;
	for (i=0; i<(int)beamednotes.size(); i++) {
		direction = getBeamDirection(beamednotes[i], voice, notepos);
		setBeamDirection(stemdir, beamednotes[i], direction);
	}

}



//////////////////////////////
//
// setBeamDirection --
//

void setBeamDirection(vector<vector<int> >& stemdir, vector<Coord>& bnote, 
		int direction) {
	int x;
	int i, j;
	for (x=0; x<(int)bnote.size(); x++) {
		i = bnote[x].i;
		j = bnote[x].j;
		stemdir[i][j] = direction;
	}
}




//////////////////////////////
//
// getBeamDirection -- return a consensus stem direction for beamed notes.
//

int getBeamDirection(vector<Coord>& coords, vector<vector<int> >& voice, 
		vector<vector<vector<int> > >& notepos) {

	// voice values are presumed to be 0 at the moment.

	int minn = 1000;
	int maxx = -1000;

	int x;
	int i, j, k;
	for (x=0; x<(int)coords.size(); x++) {
		i = coords[x].i;
		j = coords[x].j;
		if (voice[i][j] == 1) {
				return +1;
		}
		if (voice[i][j] == 2) {
				return -1;
		}
		for (k=0; k<(int)notepos[i][j].size(); k++) {
				if (minn > notepos[i][j][k]) {
				minn = notepos[i][j][k];
				}
				if (maxx < notepos[i][j][k]) {
				maxx = notepos[i][j][k];
				}
		}
	}

	if (maxx < 0 + Borderline) {
		// both minn and maxx are less than zero, so place stems up
		return +1;
	}
	if (minn > 0) {
		// both minn and maxx are greater than zero, so place stems down
		return -1;
	}

	if (abs(maxx) > abs(minn)) {
		// highest note is higher than lower note is lower, so place
		// stems down
		return -1;
	}
	if (abs(maxx) > abs(minn)) {
		// highest note is lower than lower note is lower, so place
		// stems up
		return +1;
	}

	// its a draw, so place stem up.
	return +1;
}



//////////////////////////////
//
// getBeamSegments -- arrange the beamed notes into a long list with
//    each entry being a list of notes containing one beam.  Each
//    beamed note set should have their beams all pointing in the same 
//    direction.
//

void getBeamSegments(vector<vector<Coord> >& beamednotes, 
		vector<vector<string > >& beamstates, HumdrumFile& infile, 
		vector<int> maxlayer) {

	beamednotes.reserve(1000000);
	beamednotes.resize(infile.getNumLines() * infile.getMaxTracks() / 2);

	vector<vector<vector<Coord> > > beambuffer;
	beambuffer.resize(infile.getMaxTracks() + 1);
	int i, j;
	for (i=0; i<(int)beambuffer.size(); i++) {
		beambuffer[i].resize(10); // layer  max 10, but allow growth
		for (j=0; j<(int)beambuffer[i].size(); j++) {
				beambuffer[i][j].reserve(100); // 100 notes in a beam expected as max
				beambuffer[i][j].resize(0); 
		}
	}

	Coord tcoord;
	char beamchar;
	int track, oldtrack, layer;
	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isData()) {
				continue;
		}
		oldtrack = 0;
		layer = 0;
		for (j=0; j<infile[i].getFieldCount(); j++) {
				if (!infile[i].isExInterp(j, "**kern")) {
				continue;
				}
				track = infile[i].getPrimaryTrack(j);
				if (track == oldtrack) {
				layer++;
				} else {
				layer = 0;
				}
				oldtrack = track;
				if (strcmp(infile[i][j], ".") == 0) {
				continue;
				}
				if (strchr(infile[i][j], 'r') != NULL) {
				continue;
				}
				beamchar = beamstates[i][j][0];
				if (beamchar == '\0') {
				beambuffer[track][layer].resize(0);  // possible unter. beam
				continue;
				}
				if ((beamchar == '[') || (beamchar == '=')) {
				// add a beam to the buffer and wait for more
				tcoord.i = i;
				tcoord.j = j;
				beambuffer[track][layer].push_back(tcoord);
				continue;
				}
				if (beamchar == ']') {
				// ending of a beam so store in permanent storage
				tcoord.i = i;
				tcoord.j = j;
				beambuffer[track][layer].push_back(tcoord);
				beamednotes.push_back(beambuffer[track][layer]);
				beambuffer[track][layer].resize(0);
				}
		}
	}
}



//////////////////////////////
//
// getMaxLayers --
//

void getMaxLayers(vector<int>& maxlayer, vector<vector<int> >& voice, 
		HumdrumFile& infile) {
	int track;
	maxlayer.resize(infile.getMaxTracks() + 1);
	std::fill(maxlayer.begin(), maxlayer.end(), 0);
	int i, j;
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
				if (strchr(infile[i][j], 'r') != NULL) {
				continue;
				}
				track = infile[i].getPrimaryTrack(j);
				if (voice[i][j] + 1 > maxlayer[track]) {
				maxlayer[track] = voice[i][j] + 1;
				}
		}
	}
}



//////////////////////////////
//
// getVoiceInfo -- 0 = only voice in track, 1 = layer 1, 2 = layer 2, etc.
//
// 0 voices will be stemmed up or down based on vertical positions of notes
// 1 voices will be stemmed up always
// 2 voices will be stemmed down always.
// 3 and higher are still to be determined.
//
// Future enhancement of this algorithm: if one voice contains an invisible 
// rest, then it will be ignored in the voice calculation.
//

void getVoiceInfo(vector<vector<int> >& voice, HumdrumFile& infile) {

	voice.resize(infile.getNumLines());

	int v;
	for (int i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isData()) {
				continue;
		}
		voice[i].resize(infile[i].getFieldCount());
		std::fill(voice[i].begin(), voice[i].end(), -1);
		for (int j=0; j<infile[i].getFieldCount(); j++) {
				if (!infile[i].isExInterp(j, "**kern")) {
				continue;
				} 
				if (strcmp(infile[i][j], ".") == 0) {
				continue;
				}
				v = getVoice(infile, i, j);
				voice[i][j] = v;
		}
	}
}



//////////////////////////////
//
// printvoiceInfo --
//

void printVoiceInfo(HumdrumFile& infile, vector<vector<int> >& voice) {
	PerlRegularExpression pre;
	string data;
	stringstream *pstream;


	int i, j;
	for (i=0; i<infile.getNumLines(); i++) {
		if (!(infile[i].isData() || infile[i].isLocalComment() ||
				infile[i].isMeasure() || infile[i].isInterpretation())) {
				cout << infile[i] << "\n";
				continue;
		}
		cout << infile[i] << "\t";
		if (infile[i].isMeasure()) {
				cout << infile[i][0] << "\n";
				continue;
		}
		if (infile[i].isLocalComment()) {
				cout << "!\n";
				continue;
		}
		if (infile[i].isInterpretation()) { 
				if (strncmp(infile[i][0], "**", 2) == 0) {
				cout << "**voice";
				} else if (strcmp(infile[i][0], "*-") == 0) {
				cout << "*-";
				} else {
				cout << "*";
				}
				cout << "\n";
				continue;
		}
		pstream = new stringstream;
		for (j=0; j<infile[i].getFieldCount(); j++) {
				if (!infile[i].isExInterp(j, "**kern")) {
				continue;
				}
				if (strcmp(infile[i][j], ".") == 0) {
				// ignore null tokens
				continue;
				}
				if (strchr(infile[i][j], 'r') != NULL) {
				// ignore rests
				continue;
				}
				(*pstream) << voice[i][j];
				(*pstream) << " ";
		}
		(*pstream) << ends;
		data = pstream->str();
		pre.sar(data, "\\s+$", "", "");
		if (data.empty()) {
				pre.sar(data, "^\\s*$", ".");
		}
		cout << data;
		cout << "\n";
		delete pstream;
		pstream = NULL;
	}
}




//////////////////////////////
//
// printNotePositions -- prints the vertical position of notes on the
//    staves.  Mostly for debugging purposes.  A spine at the end of the
//    data will be added containing all positions for notes on the line
//    in the sequence in which the notes occur from left to right.
//
//    The middle line of a 5-line staff is the zero position, and
//    position values are diatonic steps above or below that level:
//
//    ===== +4
//          +3
//    ===== +2
//          +1
//    =====  0
//          -1
//    ===== -2
//          -3
//    ===== -4
//

void printNotePositions(HumdrumFile& infile, 
		vector<vector<vector<int> > >& notepos) {

	PerlRegularExpression pre;
	string data;
	stringstream *pstream;


	int i, j, k;
	for (i=0; i<infile.getNumLines(); i++) {
		if (!(infile[i].isData() || infile[i].isLocalComment() ||
				infile[i].isMeasure() || infile[i].isInterpretation())) {
				cout << infile[i] << "\n";
				continue;
		}
		cout << infile[i] << "\t";
		if (infile[i].isMeasure()) {
				cout << infile[i][0] << "\n";
				continue;
		}
		if (infile[i].isLocalComment()) {
				cout << "!\n";
				continue;
		}
		if (infile[i].isInterpretation()) { 
				if (strncmp(infile[i][0], "**", 2) == 0) {
				cout << "**vpos";
				} else if (strcmp(infile[i][0], "*-") == 0) {
				cout << "*-";
				} else {
				cout << "*";
				}
				cout << "\n";
				continue;
		}
		pstream = new stringstream;
		for (j=0; j<infile[i].getFieldCount(); j++) {
				if (!infile[i].isExInterp(j, "**kern")) {
				continue;
				}
				if (strcmp(infile[i][j], ".") == 0) {
				// ignore null tokens
				continue;
				}
				if (strchr(infile[i][j], 'r') != NULL) {
				// ignore rests
				continue;
				}
				for (k=0; k<(int)notepos[i][j].size(); k++) {
				(*pstream) << notepos[i][j][k];
				if (k < (int)notepos[i][j].size()-1) {
					(*pstream) << " ";
				}
				}
				(*pstream) << " ";
		}
		(*pstream) << ends;
		data = pstream->str();
		pre.sar(data, "\\s+$", "", "");
		if (data.empty()) {
				pre.sar(data, "^\\s*$", ".");
		}
		cout << data;
		cout << "\n";
		delete pstream;
		pstream = NULL;
	}
}



//////////////////////////////
//
// getNotePositions -- Extract the vertical position of the notes
// on the staves, with the centerline of the staff being the 0 position
// and each diatonic step equal to 1, so that lines of 5-lined staff are
// at positions from bottom to top: -4, -2, 0, +2, +4.
//

void getNotePositions(vector<vector<vector<int> > >& notepos, 
		vector<vector<int> >& baseline, HumdrumFile& infile) {

	notepos.resize(infile.getNumLines());

	int location;
	char buffer[128] = {0};
	int i, j, k;
	int tokencount;

	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isData()) {
				continue;
		}
		notepos[i].resize(infile[i].getFieldCount());
		for (j=0; j<infile[i].getFieldCount(); j++) {
				if (!infile[i].isExInterp(j, "**kern")) {
				continue;
				} 
				if (strcmp(infile[i][j], ".") == 0) {
				// ignore null-tokens
				continue;
				}
				if (strchr(infile[i][j], 'r') != NULL) {
				// ignore rests
				continue;
				}

				tokencount = infile[i].getTokenCount(j);
				notepos[i][j].resize(tokencount);
				for (k=0; k<tokencount; k++) {
				infile[i].getToken(buffer, j, k, 30);
				location = Convert::kernToDiatonicPitch(buffer) -
						baseline[i][j] - 4;
				notepos[i][j][k] = location;
				}
		}
	}
}



//////////////////////////////
//
// processKernTokenStems --
//

void processKernTokenStems(HumdrumFile& infile, 
		vector<vector<int> >& baseline, int row, int col) {

	exit(1);


}



//////////////////////////////
//
// assignBasicStemDirections -- don't take beams into consideration.
//

void assignBasicStemDirections(vector<vector<int> >& stemdir, 
		vector<vector<int> >& voice, vector<vector<vector<int> > >& notepos, 
		HumdrumFile& infile) {

	int i, j;
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
				if (strchr(infile[i][j], 'r') != NULL) {
				continue;
				}
				if (removeQ) {
				removeStem2(infile, i, j);
				}

				if (strchr(infile[i][j], '/') != NULL) {
				stemdir[i][j] = +1;
				} else if (strchr(infile[i][j], '\\') != NULL) {
				stemdir[i][j] = -1;
				} else {
				stemdir[i][j] = determineChordStem(voice, notepos, infile, i, j);
				}
		}
	}
}



//////////////////////////////
//
// determineChordStem --
//

int determineChordStem(vector<vector<int> >& voice, 
		vector<vector<vector<int> > >& notepos, HumdrumFile& infile, int row, 
		int col) {

	if (notepos[row][col].size() == 0) {
		return 0;
	}

	if (voice[row][col] == 1) {
		return +1;
	}
	if (voice[row][col] == 2) {
		return -1;
	}
	if (voice[row][col] == 3) {
		return +1;
	}
	// voice == 0 means determine by vertical position

	if (notepos[row][col].size() == 1) {
		int location = notepos[row][col][0];
		if (location >= 0 + Borderline) {
				return -1;
		} else {
				return +1;
		}
	}

	// chord with more than one note so choose the extreme note as the 
	// one which decides the direction
 
	int i;
	int minn = notepos[row][col][0];
	int maxx = notepos[row][col][0];
	for (i=1; i<(int)notepos[row][col].size(); i++) {
		if (minn > notepos[row][col][i]) {
				minn = notepos[row][col][i];
		} 
		if (maxx < notepos[row][col][i]) {
				maxx = notepos[row][col][i];
		}
	}

	if (maxx < 0 + Borderline) {
		// all stems want to point upwards:
		return +1;
	}
	if (minn > 0) {
		// all stems want to point downwards:
		return -1;
	}

	if (abs(maxx) > abs(minn)) {
		return -1;
	}
	if (abs(minn) > abs(maxx)) {
		return +1;
	}

	return +1;
}



//////////////////////////////
//
// processKernTokenStemsSimpleModel --
//

void processKernTokenStemsSimpleModel(HumdrumFile& infile, 
		vector<vector<int> >& baseline, int row, int col) {

	int k;
	int& i = row;
	int& j = col;
	int tokencount = infile[i].getTokenCount(j);

	RationalNumber duration;
	if (tokencount == 1) {
		duration = Convert::kernToDurationR(infile[i][j]);
		if (duration >= 4) {
				// whole note or larger for note/chord, to not append a stem
				return;
		} 
		if ((strchr(infile[i][j], '/') != NULL) || 
				(strchr(infile[i][j], '/') != NULL)) {
			if (removeallQ || overwriteallQ) {
			if (strstr(infile[i][j], "/x") != NULL) {
				if (strstr(infile[i][j], "/xx") == NULL) {
					return;
				}
			} else if (strstr(infile[i][j], "\\x") != NULL) {
				if (strstr(infile[i][j], "\\xx") == NULL) {
					return;
				}
			}
			} else if (removeallQ || overwriteallQ) {
			removeStem2(infile, i, j);
			} else {
			// nothing to do
			return;
			}
		}
		if (strchr(infile[i][j], 'r') != NULL) {
				// rest which does not have a stem
				return;
		}
	}

	if (removeQ) {
		removeStem2(infile, i, j);
	}

	int voice = getVoice(infile, row, col);
	
	int len = strlen(infile[i][j]);
	char buffer[128] = {0};
	char buffer2[128] = {0};
	int location;
	char output[len*2+tokencount];
	output[0] = '\0';
	for (k=0; k<tokencount; k++) {
		infile[i].getToken(buffer, j, k, 30);
		if (i == 0) {
				duration = Convert::kernToDurationR(buffer);
				// if (duration >= 4) {
				//    // whole note or larger for note/chord, do not append a stem
				//    return;
				// } 
		}
		if (!((strchr(infile[i][j], '/') != NULL) || 
				(strchr(infile[i][j], '\\') != NULL))) {
			location = Convert::kernToDiatonicPitch(buffer) -
				baseline[row][col] - Middle;
			if (voice == 1) {
				addStem(buffer2, buffer, "/");
			} else if (voice == 2) {
				addStem(buffer2, buffer, "\\");
			} else {
				if (location > 0) {
					addStem(buffer2, buffer, "\\");
				} else {
					addStem(buffer2, buffer, "/");
				}
			}
			strcat(output, buffer2);
			if (k < tokencount-1) {
			strcat(output, " ");
			}
		} else {
			strcat(output, buffer);
			if (k < tokencount-1) {
				strcat(output, " ");
			}
		}
	}
	infile[i].setToken(j, output);
}



//////////////////////////////
//
// getVoice -- return 0 if the only spine in primary track, otherwise, return
// the nth column offset from 1 in the primary track.
//

int getVoice(HumdrumFile& infile, int row, int col) {
	int output = 0;
	int tcount = 0;
	int track = infile[row].getPrimaryTrack(col);
	int j;
	int testtrack;
	for (j=0; j<infile[row].getFieldCount(); j++) {
		testtrack = infile[row].getPrimaryTrack(j);
		if (testtrack == track) {
				tcount++;
		}
		if (col == j) {
				output = tcount;
		}
	}

	if (tcount == 1) {
		output = 0;
	}
	return output;
}



//////////////////////////////
//
// removeStem2 -- remove stem and any single x after the stem.
//

void removeStem2(HumdrumFile& infile, int row, int col) {
	PerlRegularExpression pre;
	string strin = infile[row][col];
	pre.sar(strin, "[\\\\/]x(?!x)", "", "g");
	pre.sar(strin, "[\\\\/](?!x)", "", "g");
	infile[row].setToken(col, strin.c_str());
}



//////////////////////////////
//
// addStem --
//

void addStem(char* output, const string& input, const string& piece) {
	PerlRegularExpression pre;

	if (pre.search(input, "(.*[ABCDEFG][n#-]*)(.*)$", "i")) {
		strcpy(output, pre.getSubmatch(1));
		strcat(output, piece.c_str());
		strcat(output, pre.getSubmatch(2));
	} else {
		strcpy(output, input.c_str());
		strcat(output, piece.c_str());
	}
}



///////////////////////////////
//
// getClefInfo -- Identify the clef of each note in the score.
//     Does not consider the case where a primary track contains 
//     more than one clef at a time.
//

void getClefInfo(vector<vector<int> >& baseline, HumdrumFile& infile) {
	vector<int> states(infile.getMaxTracks()+1, 
			Convert::kernClefToBaseline("*clefG2"));

	int i, j;
	baseline.resize(infile.getNumLines());
	for (i=0; i<(int)baseline.size(); i++) {
		baseline[i].resize(0);
	}

	int pt;
	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].isInterpretation()) {
				for (j=0; j<infile[i].getFieldCount(); j++) {
				if (!infile[i].isExInterp(j, "**kern")) {
					continue;
				}
				if (strncmp(infile[i][j], "*clef", 5) == 0) {
					pt = infile[i].getPrimaryTrack(j);
					states[pt] = Convert::kernClefToBaseline(infile[i][j]);
				}
				}
		}
		if (!infile[i].isData()) {
				continue;
		}
		baseline[i].resize(infile[i].getFieldCount());
		for (j=0; j<infile[i].getFieldCount(); j++) {
				if (!infile[i].isExInterp(j, "**kern")) {
				continue;
				}
				pt = infile[i].getPrimaryTrack(j);
				baseline[i][j] = states[pt];
		}
	}

}



//////////////////////////////
//
// checkOptions -- 
//

void checkOptions(Options& opts, int argc, char** argv) {
	opts.define("d|debug=b",    "Debugging information");
	opts.define("r|remove=b",   "Remove stems");
	opts.define("R|removeall=b","Remove all stems including explicit beams");
	opts.define("o|overwrite|replace=b","Overwrite non-explicit stems in input");
	opts.define("O|overwriteall|replaceall=b",  "Overwrite all stems in input");
	opts.define("L|no-long|not-long|not-longs=b", "Do not put stems one whole notes or breves");
	opts.define("u|up=b",       "Middle note on staff has stem up");
	opts.define("p|pos=b",     "Display only note vertical positions on staves");
	opts.define("v|voice=b",    "Display only voice/layer information");

	opts.define("author=b",     "Program author");
	opts.define("version=b",    "Program version");
	opts.define("example=b",    "Program examples");
	opts.define("h|help=b",     "Short description");
	opts.process(argc, argv);

	// handle basic options:
	if (opts.getBoolean("author")) {
		cout << "Written by Craig Stuart Sapp, "
				  << "craig@ccrma.stanford.edu, December 2010" << endl;
		exit(0);
	} else if (opts.getBoolean("version")) {
		cout << argv[0] << ", version: 26 December 2010" << endl;
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

	debugQ        = opts.getBoolean("debug");
	removeQ       = opts.getBoolean("remove");
	removeallQ    = opts.getBoolean("removeall");
	noteposQ      = opts.getBoolean("pos");
	voiceQ        = opts.getBoolean("voice");
	overwriteQ    = opts.getBoolean("overwrite");
	overwriteallQ = opts.getBoolean("overwriteall");
	notlongQ      = opts.getBoolean("no-long");
	if (opts.getBoolean("up")) {
		Middle = 4;
		Borderline = 1;
	}
	removeallQ = opts.getBoolean("removeall");
	if (removeallQ) {
		removeQ = 1;
	}
	if (overwriteallQ) {
		overwriteQ = 1;
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

void usage(const string& command) {

}



//////////////////////////////
//
// getBeamState -- Analyze structure of beams and store note layout
//      directives at the same time.
//
// Type          Humdrum     MuseData
// start          L           [
// continue                   =
// end            J           ]
// forward hook   K           /
// backward hook  k           \  x
//

void getBeamState(vector<vector<string > >& beams, HumdrumFile& infile) {
	int i, j, t;
	int ii;
	int contin;
	int start;
	int stop;
	int flagr;
	int flagl;
	int track;
	RationalNumber rn;

	vector<vector<int> > beamstate;   // state of beams in tracks/layers
	vector<vector<int> > gracestate;  // independents state for grace notes

	string gbinfo;

	beamstate.resize(infile.getMaxTracks() + 1);
	gracestate.resize(infile.getMaxTracks() + 1);
	for (i=0; i<(int)beamstate.size(); i++) {
		beamstate[i].resize(100);     // maximum of 100 layers in each track...
		gracestate[i].resize(100);    // maximum of 100 layers in each track...
		std::fill(beamstate[i].begin(), beamstate[i].end(), 0);
		std::fill(gracestate[i].begin(), gracestate[i].end(), 0);
	}

	beams.resize(infile.getNumLines());
	vector<int> curlayer;
	curlayer.resize(infile.getMaxTracks() + 1);
	vector<int> laycounter;

	for (i=0; i<infile.getNumLines(); i++) {
		gbinfo.clear();
		if (infile[i].isMeasure()) {
				// don't allow beams across barlines.  Mostly for 
				// preventing buggy beams from propagating...
				for (t=1; t<=infile.getMaxTracks(); t++) {
				std::fill(beamstate[t].begin(), beamstate[t].end(), 0);
				std::fill(gracestate[t].begin(), gracestate[t].end(), 0);
				}
		}

		if (!infile[i].isData() && !infile[i].isMeasure()) {
				continue;
		}

		if (!infile[i].isData()) {
				continue;
		}

		beams[i].resize(infile[i].getFieldCount());
		for (j=0; j<(int)beams[i].size(); j++) {
				beams[i][j].resize(1);
				beams[i][j][0] = '\0';
		}

		std::fill(curlayer.begin(), curlayer.end(), 0);
		for (j=0; j<infile[i].getFieldCount(); j++) {
				track = infile[i].getPrimaryTrack(j);
				curlayer[track]++;
				if (strcmp(infile[i][j], ".") == 0) {
				// ignore null tokens
				continue;
				}         
				if (strchr(infile[i][j], 'r') != NULL) {
				// ignore rests.  Might be useful to not ignore
				// rests if beams extent over rests...
				continue;
				}         
				rn = Convert::kernToDurationR(infile[i][j]);
				if (rn >= 1) {
				beamstate[track][curlayer[track]] = 0;
				continue;
				}
				if (rn == 0) {
				// grace notes;
				countBeamStuff(infile[i][j], start, stop, flagr, flagl);
				if ((start != 0) && (stop != 0)) {
					cerr << "Funny error in grace note beam calculation" << endl;
					exit(1);
				}
				if (start > 7) {
					cerr << "Too many beam starts" << endl;
				}
				if (stop > 7) {
					cerr << "Too many beam ends" << endl;
				}
				if (flagr > 7) {
					cerr << "Too many beam flagright" << endl;
				}
				if (flagl > 7) {
					cerr << "Too many beam flagleft" << endl;
				}
				contin = gracestate[track][curlayer[track]];
				contin -= stop;
				for (ii=0; ii<contin; ii++) { 
					gbinfo += '=';
				}
				if (start > 0) {
					for (ii=0; ii<start; ii++) {
						  gbinfo += '[';
					}
				} else if (stop > 0) {
					for (ii=0; ii<stop; ii++) {
						  gbinfo += ']';
					}
				}
				for (ii=0; ii<flagr; ii++) {
					gbinfo += '/';
				}
				for (ii=0; ii<flagl; ii++) {
					gbinfo += '\\';
				}
				if (gbinfo.size() > 6) {
					cerr << "Error too many grace note beams" << endl;
					cerr << "Size is " << gbinfo.size() << endl;
					exit(1);
				}
				beams[i][j] = gbinfo;
				gracestate[track][curlayer[track]] = contin;
				gracestate[track][curlayer[track]] += start;

				} else {
				// regular notes which are shorter than a quarter note
				// (including tuplet quarter notes which should be removed):

				countBeamStuff(infile[i][j], start, stop, flagr, flagl);
				if ((start != 0) && (stop != 0)) {
					cerr << "Funny error in note beam calculation" << endl;
					exit(1);
				}
				if (start > 7) {
					cerr << "Too many beam starts" << endl;
				}
				if (stop > 7) {
					cerr << "Too many beam ends" << endl;
				}
				if (flagr > 7) {
					cerr << "Too many beam flagright" << endl;
				}
				if (flagl > 7) {
					cerr << "Too many beam flagleft" << endl;
				}
				contin = beamstate[track][curlayer[track]];
				contin -= stop;
				for (ii=0; ii<contin; ii++) { 
					gbinfo += '=';
				}
				if (start > 0) {
					for (ii=0; ii<start; ii++) {
						gbinfo += '[';
					}
				} else if (stop > 0) {
					for (ii=0; ii<stop; ii++) {
						gbinfo += ']';
					}
				}
				for (ii=0; ii<flagr; ii++) {
					gbinfo += '/';
				}
				for (ii=0; ii<flagl; ii++) {
					gbinfo += '\\';
				}
				if (gbinfo.size() > 6) {
					cerr << "Error too many grace note beams" << endl;
					cerr << "Size is " << gbinfo.size() << endl;
					cerr << "Value is " << gbinfo << endl;
					exit(1);
				}
				beams[i][j] = gbinfo;
				beamstate[track][curlayer[track]] = contin;
				beamstate[track][curlayer[track]] += start;
				}
		}
	}
}



//////////////////////////////
//
// countBeamStuff --
//

void countBeamStuff(const string& token, int& start, int& stop, int& flagr, 
		int& flagl) {
	int i = 0;
	start = stop = flagr = flagl = 0;
	while (token[i] != '\0') {
		switch (token[i]) {
			case 'L': start++;  break;
			case 'J': stop++;   break;
			case 'K': flagr++;  break;
			case 'k': flagl++;  break;
		}
		i++;
	}
}



