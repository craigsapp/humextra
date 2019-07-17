//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Oct 24 10:39:47 PDT 1999
// Last Modified: Sat Nov 25 20:17:58 PST 2000 Added instrument selection
// Last Modified: Tue Feb 20 19:20:09 PST 2001 Add articulation interpretation
// Last Modified: Sat Aug 23 08:57:54 PDT 2003 Added *free/*strict control
// Last Modified: Wed Mar 24 19:56:04 PST 2004 Fixed initial *MM ignore
// Last Modified: Wed Apr  7 16:22:38 PDT 2004 Grace-note noteoff hack
// Last Modified: Tue Sep  7 03:31:49 PDT 2004 Added extra space at end
// Last Modified: Tue Sep  7 03:43:09 PDT 2004 More grace-note noteoff fixing
// Last Modified: Tue Sep  7 20:58:38 PDT 2004 Initial dynamics processing
// Last Modified: Fri Sep 10 02:26:59 PDT 2004 Added padding option
// Last Modified: Fri Sep 10 19:46:53 PDT 2004 Added cresc. decresc. control
// Last Modified: Sun Sep 12 05:20:44 PDT 2004 Added human and metric volume
// Last Modified: Wed Mar 23 00:35:18 PST 2005 Added constant volume back
// Last Modified: Sat Dec 17 22:46:11 PST 2005 Added **time processing
// Last Modified: Sat Jun  3 10:35:29 PST 2005 Added **tempo processing
// Last Modified: Sun Jun  4 19:32:22 PDT 2006 Added PerfViz match files
// Last Modified: Tue Sep 12 19:36:08 PDT 2006 Added **idyn processing
// Last Modified: Sun Oct  1 21:04:35 PDT 2006 Continued work in **idyn
// Last Modified: Thu May  3 22:55:15 PDT 2007 Added *pan controls
// Last Modified: Thu Oct 30 12:42:35 PST 2008 Added --no-rest option
// Last Modified: Thu Nov 20 08:19:40 PST 2008 Added **Dcent interpretation
// Last Modified: Sun Nov 23 22:49:15 PST 2008 Added --temperament option
// Last Modified: Tue May 12 12:14:09 PDT 2009 Added rhythmic scaling factor
// Last Modified: Tue Feb 22 13:23:24 PST 2011 Added --stdout
// Last Modified: Fri Feb 25 13:00:02 PST 2011 Added --met
// Last Modified: Fri Feb 25 15:15:09 PST 2011 Added --timbres and --autopan
// Last Modified: Wed Oct 12 16:23:02 PDT 2011 Fixed --temperament pc 0 prob
// Last Modified: Fri Aug  3 16:09:29 PDT 2012 Added DEFAULT for --timbres
// Last Modified: Tue Oct 16 21:02:56 PDT 2012 Added getTitle/song title
// Last Modified: Mon Nov 18 13:04:44 PST 2013 Default output as ASCII MIDI
// Last Modified: Wed Dec 11 22:24:36 PST 2013 Added !!midi-transpose:
// Last Modified: Wed Mar 30 23:12:38 PDT 2016 Added embedded options
// Last Modified: Mon May 23 21:42:33 PDT 2016 Reversed track numbers
// Filename:      ...sig/examples/all/hum2mid.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/hum2mid.cpp
// Syntax:        C++; museinfo
//
// Description:   Converts Humdrum **kern data into MIDI data in a
//                Standard MIDI File format.
//
// Todo:
//    * Check to make sure input files are not the same as the -o filename
//    * Allow multiple input / outputs
//    * Allow multiple inputs one output (already done?)
//

#include "museinfo.h"
#include "PerlRegularExpression.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

#define TICKSPERQUARTERNOTE 120

//  Dynamics to attack velocity conversions:

// Set based on conversions to MIDI files made in Finale:
// (Every 13 increments)
#define DYN_FFFF 127
#define DYN_FFF  114
#define DYN_FF   101
#define DYN_F    88
#define DYN_MF   75
#define DYN_MP   62
#define DYN_P    49
#define DYN_PP   36
#define DYN_PPP  23
#define DYN_PPPP 10

// PerfViz data structure

class PerfVizNote {
	public:
		int pitch;           // pitch name of note (in base40 notation)
		int beat;            // beat number in measure of note
		int vel;             // MIDI attack velocity of note
		int ontick;          // tick time for note on
		int offtick;         // tick time for note off
		double scoredur;     // score duration of notes in beats
		double absbeat;      // absolute beat position of note in score

		double beatfrac;     // starting fraction of beat
		double beatdur;      // duration of note in beats

		static int sid;      // serial number id
		static int bar;      // current measure number
		static int key;      // used to print key signatures
		static int tstop;    // time signature top
		static int tsbottom; // time signature bottom
		static double approxtempo;  // the approximate tempo marking

};

int PerfVizNote::sid         =  0;
int PerfVizNote::bar         =  0;
int PerfVizNote::key         = -1;
int PerfVizNote::tstop       =  0;
int PerfVizNote::tsbottom    =  0;
double PerfVizNote::approxtempo =  0;


// global variables:
string outlocation;
int   trackcount  = 0;           // number of tracks in MIDI file
int   track       = 0;           // track number starting at 0
int   Offset      = 0;           // start-time offset in ticks
int   tempo       = 60;          // default tempo
vector<int> Rtracks;
vector<int> Ktracks;

// user interface variables
Options options;
int     storeCommentQ    =   1;    // used with -C option
int     storeTextQ       =   1;    // used with -T option
int     plusQ            =   0;    // used with --plus option
int     defaultvolume    =  64;    // used with -v option
double  tscaling         =   1.0;  // used with -s option
int     multitimbreQ     =   1;    // used with -c option
int     instrumentQ      =   1;    // used with -I option
int     fixedChannel     =   0;    // used with -c option
int     instrumentnumber =  -1;    // used with -f option
int     forcedQ          =   0;    // used with -f option
int     mine             =   0;    // used with -m option
int     shortenQ         =   0;    // used with -s option
int     shortenamount    =   0;    // used with -s option
int     plainQ           =   0;    // used with -p option
int     debugQ           =   0;    // used with --debug option
int     dynamicsQ        =   1;    // used with -D option
int     padQ             =   1;    // used with -P option
int     humanvolumeQ     =   5;    // used with --hv option
int     metricvolumeQ    =   5;    // used with --mv option
int     sforzando        =  20;    // used with --sf option
int     fixedvolumeQ     =   0;    // used with -v option
int     timeQ            =   0;    // used with --time option
int     autopanQ         =   0;    // used with --autopan option
int     tempospineQ      =   0;    // used with --ts option
int     perfvizQ         =   0;    // used with --pvm option
int     idynQ            =   0;    // used with -d option
double  idynoffset       =   0.0;  // used with -d option
int     timeinsecQ       =   0;    // used with --time-in-seconds option
int     norestQ          =   0;    // used with --no-rest option
int     fillpickupQ      =   0;    // used with --fill-pickup option
int     stdoutQ          =   0;    // used with --stdout option
int     starttick        =   0;    // used with --no-rest and --fill-pickup options
int     bendQ            =   0;    // used with --bend option
int     metQ             =   0;    // used with --met option
int     met2Q            =   0;    // used with --met2 option
int     tassoQ           =   0;    // used with --tasso option
int     infoQ            =   0;    // used with --info option
int     timbresQ         =   0;    // used with --timbres option
vector<string> TimbreName;         // used with --timbres option
vector<int> TimbreValue;           // used with --timbres option
vector<int> TimbreVolume;          // used with --timbres option
vector<int> VolumeMapping;         // used with --timbres option
double  bendamt          = 200;    // used with --bend option
int     bendpcQ          =   0;    // used with --temperament option
double  bendbypc[12]     = {0};    // used with --temperament and --monotune
int     monotuneQ        =   0;    // used with --monotune option
int     MidiTranspose    = 0;      // used with --transpose
double  rhysc            = 1.0;    // used with -r option
	// for example, use -r 4.0 to make written sixteenth notes
	// appear as if they were quarter notes in the MIDI file.

stringstream *PVIZ = NULL;    // for storing individual notes in PerfViz data file.
double   tickfactor = 960.0 / 1000.0;

// function declarations:
void      assignTracks      (HumdrumFile& infile, vector<int>& trackchannel);
double    checkForTempo     (HumdrumRecord& record);
void      checkOptions      (Options& opts, int argc, char** argv);
void      example           (void);
int       makeVLV           (uchar *buffer, int number);
void      reviseInstrumentMidiNumbers(const char* string);
int       setMidiPlusVolume (const char* kernnote);
void      storeMetaText     (smf::MidiFile& mfile, int track, const string& string,
                             int tick, int metaType = 1);
void      storeMidiData     (HumdrumFile& infile, smf::MidiFile& outfile);
void      storeInstrument   (int ontick, smf::MidiFile& mfile, HumdrumFile& infile,
                             int line, int row, int pcQ);
void      usage             (const char* command);
void      storeFreeNote     (vector<vector<int> >& array,int ptrack,int midinote);
void      getDynamics       (HumdrumFile& infile, vector<string>& dynamics,
                             int defaultdynamic);
void      getDynamicAssignments(HumdrumFile& infile, vector<int>& assignments);
void      getStaffValues    (HumdrumFile& infile, int staffline,
                             vector<vector<int> >& staffvalues);
void      getNewDynamics    (vector<int>& currentdynamic,
                             vector<int>& assignments,
                             HumdrumFile& infile, int line,
                             vector<string>& crescendos,
                             vector<string>& accentuation);
void      processCrescDecresc(HumdrumFile& infile,
                             vector<string>& dynamics,
                             vector<string>& crescendos);
void      interpolateDynamics(HumdrumFile& infile, string& dyn,
                             string& cresc);
void      generateInterpolation(HumdrumFile& infile, string& dyn,
                             string& cresc, int startline, int stopline,
                             int direction);
int       findtermination    (string& dyn, string& cresc, int start);
char      adjustVolumeHuman  (int startvol, int delta);
char      adjustVolumeMetric (int startvol, int delta, double metricpos);
char      applyAccentuation  (int dynamic, int accent);
int       getMillisecondTime (HumdrumFile& infile, int line);
int       getFileDurationInMilliseconds(HumdrumFile& infile);
int       getMillisecondDuration(HumdrumFile& infile, int row, int col,
                             int subcol);
void      addTempoTrack      (HumdrumFile& infile, smf::MidiFile& outfile);
void      getBendByPcData    (double* bendbypc, const string& filename);
void      insertBendData     (smf::MidiFile& outfile, double* bendbypc);
void      getKernTracks      (vector<int>& tracks, HumdrumFile& infile);
void      getTitle           (string& title, HumdrumFile& infile);
void      addMonoTemperamentAdjustment(smf::MidiFile& outfile, int track,
                              int channel, int ticktime, int midinote,
                              double* bendbypc);
void      defineOptions       (Options& opts, int argc, char* argv[]);
void      processOptions      (Options& opts, int argc, char* argv[]);
void      checkEmbeddedOptions(HumdrumFile& infile, int argc, char* argv[]);
void      checkForTimeSignature(smf::MidiFile& outfile, HumdrumFile& infile,
                               int line);
void      checkForKeySignature(smf::MidiFile& outfile, HumdrumFile& infile,
                               int line);
string    getInstrumentName   (HumdrumFile& infile, int ptrack);
vector<int> getGraceNoteState(HumdrumFile& infile);

// PerfViz related functions:
void      writePerfVizMatchFile(const string& filename, stringstream& contents);
ostream& operator<<            (ostream& out, PerfVizNote& note);
void     printPerfVizKey       (int key);
void     printPerfVizTimeSig   (int tstop, int tsbottom);
void     printPerfVizTempo     (double approxtempo);
void     printRational          (ostream& out, double value);
void     storePan              (int ontime, smf::MidiFile& outfile,
                                HumdrumFile& infile, int row, int column);
void     adjustEventTimes      (smf::MidiFile& outfile, int starttick);
void     checkForBend          (smf::MidiFile& outfile, int notetick, int channel,
                                HumdrumFile& infile, int row, int col,
                                double scalefactor);
void     storeTimbres          (vector<string>& name, vector<int>& value,
                                vector<int>& volumes, const string& string);
void     autoPan               (smf::MidiFile& outfile, HumdrumFile& infile);

vector<int> tracknamed;      // for storing boolean if track is named
vector<int> trackchannel;    // channel of each track

#define TICKSPERQUARTERNOTE 120
int tpq = TICKSPERQUARTERNOTE;

//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
	VolumeMapping.resize(0);

	// for humanizing processes
	#ifndef VISUAL
		srand48(time(NULL));
	# else
		srand(time(NULL));
	#endif

	stringstream *perfviz = NULL;

	HumdrumFile infile;
	smf::MidiFile    outfile;

	// process the command-line options
	checkOptions(options, argc, argv);

	if (perfvizQ) {
		tpq   = 480;
		tempo = 120;
	}
	outfile.setTicksPerQuarterNote(tpq);

	if (timeQ) {
		outfile.setMillisecondTicks();
	}

	// figure out the number of input files to process
	// only the first argument will be processed.  If there are
	// no arguments, then standard input will be used.
	int numinputs = options.getArgCount();

	for (int i=0; i<1 || i==0; i++) {
		infile.clear();

		// if no command-line arguments read data file from standard input
		if (numinputs < 1) {
			infile.read(cin);
		} else {
			infile.read(options.getArg(i+1));
		}

		checkEmbeddedOptions(infile, argc, argv);

		// analyze the input file according to command-line options
		infile.analyzeRhythm("4", debugQ);

		infile.getKernTracks(Ktracks);

		reverse(Ktracks.begin(), Ktracks.end());

		Rtracks.resize(infile.getMaxTracks() + 1);
		fill(Rtracks.begin(), Rtracks.end(), -1);
		for (i=0; i<(int)Ktracks.size(); i++) {
			Rtracks[Ktracks[i]] = i + 1;
		}

		if (perfvizQ) {
			perfviz = new stringstream[1];
			string filename;
			PVIZ = perfviz;
			perfviz[0] << "info(matchFileVersion,2.0).\n";
			if (numinputs < 1) {
				perfviz[0] << "info(scoreFileName,'STDIN').\n";
			} else {
				perfviz[0] << "info(scoreFileName,'";
				filename = strrchr(options.getArg(i+1).c_str(), '/');
				if (filename == "") {
					filename = options.getArg(i+1);
				} else {
					filename.erase(0, 1);
				}
				perfviz[0] << filename;
				perfviz[0] << "').\n";
			}
			if (options.getBoolean("output")) {
				perfviz[0] << "info(midiFileName,'";
				filename = strrchr(options.getString("output").c_str(), '/');
				if (filename == "") {
					filename = options.getString("output");
				} else {
					filename.erase(0, 1);
				}
				perfviz[0] << filename;
				perfviz[0] << "').\n";
			} else {
				perfviz[0] << "info(midifileName,'STDOUT').\n";
			}
			perfviz[0] << "info(midiClockUnits,";
			perfviz[0] << tpq << ").\n";
			perfviz[0] << "info(midiClockRate,500000).\n";
		}

		tracknamed.resize(Ktracks.size() + 1);
		trackchannel.resize(Ktracks.size() + 1);
		for (int j=0; j<(int)trackchannel.size(); j++) {
			tracknamed[j]   = 0;
			trackchannel[j] = fixedChannel;
		}
		if (multitimbreQ) {
			assignTracks(infile, trackchannel);
		}
		outfile.addTrack(Ktracks.size());
		outfile.absoluteTicks();

		/*
		// removed this code because of all of the lousy free MIDI
		// sequencing programs that choke on system exclusive messages.

		// store the "General MIDI activation" system exclusive:
		// don't bother if this is just a Type 0 MIDI file
		if (!options.getBoolean("type0")) {
			vector<uchar> gmsysex;
			gmsysex.resize(6);
			gmsysex[0] = 0xf0;     // Start of SysEx
			gmsysex[1] = 0x7e;     // Universal (reserved) ID number
			gmsysex[2] = 0x7f;     // Device ID (general transmission)
			gmsysex[3] = 0x09;     // Means 'This is a message about General MIDI'
			gmsysex[4] = 0x01;     // Means 'Turn General MIDI On'. 02 means 'Off'
			gmsysex[5] = 0xf7;     // End of SysEx
			outfile.addEvent(0, 0, gmsysex);
		}
		*/

		if (bendpcQ) {
			insertBendData(outfile, bendbypc);
		}

		storeMidiData(infile, outfile);

		if (tempospineQ) {
			addTempoTrack(infile, outfile);
		}

		outfile.sortTracks();
		if (norestQ) {
			adjustEventTimes(outfile, starttick);
		}
		if (fillpickupQ) {
			RationalNumber pup = 0;
			for (int i=0; i<infile.getNumLines(); i++) {
				if (!infile[i].isData()) {
					continue;
				}
				pup = infile.getBeatR(i);
				break;
			}
			if (pup > 0) {
				pup *= tpq;
				starttick = int(pup.getFloat() + 0.5);
				adjustEventTimes(outfile, starttick);
			}
		}
		if (stdoutQ) {
			outfile.write(cout);
		} else if (outlocation == "") {
			// outfile.printHex(cout);
			cout << outfile;
		} else if (infoQ) {
			cout << outfile;
		} else {
			outfile.write(outlocation);
		}

		if (perfvizQ) {
			// currently you cannot create multiple PerfViz files from
			// multiple inputs.
			writePerfVizMatchFile(options.getString("perfviz").c_str(), perfviz[0]);
			delete [] perfviz;
		}
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////



//////////////////////////////
//
// checkEmbeddedOptions --
//

void checkEmbeddedOptions(HumdrumFile& infile, int argc, char* argv[]) {
	vector<int> oline;;
	int i;
	PerlRegularExpression pre;
	string ostring;

	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isGlobalComment()) {
			continue;
		}
		if (pre.search(infile[i][0], "^\\!\\!hum2mid:\\s*(.*)\\s*$")) {
			oline.push_back(i);
		}
	}

	if (oline.size() == 0) {
		return;
	}

	Options localoptions;
	defineOptions(localoptions, argc, argv);

	for (i=0; i<(int)oline.size(); i++) {
		if (pre.search(infile[oline[i]][0], "^\\!\\!hum2mid:\\s*(.*)\\s*$")) {
			ostring = pre.getSubmatch(1);
			localoptions.appendOptions(ostring);
		}
	}

	localoptions.process();

	processOptions(localoptions, argc, argv);
}



//////////////////////////////
//
// insertBendData -- store pitch bend data, one channel per
//      pitch class.  Channel 10 is skipped because it is used
//      as a drum channel on General MIDI synthesizers.
//

void insertBendData(smf::MidiFile& outfile, double* bendbypc) {
	int i, j;
	int channel;
	// int track = 0;
	double bendvalue;
	if (outfile.getNumTracks() > 1) {
		// don't store in first track if a multi-track file.
		// track = 1;
	}
	vector<uchar> mididata;
	mididata.resize(2);
	if (instrumentnumber < 0) {
		mididata[1] = 0;
	} else {
		mididata[1] = instrumentnumber;
	}
	for (i=0; i<12; i++) {
		channel = i;
		if (channel >= 9) {
			channel++;
		}
		bendvalue = bendbypc[i];
		// outfile.addPitchBend(track, offset, channel, bendvalue);
		if (outfile.getNumTracks() == 0) {
			outfile.addPitchBend(0, Offset, channel, bendvalue);
		} else {
			for (j=1; j<outfile.getNumTracks(); j++) {
				outfile.addPitchBend(j, Offset, channel, bendvalue);
			}
		}
		if (forcedQ) {
			mididata[0] = 0xc0 | (0x0f & channel);
			// outfile.addEvent(track, offset, mididata);
			if (outfile.getNumTracks() == 0) {
				outfile.addEvent(0, Offset, mididata);
			} else {
				for (j=1; j<outfile.getNumTracks(); j++) {
					outfile.addEvent(j, Offset, mididata);
				}
			}
		}
	}
}



//////////////////////////////
//
// addMonoTemperamentAdjustment -- store a pitch bend message just before
//    an note is tured on in a
//

void addMonoTemperamentAdjustment(smf::MidiFile& outfile, int track, int channel,
	int ticktime, int midinote, double* bendbypc) {
	double bendvalue = bendbypc[midinote % 12];
	outfile.addPitchBend(track, ticktime, channel, bendvalue);
}



//////////////////////////////
//
// adjustEventTimes -- set the first event time to starttick and
//    also subtract that value from all events in the MIDI file;
//

void adjustEventTimes(smf::MidiFile& outfile, int starttick) {
	int i, j;
	smf::MidiEvent* eventptr;
	int atick;
	int minval = 1000000000;
	for (i=0; i<outfile.getTrackCount(); i++) {
		for (j=0; j<outfile.getNumEvents(i); j++) {
			eventptr = &outfile.getEvent(i, j);
			if (eventptr->size() <= 0) {
				continue;
			}
			if (((*eventptr)[0] & 0xf0) == 0x90) {
				if (eventptr->tick < minval) {
					minval = eventptr->tick;
				}
				break;
			}
		}
	}

	if (minval > 1000000) {
	// minimum time is ridiculously large, so don't do anything
	return;
	}

	for (i=0; i<outfile.getTrackCount(); i++) {
		for (j=0; j<outfile.getNumEvents(i); j++) {
			eventptr = &outfile.getEvent(i, j);
			atick = eventptr->tick;
			atick = atick - minval;
			if (atick < 0) {
				atick = 0;
			}
			eventptr->tick = atick + starttick;
		}
	}
}



//////////////////////////////
//
// addTempoTrack --
//

void addTempoTrack(HumdrumFile& infile, smf::MidiFile& outfile) {
	int i, j;
	double tempo;
	double absbeat;
	int ontick;
	int ttempo;
	vector<uchar> mididata;

	// should erase previous contents of tempo track first...

	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].getType() == E_humrec_data) {
			for (j=0; j<infile[i].getFieldCount(); j++) {
				if (strcmp(infile[i].getExInterp(j), "**tempo") == 0) {
					if (std::isdigit(infile[i][j][0])) {
						tempo = strtod(infile[i][j], NULL);
						//cout << "The tempo value read was " << tempo << endl;
						tempo = tempo * tscaling;
						mididata.resize(6);
						mididata[0] = 0xff;
						mididata[1] = 0x51;
						mididata[2] = 3;
						ttempo = (int)(60000000.0 / tempo + 0.5);
						mididata[3] = (uchar)((ttempo >> 16) & 0xff);
						mididata[4] = (uchar)((ttempo >> 8) & 0xff);
						mididata[5] = (uchar)(ttempo & 0xff);
						absbeat = infile.getAbsBeat(i);
						ontick = int(absbeat * outfile.getTicksPerQuarterNote());
						outfile.addEvent(0, ontick + Offset, mididata);
					}
				}
			}
		}
	}

}



//////////////////////////////
//
// assignTracks -- give a track number for each **kern spine in the input file
//    trying to place the same instruments into the same channels if
//    there are not enough channels to go around.
//

void assignTracks(HumdrumFile& infile, vector<int>& trackchannel) {
	int i, j;

	vector<int> instruments;        // MIDI instruments indicated in track
	// trackchannel.resize(infile.getMaxTracks() + 1);
	trackchannel.resize(Ktracks.size() + 1);
	instruments.resize(trackchannel.size());
	for (i=0; i<(int)instruments.size(); i++) {
		if (forcedQ) {
			instruments[i] = instrumentnumber;
		} else {
			instruments[i] = -1;
		}
		trackchannel[i] = 0;
	}
	HumdrumInstrument x;
	int inst = -1;
	int ptrack = 0;
	int instcount = 0;
	PerlRegularExpression pre2;

	// VolumeMapping.resize(infile.getMaxTracks()+1);
	VolumeMapping.resize(Ktracks.size() + 1);
	fill(VolumeMapping.begin(), VolumeMapping.end(), 64);
	if (idynQ) {
		VolumeMapping.resize(0);
	}

	for (i=0; i<infile.getNumLines(); i++) {
		if (debugQ) {
			cout << "line " << i+1 << ":\t" << infile[i] << endl;
		}

		if (infile[i].isData()) {
			break;
		}
		if (!infile[i].isInterpretation()) {
			continue;
		}

		for (j=0; j<infile[i].getFieldCount(); j++) {
			track = Rtracks[infile[i].getPrimaryTrack(j)];
			if (strncmp(infile[i][j], "*I", 2) == 0) {
				if (timbresQ) {
					PerlRegularExpression pre;
					if (!pre.search(infile[i][j], "^\\*I\"\\s*(.*)\\s*", "")) {
						inst = -1;
					} else {
						string targetname;
						targetname = pre.getSubmatch(1);
						int i;
						inst = -1;

						for (i=0; i<(int)TimbreName.size(); i++) {
							if (pre2.search(targetname, TimbreName[i], "i")) {
							// if (TimbreName[i] == targetname) {
								inst = TimbreValue[i];
								if (track < (int)VolumeMapping.size()) {
									VolumeMapping[track] = TimbreVolume[i];
								}
								break;
							}
						}

						if (inst == -1) {
							// search for default timbre setting if not otherwise
							// found.
							for (i=0; i<(int)TimbreName.size(); i++) {
								if (pre2.search("DEFAULT", TimbreName[i], "i")) {
								// if (TimbreName[i] == targetname) {
									inst = TimbreValue[i];
									if (track < (int)VolumeMapping.size()) {
										VolumeMapping[track] = TimbreVolume[i];
									}
									break;
								}
							}
						}

					}

				} else if (!forcedQ) {
					inst = x.getGM(infile[i][j]);
				} else {
					inst = instrumentnumber;
				}

				if (inst != -1) {
					ptrack = Rtracks[infile[i].getPrimaryTrack(j)];
					if (instruments[ptrack] == -1) {
						if (!forcedQ) {
							instruments[ptrack] = inst;
						} else {
							instruments[ptrack] = instrumentnumber;
						}
						instcount++;
					}
				}
			}
		}
	}

	int nextChannel = 0;        // next midi channel available to assign

	if (instcount < 14) {
		// have enough space to store each instrument on a separate channel
		// regardless of instrumentation
		nextChannel = 1;        // channel 0 is for undefined instrument spines
		for (i=0; i<(int)instruments.size(); i++) {
			if (instruments[i] == -1) {
				trackchannel[i] = 0;
			} else {
				if (nextChannel == 9) {
					// avoid percussion channel
					nextChannel++;
				}
				trackchannel[i] = nextChannel++;
			}

			// avoid General MIDI percussion track.
			if (nextChannel == 9) {
				nextChannel++;
			}
		}
	} else {
		// need to conserve channels:  place duplicate instrument tracks on the
		// same channel.
		nextChannel = 1;     // channel 0 is for undefined instrument spines
		int foundDup = -1;
		for (i=0; i<(int)instruments.size(); i++) {
			foundDup = -1;
			for (j=0; j<i; j++) {
				if (instruments[j] == instruments[i]) {
					foundDup = j;
				}
			}
			if (instruments[i] == -1) {
				trackchannel[i] = 0;
			} else if (foundDup != -1) {
				trackchannel[i] = trackchannel[foundDup];
			} else {
				if (nextChannel == 10) {
					// avoid percussion channel
					nextChannel++;
				}
				trackchannel[i] = nextChannel++;
			}

			// avoid General MIDI percussion track.
			if (nextChannel == 10) {
				nextChannel++;
			}
		}

		if (nextChannel > 16) {
			// channel allocation is over quota: squash over-allocations:
			for (i=0; i<(int)trackchannel.size(); i++) {
				if (trackchannel[i] > 15) {
					trackchannel[i] = 0;
				}
			}
		}

	}

	// don't conserve tracks if there is enough to go around
	if (Ktracks.size() < 13) {
		if (trackchannel.size() > 0) {
			trackchannel[0] = 0;
			for (i=1; i<(int)Ktracks.size(); i++) {
				trackchannel[i] = i-1;
				if (i>=10) {   // avoid general midi drum track channel
					trackchannel[i] = i;
				}
			}
		}
	}

}



//////////////////////////////
//
// checkForTempo --
//

double checkForTempo(HumdrumRecord& record) {
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
		// C  = 132 qpm
		// C| = 176 qpm

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
		if (pre.search(record[0], "^\\*M4/2$")) {
			return 132.0;
		} else if (pre.search(record[0], "^\\*M2/1$")) {
			return 176.0;
		}
	} else if (metQ) {

		// mensural tempo scalings
		// O           = 58 qpm
		// O.          = 58 qpm
		// C.          = 58 qpm
		// C           = 58 qpm
		// C|          = 72 qpm
		// O2          = 75 qpm
		// C2          = 75 qpm
		// O|          = 76 qpm
		// C|3, 3, 3/2 = 110 qpm
		// C2/3        = 1.5 * 72 = 108 qpm
		// C3          = 110 qpm
		// O3/2        = 58 * 1.5 = 87 qpm
		// O/3         = 110 qpm
		// C|2, Cr     = 144 qpm (previously 220 qpm but too fast)

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
				return (double)metQ * 1.25;
			}
			return (double)metQ * 1.333;
		} else if (strcmp(mensuration, "C|3") == 0) {
			return (double)metQ * 1.8965517;
		} else if (strcmp(mensuration, "C3") == 0) {
			return (double)metQ * 1.25;
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
			return (double)metQ * 2.5;
		} else if (strcmp(mensuration, "Cr") == 0) {
			return (double)metQ * 2.5;
		}
	}

	return -1.0;
}



//////////////////////////////
//
// checkOptions --
//

void checkOptions(Options& opts, int argc, char* argv[]) {
	defineOptions(opts, argc, argv);
	processOptions(opts, argc, argv);
}


void defineOptions(Options& opts, int argc, char* argv[]) {
	opts.define("C|nocomments=b",  "Do not store comments as meta text");
	opts.define("D|nodynamics=b",  "Do not encode dynamics found in file");
	opts.define("showdynamics=b",  "Show the calculated dynamics by input line");
	opts.define("n|note|comment=s","Store a comment line in the file");
	opts.define("T|notext=b",      "Do not non-musical data as meta text");
	opts.define("o|output=s:midi.mid", "Output midifile");
	opts.define("0|O|type0|zero=b","Generate a type 0 MIDI file");
	opts.define("plus=b",          "Create a MIDIPlus compliant MIDI file");
	opts.define("time=b",          "Use timing from a **time spine");
	opts.define("sec|time-in-seconds=b", "Use timing from a **time spine");
	opts.define("v|volume=i:64",   "Default attack velocity");
	opts.define("d|dyn|idyn=d:40.0","Extract attack velocities from **idyn");
	opts.define("t|tempo-scaling=d:1.0", "Tempo scaling");
	opts.define("transpose=i:0", "Internal transposition");
	opts.define("ts|tempo|tempo-spine=b", "Use tempo markings from tempo spine");
	opts.define("I|noinstrument=b", "Do not store MIDI instrument programs");
	opts.define("i|instruments=s", "Specify MIDI conversions for instruments");
	opts.define("f|forceinstrument=i:0", "MIDI instrument for all tracks");
	opts.define("c|channel=i:-1",   "Channel to store for MIDI data");
	opts.define("m|min=i:0",        "Minimum tick duration of notes");
	opts.define("r|rhythmic-scaling=d:1.0", "Scale all score durations");
	opts.define("s|shorten=i:0",    "Shortening tick value for note durations");
	opts.define("p|plain=b",        "Play with articulation interpretation");
	opts.define("P|nopad=b",        "Do not pad ending with spacer note");
	opts.define("tasso=b",          "Default tempo by omet mensuration for Tasso music");
	opts.define("met=d:232",        "Tempo control from metrical symbols");
	opts.define("met2=d:336",       "Tempo control from metrical symbols, older era");
	opts.define("no-met=b",         "Do not use --met option");
	opts.define("no-met2=b",        "Do not use --met2 option");
	opts.define("hv|humanvolume=i:5","Apply a random adjustment to volumes");
	opts.define("mv|metricvolume=i:3","Apply metric accentuation to volumes");
	opts.define("fs|sforzando=i:20","Increase sforzandos by specified amount");
	opts.define("no-rest=b",      "Do not put rests at start of midi");
	opts.define("fp|fill-pickup|fill-pickups=b", "Fill in rests to make pickup bars complete");
	opts.define("pvm|perfviz=s:", "Create a PerfViz match file for MIDI output");
	opts.define("debug=b",        "Debugging turned on");
	opts.define("stdout=b",       "Print MIDI file to standard output");
	opts.define("mark=b",         "Handle marked notes somehow");
	opts.define("info=b",         "Display as quasi ASCII MIDI");
	opts.define("bend=d:200.0",   "Turn on pitch-bend with given half-depth");
	opts.define("temperament|tune=s:", "Turn on pitch-bend with given data file");
	opts.define("monotune=s:", "Turn on pitch-bend tuning for monophonic tracks");
	opts.define("timbres=s",      "Timbral assignments by instrument name");
	opts.define("autopan=b",      "Pan tracks from left to right");

	opts.define("author=b",  "author of program");
	opts.define("version=b", "compilation info");
	opts.define("example=b", "example usages");
	opts.define("h|help=b",  "short description");
	opts.process(argc, argv);
}



void processOptions(Options& opts, int argc, char* argv[]) {

	// handle basic options:
	if (opts.getBoolean("author")) {
		cout << "Written by Craig Stuart Sapp, "
			<< "craig@ccrma.stanford.edu, May 1998" << endl;
		exit(0);
	} else if (opts.getBoolean("version")) {
		cout << argv[0] << ", version: April 2002" << endl;
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

	if (opts.getBoolean("instruments")) {
		reviseInstrumentMidiNumbers(opts.getString("instruments").c_str());
	}

	if (opts.getBoolean("nocomments")) {
		storeCommentQ = 0;
	} else {
		storeCommentQ = 1;
	}

	if (opts.getBoolean("tasso")) {
		tassoQ = 1;
	}

	if (opts.getBoolean("met") && !opts.getBoolean("no-met")) {
		metQ = int(opts.getDouble("met")+0.5);
	} else {
		metQ = 0;
	}

	if (opts.getBoolean("met2") && !opts.getBoolean("no-met2")) {
		met2Q = int(opts.getDouble("met2")+0.5);
		metQ  = met2Q;
	} else {
		met2Q = 0;
	}

	storeTextQ = opts.getBoolean("notext");
	plusQ      = opts.getBoolean("plus");
	plainQ     = opts.getBoolean("plain");

	if (opts.getBoolean("nodynamics")) {
		dynamicsQ = 0;
	} else {
		dynamicsQ = 1;
		if (opts.getBoolean("showdynamics")) {
			dynamicsQ += 1;
		}
	}

	defaultvolume = opts.getInt("volume");
	if (defaultvolume < 1) {
		defaultvolume = 1;
	} else if (defaultvolume > 127) {
		defaultvolume = 127;
	}

	tscaling = opts.getDouble("tempo-scaling");
	if (tscaling < 0.001) {
		tscaling = 1.0;
	} else if (tscaling > 1000.0) {
		tscaling = 1.0;
	}
	// tscaling = 1.0 / tscaling;
	instrumentQ = !opts.getBoolean("noinstrument");

	if (opts.getBoolean("channel")) {
		multitimbreQ = 0;
		fixedChannel = opts.getInteger("channel") - 1;
		if (fixedChannel < 0) {
			fixedChannel = 0;
		}
		if (fixedChannel > 15) {
			fixedChannel = 15;
		}
		instrumentQ = 0;
	} else {
		multitimbreQ  = 1;
		fixedChannel = -1;
	}

	if (opts.getBoolean("output")) {
		outlocation = opts.getString("output");
	}

	forcedQ = opts.getBoolean("forceinstrument");
	if (forcedQ) {
		instrumentnumber = opts.getInteger("forceinstrument");
		if (instrumentnumber < 0 || instrumentnumber > 127) {
			instrumentnumber = 0;
		}
	} else {
		instrumentnumber = -1;
	}

	mine = opts.getInteger("min");
	if (mine < 0) {
		mine = 0;
	}
	debugQ        =  opts.getBoolean("debug");
	stdoutQ       =  opts.getBoolean("stdout");
	shortenQ      =  opts.getBoolean("shorten");
	shortenamount =  opts.getInteger("shorten");
	padQ          = !opts.getBoolean("nopad");
	humanvolumeQ  =  opts.getInteger("humanvolume");
	fixedvolumeQ  =  opts.getBoolean("volume");
	timeQ         =  opts.getBoolean("time");
	timeinsecQ    =  opts.getBoolean("time-in-seconds");
	tempospineQ   =  opts.getBoolean("tempo-spine");
	perfvizQ      =  opts.getBoolean("perfviz");
	metricvolumeQ =  opts.getInteger("metricvolume");
	sforzando     =  opts.getInteger("sforzando");
	idynQ         =  opts.getBoolean("dyn");
	idynoffset    =  opts.getDouble("dyn");
	norestQ       =  opts.getBoolean("no-rest");
	fillpickupQ   =  opts.getBoolean("fill-pickup");
	autopanQ      =  opts.getBoolean("autopan");
	bendQ         =  opts.getBoolean("bend");
	infoQ         =  opts.getBoolean("info");
	rhysc         = opts.getDouble("rhythmic-scaling");
	if (bendQ) {
		bendamt    =  opts.getDouble("bend");
		if (bendamt <= 0.0) {
			bendamt = 200.0;
		}
	}
	bendpcQ       =  opts.getBoolean("temperament");
	monotuneQ     =  opts.getBoolean("monotune");
	if (bendpcQ) {
		bendQ = 0;   // disable other type of bending (but keep bendamt)
		forcedQ = 1; // force a timber setting for all channels (piano default)
		getBendByPcData(bendbypc, opts.getString("temperament").c_str());
		// for different method, see: http://www.xs4all.nl/~huygensf/scala
	} else if (monotuneQ) {
		bendQ = 0;
		getBendByPcData(bendbypc, opts.getString("monotune").c_str());
	}

	timbresQ = opts.getBoolean("timbres");
	if (timbresQ) {
		storeTimbres(TimbreName, TimbreValue, TimbreVolume,
				opts.getString("timbres"));
	} else {
		TimbreName.resize(0);
		TimbreValue.resize(0);
	}

	if (opts.getBoolean("transpose")) {
		MidiTranspose = opts.getInteger("transpose");
	}
}



//////////////////////////////
//
// getBendByPcData --
//

void getBendByPcData(double* bendbypc, const string& filename) {
	int i, j;
	for (i=0; i<12; i++) {
		bendbypc[i] = 0.0;
	}

	HumdrumFile temperamentdata;
	temperamentdata.read(filename);
	HumdrumFile& td = temperamentdata;

	int pc;
	double dcent;
	for (i=0; i<td.getNumLines(); i++) {
		if (td[i].getType() != E_humrec_data) {
			continue;
		}
		pc = -1;
		dcent = 0;
		for (j=0; j<td[i].getFieldCount(); j++) {
			if (strcmp(td[i].getExInterp(j), "**kern") == 0) {
				if (strcmp(td[i][j], ".") == 0) {
					// ignore null tokens
					continue;
				}
				if (strchr(td[i][j], 'r') != NULL) {
					// ignore rests
					continue;
				}
				pc = Convert::kernToMidiNoteNumber(td[i][j]) % 12;
			} else if (strcmp(td[i].getExInterp(j), "**Dcent") == 0) {
				if (strcmp(td[i][j], ".") == 0) {
					// determine value of null tokens
					sscanf(td.getDotValue(i,j), "%lf", &dcent);
				} else {
					sscanf(td[i][j], "%lf", &dcent);
				}
			}
		}
		if (pc >= 0 && pc < 12) {
			bendbypc[pc] = dcent / bendamt;
		}
	}
}



//////////////////////////////
//
// example --
//

void example(void) {

}



//////////////////////////////
//
// storeMetaText --
//

void storeMetaText(smf::MidiFile& mfile, int track, const string& text, int tick,
		int metaType) {
	int i;
	int length = text.size();
	vector<uchar> metadata;
	uchar size[23] = {0};
	int lengthsize =  makeVLV(size, length);

	metadata.resize(2+lengthsize+length);
	metadata[0] = 0xff;
	metadata[1] = metaType;
	for (i=0; i<lengthsize; i++) {
		metadata[2+i] = size[i];
	}
	for (i=0; i<length; i++) {
		metadata[2+lengthsize+i] = text[i];
	}

	mfile.addEvent(track, tick + Offset, metadata);
}



//////////////////////////////
//
// makeVLV --
//

int makeVLV(uchar *buffer, int number) {

	unsigned long value = (unsigned long)number;

	if (value >= (1 << 28)) {
		cout << "Error: number too large to handle" << endl;
		exit(1);
	}

	buffer[0] = (value >> 21) & 0x7f;
	buffer[1] = (value >> 14) & 0x7f;
	buffer[2] = (value >>  7) & 0x7f;
	buffer[3] = (value >>  0) & 0x7f;

	int i;
	int flag = 0;
	int length = -1;
	for (i=0; i<3; i++) {
		if (buffer[i] != 0) {
			flag = 1;
		}
		if (flag) {
			buffer[i] |= 0x80;
		}
		if (length == -1 && buffer[i] >= 0x80) {
			length = 4-i;
		}
	}

	if (length == -1) {
		length = 1;
	}

	if (length < 4) {
		for (i=0; i<length; i++) {
			buffer[i] = buffer[4-length+i];
		}
	}

	return length;
}



//////////////////////////////
//
// getIdynDynamics -- extracts **idyn amplitude values for notes.
//

void getIdynDynamics(HumdrumFile& infile, vector<string>& dynamics,
		double idynoffset) {
	int i, j, k;
	int intdyn;
	char cdyn;
	double dyn;
	int notecount;
	char buffer[1024] = {0};
	dynamics.resize(infile.getNumLines());
	for (i=0; i<(int)dynamics.size(); i++) {
		dynamics[i].resize(8);
		dynamics[i].resize(0);
	}

	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].getType() != E_humrec_data) {
			continue;
		}
		for (j=0; j<infile[i].getFieldCount(); j++) {
			if (strcmp(infile[i].getExInterp(j), "**idyn") != 0)  {
				continue;
			}
			if (strcmp(infile[i][j], ".") == 0) {
				continue;
			}

			notecount = infile[i].getTokenCount(j);
			for (k=0; k<notecount; k++) {
				dyn = 0.0;
				infile[i].getToken(buffer, j, k);
				if (sscanf(buffer, "%lf", &dyn)) {
					dyn += idynoffset;
					// dyn *= 2.0;
				}
				intdyn = int(dyn + 0.5);
				if (intdyn < 1) {
					intdyn = 1;
				}
				if (intdyn > 127) {
					intdyn = 127;
				}
				cdyn = char(intdyn);
				dynamics[i].push_back(cdyn);
			}
		}
	}


	if (debugQ) {
		for (i=0; i<(int)dynamics.size(); i++) {
			cout << i << ":";
			for (j=0; j<(int)dynamics[i].size(); j++) {
				cout << "\t" << (int)dynamics[i][j];
			}
			cout << "\n";
		}
	}

}



//////////////////////////////
//
// storeMidiData --
//

void storeMidiData(HumdrumFile& infile, smf::MidiFile& outfile) {
	double duration = 0.0;
	int midinote = 0;
	int base40note = 0;
	double absbeat = 0.0;
	int ontick = 0;
	int idyncounter = 0;   // used for individual note volumes
	int offtick = 0;
	vector<uchar> mididata;
	int i, j, k;
	int ii;
	int tokencount = 0;
	char buffer1[1024] = {0};
	double pvscoredur = 0.0;
	int staccatoQ = 0;
	int accentQ = 0;
	int sforzandoQ = 0;
	int volume = defaultvolume;
	int ttempo;
	vector<int> freeQ;
	freeQ.resize(infile.getMaxTracks());
	fill(freeQ.begin(), freeQ.end(), 0);
	vector<vector<int> > freenotestate;
	freenotestate.resize(freeQ.size());
	for (i=0; i<(int)freenotestate.size(); i++) {
		freenotestate[i].resize(0);
		// freenotestate[i].allowGrowth();
	}
	int ptrack = 0;

	vector<string> dynamics;
	if (idynQ) {
		getIdynDynamics(infile, dynamics, idynoffset);
	} else {
		getDynamics(infile, dynamics, defaultvolume);
	}

	// store a default tempo marking if the tempo scaling option
	// was used.
	if (tscaling != 1.0) {
		ttempo = (int)(100 * tscaling);
		if (ttempo > 0) {
			mididata.resize(6);
			mididata[0] = 0xff;
			mididata[1] = 0x51;
			mididata[2] = 3;
			ttempo = (int)(60000000.0 / ttempo + 0.5);
			mididata[3] = (uchar)((ttempo >> 16) & 0xff);
			mididata[4] = (uchar)((ttempo >> 8) & 0xff);
			mididata[5] = (uchar)(ttempo & 0xff);
			outfile.addEvent(0, 0 + Offset, mididata);
		}
	}

	if (options.getBoolean("comment")) {
		storeMetaText(outfile, 0, options.getString("comment"), 0);
	}

	if (perfvizQ) {
		// set the tempo to MM 120.0 at tick time 0.
		mididata.resize(6);
		mididata[0] = 0xff;
		mididata[1] = 0x51;
		mididata[2] = 3;
		ttempo = (int)(60000000.0 / 120.0 + 0.5);
		mididata[3] = (uchar)((ttempo >> 16) & 0xff);
		mididata[4] = (uchar)((ttempo >> 8) & 0xff);
		mididata[5] = (uchar)(ttempo & 0xff);
		outfile.addEvent(0, 0, mididata);
	}

	if (autopanQ) {
		autoPan(outfile, infile);
	}

	if (storeTextQ) {
		// store the title
		string title;
		title.resize(0);
		getTitle(title, infile);
		if (title.size() > 0) {
			storeMetaText(outfile, 0, title, 0, 3);
		}
	}

   vector<int> gracenote = getGraceNoteState(infile);

	PerlRegularExpression pre;

	for (i=0; i<infile.getNumLines(); i++) {
		if (debugQ) {
			cout << "Line " << i+1 << "::\t" << infile[i] << endl;
		}

		if (infile[i].isGlobalComment()) {
			if (pre.search(infile[i][0], "midi-transpose\\s*:\\s*(-?\\d+)")) {
				MidiTranspose = atoi(pre.getSubmatch(1));
			}
		}

		if (storeCommentQ && (infile[i].getType() == E_humrec_global_comment)) {
			if (timeQ || perfvizQ) {
				ontick = getMillisecondTime(infile, i);
				if (perfvizQ) {
					ontick = int(ontick * tickfactor + 0.5);
				}
			} else {
				absbeat = infile.getAbsBeat(i);
				ontick = int(absbeat * outfile.getTicksPerQuarterNote());
			}
			storeMetaText(outfile, 0, infile[i].getLine(), ontick + Offset);
		} else if (storeTextQ && (infile[i].getType() == E_humrec_bibliography)) {
			if (timeQ || perfvizQ) {
				ontick = getMillisecondTime(infile, i);
				if (perfvizQ) {
					ontick = int(ontick * tickfactor + 0.5);
				}
			} else {
				absbeat = infile.getAbsBeat(i);
				ontick = int(absbeat * outfile.getTicksPerQuarterNote());
			}
			if (strncmp(&(infile[i].getLine()[3]), "YEC", 3) == 0) {
				storeMetaText(outfile, 0, infile[i].getLine(), ontick+Offset, 2);
			// store OTL as regular text, creating sequence name separately
			//} else if (strncmp(&(infile[i].getLine()[3]), "OTL", 3) == 0) {
			//   storeMetaText(outfile, 0, infile[i].getLine(), ontick+offset, 3);
			} else if (storeCommentQ) {
				storeMetaText(outfile, 0, infile[i].getLine(), ontick + Offset);
			}
		}
		if (infile[i].isInterpretation() || infile[i].isGlobalComment()) {
			tempo = (int)checkForTempo(infile[i]);
			if (tempo > 0 && !tempospineQ && !perfvizQ) {
				// cout << "The tempo read was " <<  tempo << endl;
				ttempo = tempo;  // scaling already applied.
				mididata.resize(6);
				mididata[0] = 0xff;
				mididata[1] = 0x51;
				mididata[2] = 3;
				ttempo = (int)(60000000.0 / ttempo + 0.5);
				mididata[3] = (uchar)((ttempo >> 16) & 0xff);
				mididata[4] = (uchar)((ttempo >> 8) & 0xff);
				mididata[5] = (uchar)(ttempo & 0xff);
				if (timeQ || perfvizQ) {
					ontick = getMillisecondTime(infile, i);
					if (perfvizQ) {
						ontick = int(ontick * tickfactor + 0.5);
					}
				} else {
					absbeat = infile.getAbsBeat(i);
					ontick = int(absbeat * outfile.getTicksPerQuarterNote());
				}
				outfile.addEvent(0, ontick + Offset, mididata);
				// outfile.addEvent(0, 10 + Offset, mididata);
				tempo = -1;
			}

         // convert notes to MIDI
			for (j=infile[i].getFieldCount()-1; j>=0; j--) {
				if (timeQ || perfvizQ) {
					ontick = getMillisecondTime(infile, i);
					if (perfvizQ) {
						ontick = int(ontick * tickfactor + 0.5);
					}
				} else {
					absbeat = infile.getAbsBeat(i);
					ontick = int(absbeat * outfile.getTicksPerQuarterNote());
				}
				int track = Rtracks[infile[i].getPrimaryTrack(j)];

				if (strcmp(infile[i][j], "**kern") == 0 && forcedQ && !bendpcQ) {
					vector<uchar> mididata;
					mididata.resize(2);
					mididata[0] = 0xc0 | (0x0f & trackchannel[track]);
					mididata[1] = instrumentnumber;
					outfile.addEvent(track, ontick + Offset, mididata);
					continue;
				}

				// [20160813] skip over non-kern spines otherwise there will
				// be a segmentation in the track assignments.
				if (!infile[i].isExInterp(j, "**kern")) {
					continue;
				}
				if (strncmp(infile[i][j], "*I", 2) == 0) {
					storeInstrument(ontick + Offset, outfile, infile, i, j,
							instrumentQ);
					continue;
				}
				if ((!autopanQ) && (strncmp(infile[i][j], "*pan=", 5) == 0)) {
					storePan(ontick + Offset, outfile, infile, i, j);
					continue;
				}

				// capture info data for PerfViz:
				int length = strlen(infile[i][j]);
				int key;
				if (infile[i][j][length-1] == ':') {
					key = Convert::kernToBase40(infile[i][j]);
					key += MidiTranspose;
					if (key > 127) { key = 127; }
					else if (key < 0) { key = 0; }
					if (key != PerfVizNote::key) {
						printPerfVizKey(key);
						PerfVizNote::key = key;
					}
				}
				int tstop;
				int tsbottom;
				if (sscanf(infile[i][j], "*M%d/%d", &tstop, &tsbottom) == 2) {
					if (tstop != PerfVizNote::tstop ||
						tsbottom != PerfVizNote::tsbottom) {
						printPerfVizTimeSig(tstop, tsbottom);
						PerfVizNote::tstop = tstop;
						PerfVizNote::tsbottom = tsbottom;
					}
				}
				double approxtempo;
				if (sscanf(infile[i][j], "*MM%lf", &approxtempo) == 1) {
					if (approxtempo != PerfVizNote::approxtempo) {
						printPerfVizTempo(approxtempo);
						PerfVizNote::approxtempo = approxtempo;
					}
				}
				// info fields to be addressed in the future:
				// info(beatSubdivisions,[3]).
				// info(tempoIndication,[andante]).
				// info(subtitle,[]).


				if (strcmp(infile[i][j], "*free") == 0) {
					freeQ[infile[i].getPrimaryTrack(j)-1] = 1;
				} else if (strcmp(infile[i][j], "*strict") == 0) {
					freeQ[infile[i].getPrimaryTrack(j)-1] = 0;

					// turn off any previous notes in the track
					// started during a free rhythm section
					// why minus 1? [20150523]
					ptrack = Rtracks[infile[i].getPrimaryTrack(j) - 1];
					for (ii=0; ii<(int)freenotestate[ptrack].size(); ii++) {
						// turn off the note if it is zero or above
						if (freenotestate[ptrack][ii] >= 0) {
							mididata.resize(3);
							if (bendpcQ) {
								int pcchan = freenotestate[ptrack][ii] % 12;
								if (pcchan >= 9) {
									pcchan++;
								}
								mididata[0] = 0x80 | pcchan;
							} else {
								mididata[0] = 0x80 | trackchannel[track];
							}
							mididata[1] = (uchar)freenotestate[ptrack][ii];
							mididata[2] = 0;
							outfile.addEvent(track, ontick + Offset + 1, mididata);
							// added 1 to the previous line for grace notes 7Apr2004
							freenotestate[ptrack][ii] = -1;
							if (ii == ((int)freenotestate[ptrack].size() - 1)) {
								// shrink-wrap the free note off array
								freenotestate[ptrack].resize(ii);
								break;
							}
						}
					}
				}

			}
		} else if (infile[i].getType() == E_humrec_data_measure) {
			PerfVizNote::bar += 1;
		} else if (infile[i].getType() == E_humrec_data) {
			checkForTimeSignature(outfile, infile, i);
			checkForKeySignature(outfile, infile, i);
			idyncounter = 0;
			for (j=infile[i].getFieldCount()-1; j>=0; j--) {
				if (strcmp(infile[i][j], ".") == 0) {
					continue;
				}
				if (!infile[i].isExInterp(j, "**kern")) {
					continue;
				}

				if (idynQ) {
					if ((int)dynamics[i].size() > idyncounter) {
						volume = dynamics[i][idyncounter];
					} else {
					//  cout << "Warning: bad volume data on line " << i+1 << endl;
					//  cout << "Size of dynamics array is: "
					//       << dynamics[i].size() << endl;
					//  cout << "Note counter on line: " << idyncounter << endl;
					//  volume = 64;
					}
				} else {
					volume = dynamics[Rtracks[infile[i].getPrimaryTrack(j)]-1][i];
				}

				if (strcmp(infile[i][j], ".") == 0) {
					continue;
				}

				if (infile[i].getExInterpNum(j) != E_KERN_EXINT) {
					if (timeQ || perfvizQ) {
						ontick = getMillisecondTime(infile, i);
						if (perfvizQ) {
							ontick = int(ontick * tickfactor + 0.5);
						}
					} else {
						absbeat = infile.getAbsBeat(i);
						ontick = int(absbeat * outfile.getTicksPerQuarterNote());
					}
					track = Rtracks[infile[i].getPrimaryTrack(j)];
					if (storeTextQ) {
						storeMetaText(outfile, track, infile[i][j], ontick+Offset);
					}
					continue;
				} else {
					// process **kern note events
					// ggg

					track      = Rtracks[infile[i].getPrimaryTrack(j)];
					tokencount = infile[i].getTokenCount(j);
					ptrack     = Rtracks[infile[i].getPrimaryTrack(j)] - 1;

					if (freeQ[ptrack] != 0) {
						// turn off any previous notes in the track
						// during a free rhythm section
						for (ii=0; ii<(int)freenotestate[ptrack].size(); ii++) {
							// turn off the note if it is zero or above
							if (freenotestate[ptrack][ii] >= 0) {
								mididata.resize(3);
								if (bendpcQ) {
									int pcchan = freenotestate[ptrack][ii] % 12;
									if (pcchan >= 9) {
										pcchan++;
									}
									mididata[0] = 0x90 | pcchan;
								} else {
									mididata[0] = 0x80 | trackchannel[track];
								}
								mididata[1] = (uchar)freenotestate[ptrack][ii];
								mididata[2] = 0;
								outfile.addEvent(track, ontick + Offset, mididata);
								freenotestate[ptrack][ii] = -1;
								if (ii == ((int)freenotestate[ptrack].size() - 1)) {
									// shrink-wrap the free note off array
									freenotestate[ptrack].resize(ii);
									break;
								}
							}
						}
					}
					// This code is disabling dynamics, should be fixed.
					// [20130424]
					// But needs to be here so that --timbre volumes work [20131012]
					if (VolumeMapping.size() > 0) {
						volume = VolumeMapping[Rtracks[infile[i].getPrimaryTrack(j)]];
					}
					for (k=0; k<tokencount; k++) {
						infile[i].getToken(buffer1, j, k);

						// skip tied notes
						if (strchr(buffer1, '_') || strchr(buffer1, ']')) {
							continue;
						}

						if (timeQ) {
							duration = getMillisecondDuration(infile, i, j, k);
						} else if (perfvizQ) {
							duration = getMillisecondDuration(infile, i, j, k);
							// perfviz duration shortened later

							// store the score duration of the note for perfviz
							if (strchr(buffer1, '[')) {
								// total tied note durations
								pvscoredur = infile.getTiedDuration(i, j, k);
							} else {
								pvscoredur = rhysc * Convert::kernToDuration(buffer1);
							}
						} else {
							if (strchr(buffer1, '[')) {
								// total tied note durations
								duration = infile.getTiedDuration(i, j, k);
							} else {
								duration = rhysc * Convert::kernToDuration(buffer1);
							}
						}
						midinote = Convert::kernToMidiNoteNumber(buffer1);
						// skip rests
						if (midinote < 0) {
							continue;
						}
						midinote += MidiTranspose;
						if (midinote > 127) { midinote = 127; }
						else if (midinote < 0) { midinote = 0; }
						// base40note will be inaccurate if MidiTranspose <> 0
						base40note = Convert::kernToBase40(buffer1);

						if (!plainQ) {
							accentQ    = strchr(buffer1, '^')  == NULL ? 0 : 1;
							sforzandoQ = strchr(buffer1, 'z')  == NULL ? 0 : 1;
							staccatoQ  = strchr(buffer1, '\'') == NULL ? 0 : 1;
							// treat attacas/staccatissimos as staccatos
							staccatoQ |= strchr(buffer1, '`') == NULL ? 0 : 1;
							// treat pizzicatos as staccatos
							staccatoQ |= strchr(buffer1, '"') == NULL ? 0 : 1;
							// treat spiccatos as staccatos (maybe shorten later?)
							staccatoQ |= strchr(buffer1, 's') == NULL ? 0 : 1;
						}

						if (shortenQ) {
							duration -= shortenamount;
							if (duration < mine) {
								duration = mine;
							}
						}

						if (staccatoQ) {
							duration = duration * 0.5;
						}
						if (accentQ) {
							volume = (int)(volume * 1.3 + 0.5);
						}
						if (sforzandoQ) {
							volume = (int)(volume * 1.5 + 0.5);
						}
						if (volume > 127) {
							volume = 127;
						}
						if (volume < 1) {
							volume = 1;
						}

						if (plusQ) {
							volume = setMidiPlusVolume(buffer1);
						}
						if (timeQ) {
							ontick  = getMillisecondTime(infile, i);
							offtick = (int)(ontick + duration + 0.5);
						} else if (perfvizQ) {
							ontick  = int(getMillisecondTime(infile,i)*tickfactor+0.5);
							offtick = int(getMillisecondTime(infile,i)*tickfactor +
									duration*tickfactor + 0.5);
						} else {
							absbeat = infile.getAbsBeat(i);
							ontick  = int(absbeat * outfile.getTicksPerQuarterNote());
							offtick = int(duration *
									outfile.getTicksPerQuarterNote()) + ontick;
						}

                  // gracenote time adjustment
                  double tickadj = tempo * tpq  / 600.0;
                  ontick -= int(gracenote[i] * tickadj + 0.5);
                  offtick -= int(gracenote[i] * tickadj + 0.5);
                  if (ontick + Offset < 0) {
                     ontick = 0;
                  }
                  if (offtick + Offset < 0) {
                     offtick = 0;
                  }

						if (shortenQ) {
							offtick -= shortenamount;
							if (offtick - ontick < mine) {
								offtick = ontick + mine;
							}
						}

						// fix for grace note noteoffs (7 Sep 2004):
						if (timeQ) {
							if (offtick <= ontick) {
								offtick = ontick + 100;
							}
						} else if (perfvizQ) {
							if (offtick <= ontick) {
								offtick = int(ontick + 100 * tickfactor + 0.5);
							}
						} else {
							if (offtick <= ontick) {
								offtick = ontick + (int)(infile[i].getDuration()
										* outfile.getTicksPerQuarterNote()+0.5);
							}
							// in case the duration of the line is 0:
							if (offtick <= ontick) {
								offtick = ontick + 12;
							}
						}

						if (volume < 0) {
							volume = 1;
						}
						if (volume > 127) {
							volume = 127;
						}
						mididata.resize(3);
						if (bendpcQ) {
							int pcchan = midinote % 12;
							if (pcchan >= 9) {
								pcchan++;
							}
							mididata[0] = 0x90 | pcchan;
						} else {
							mididata[0] = 0x90 | trackchannel[track];
						}
						mididata[1] = midinote;
						mididata[2] = volume;
						if (fixedvolumeQ) {
							mididata[2] = defaultvolume;
						}
						outfile.addEvent(track, ontick + Offset, mididata);
						if (bendQ) {
							checkForBend(outfile, ontick + Offset, trackchannel[track],
									infile, i, j, bendamt);
						}
						idyncounter++;
						if (perfvizQ && PVIZ != NULL) {
							PerfVizNote pvnote;
							pvnote.pitch       = base40note;
							pvnote.scoredur    = pvscoredur;
							pvnote.absbeat     = infile[i].getAbsBeat();
							pvnote.beat        = int(infile[i].getBeat());
							pvnote.beatfrac    = infile[i].getBeat() - pvnote.beat;
							pvnote.beatdur     = pvscoredur;
							pvnote.vel         = volume;
							pvnote.ontick      = ontick;
							pvnote.offtick     = offtick;
							PVIZ[0] << pvnote;
						}
						if (freeQ[ptrack] == 0) {
							// don't specify the note off duration when rhythm is free.
							if (bendpcQ) {
								int pcchan = midinote % 12;
								if (pcchan >= 9) {
									pcchan++;
								}
								mididata[0] = 0x80 | pcchan;
							} else {
								mididata[0] = 0x80 | trackchannel[track];
							}
							if (monotuneQ) {
								addMonoTemperamentAdjustment(outfile, track,
										trackchannel[track], offtick+Offset,
										midinote, bendbypc);
							}
							outfile.addEvent(track, offtick + Offset, mididata);
						} else {
							// store the notes to be turned off later
							storeFreeNote(freenotestate, ptrack, midinote);
						}
					}
				}
			}
		}
	}

	// now add the end-of-track message to all tracks so that they
	// end at the same time.

	mididata.resize(3);

	if (timeQ) {
		ontick = getFileDurationInMilliseconds(infile);
		ontick += 1000;
	} else if (perfvizQ) {
		ontick = getFileDurationInMilliseconds(infile);
		ontick += 1000;
		ontick = int(ontick * tickfactor + 0.5);
	} else {
		absbeat = infile.getAbsBeat(infile.getNumLines()-1);
		ontick = int(absbeat * outfile.getTicksPerQuarterNote());
		// note that extra time is added so that the last note will not get
		// cut off by the MIDI player.
		ontick += 120;
	}

	// stupid Microsoft Media player (et al.) will still cut off early,
	// so add a dummy note-off at end as well...

	if (options.getBoolean("type0")) {
		outfile.joinTracks();
		if (padQ) {
			mididata[0] = 0x90;
			mididata[1] = 0x00;
			mididata[2] = 0x00;
			outfile.addEvent(0, ontick-1+Offset, mididata);
		}
		mididata[0] = 0xff;
		mididata[1] = 0x2f;
		mididata[2] = 0x00;
		outfile.addEvent(0, ontick+Offset, mididata);
	} else {  // type 1 MIDI file
		int trackcount = outfile.getNumTracks();
		for (i=0; i<trackcount; i++) {
			// Skip padding the first track, because Windows Media Player
			// won't play the MIDI file otherwise.
			if (i > 0 && padQ) {
				mididata[0] = 0x90;
				mididata[1] = 0x00;
				mididata[2] = 0x00;
				outfile.addEvent(i, ontick-1+Offset, mididata);
			}
			mididata[0] = 0xff;
			mididata[1] = 0x2f;
			mididata[2] = 0x00;
			outfile.addEvent(i, ontick+Offset, mididata);
		}
	}
}


//////////////////////////////
//
// getGraceNoteState --
//

vector<int> getGraceNoteState(HumdrumFile& infile) {
   vector<int> states(infile.getNumLines(), 0);
   for (int i=0; i<infile.getNumLines(); i++) {
      if (infile[i].getDuration() == 0.0) {
         states[i] = 1;
      }
   }
   for (int i=(int)states.size() - 2; i>=0; i--) {
      if (states[i]) {
         states[i] += states[i+1];
      }
   }
   return states;
}


//////////////////////////////
//
// checkForKeySignature -- Search backwards from the previous line
//     for a key signature and key.
//

void checkForKeySignature(smf::MidiFile& outfile, HumdrumFile& infile, int line) {
	int i;
	// int foundkeysig = -1;
	// int foundkey = -1;

	string primary;
	string key;
	string keysig;
	PerlRegularExpression pre;
	PerlRegularExpression pre2;
	for (i=line-1; i>0; i--) {
		if (infile[i].isData()) {
			break;
		}
		if (strncmp(infile[i][0], "**", 2) == 0) {
			// found nothing to the start of the data stream.
			break;
		} else if (infile[i].isInterpretation()) {
			int firstkern = -1;
			for (int j=0; j<infile[i].getFieldCount(); j++) {
				if (infile[i].isExInterp(j, "**kern")) {
					if (firstkern == -1) {
						if (pre2.search(infile[i][j], "^\\*([A-Ga-g][#-]*):")) {
							// extract the match later.
							key = pre2.getSubmatch(1);
						} else if (pre2.search(infile[i][j], "^(\\*k[[][A-Ga-g#-]*[]])")) {
							keysig = pre2.getSubmatch(1);
						}
						firstkern = j;
					} else {
						if (strcmp(infile[i][firstkern], infile[i][j]) != 0) {
							firstkern  = -1;
						} else {
							// all is well, so continue
						}
					}
				}
			}
		}
	}

	if (key.empty() || keysig.empty()) {
		return;
	}

	int ontick = 0;
	if (timeQ) {
		ontick = getMillisecondTime(infile, line);
	} else {
		double absbeat = infile.getAbsBeat(line);
		ontick = int(absbeat * outfile.getTicksPerQuarterNote());
	}

	uchar mode = 0; // major

	if (islower(key[0])) {
		mode = 1; // minor
	}

	int keynum = Convert::kernKeyToNumber(keysig.c_str());
	uchar accid = 0;
	if (keynum >= 0) {
		accid = (uchar)keynum;
	} else {
		accid = (uchar)(0x100 + keynum);
	}
	
	vector<uchar> metadata;
	metadata.push_back(accid);
	metadata.push_back(mode);
	outfile.addMetaEvent(0, ontick, 0x59, metadata);
}



//////////////////////////////
//
// checkForTimeSignature -- Search backwards from the previous line
//     for a time signature.  The time signature must be the same in every
//     part; otherwise, find a line such as
//         !!primary-mensuration: met(C|)
//     which can be used to convert to a time signature.
//

void checkForTimeSignature(smf::MidiFile& outfile, HumdrumFile& infile, int line) {
	int i;
	int foundsig = -1;
	int foundprimary = -1;
	// int foundline = -1;
	string primary;
	PerlRegularExpression pre;
	PerlRegularExpression pre2;
	for (i=line-1; i>0; i--) {
		if (infile[i].isData()) {
			break;
		}
		if (infile[i].isGlobalComment()) {
			if (pre.search(infile[i][0],
					"!!primary-mensuration:\\s*met\\(([^)]+)\\)\\s*$")) {
				foundprimary = i;
				primary = pre.getSubmatch(1);
				// cerr << "PRIMARY MENSURATION " << primary << endl;
				break;
			}
		} else if (strncmp(infile[i][0], "**", 2) == 0) {
			// found nothing to the start of the data stream.
			break;
		} else if (infile[i].isInterpretation()) {
			int firstkern = -1;
			for (int j=0; j<infile[i].getFieldCount(); j++) {
				if (infile[i].isExInterp(j, "**kern")) {
					if (firstkern == -1) {
						if (pre2.search(infile[i][j],
								"^\\*M(\\d+)/(\\d+)%?(\\d*)$")) {
							// extract the matches later.
						} else {
							break;
						}
						firstkern = j;
					} else {
						if (strcmp(infile[i][firstkern], infile[i][j]) != 0) {
							firstkern  = -1;
						} else {
							// all is well, so continue
						}
					}
				}
			}
			if (firstkern >= 0) {
				foundsig = firstkern;
				break;
			}
		}
		if ((foundprimary >= 0) || (foundsig >= 0)) {
			break;
		}
	}

	if ((foundprimary < 0) && (foundsig < 0)) {
		return;
	}

	int ontick = 0;
	if (timeQ) {
		ontick = getMillisecondTime(infile, line);
	} else {
		double absbeat = infile.getAbsBeat(line);
		ontick = int(absbeat * outfile.getTicksPerQuarterNote());
	}

	int top = 4;
	int bottom = 1;
	int bottom2 = 0;
	int cticks = 24;

	if (foundsig >= 0) {
		top     = atoi(pre2.getSubmatch(1));
		bottom  = atoi(pre2.getSubmatch(2));
		bottom2 = atoi(pre2.getSubmatch(3));
	} else if (foundprimary >= 0) {
		if (
				(primary == "C|")  ||
				(primary == "C.")  ||
				(primary == "C")   ||
				(primary == "C2")
			) {
			top    = 2;
			bottom = 1;
		}
		if (
				(primary == "3")    ||
				(primary == "3/2")  ||
				(primary == "C|3")  ||
				(primary == "C3")   ||
				(primary == "O|")   ||
				(primary == "O")    ||
				(primary == "O2")   ||
				(primary == "O/3")  ||
				(primary == "O3/2")
			) {
			top    = 3;
			bottom = 1;
		}
	}

	int ispow2 = ((bottom & (bottom - 1)) == 0);
	if (bottom2 > 0) {
		if ((top == 3) && (bottom == 3) && (bottom2 == 2)) {
			// 2/1 with coloration
			outfile.addTimeSignature(0, ontick, 2, 1, 64);
		}
		// MenCircle with coloration should go here.
	} else if (ispow2) {
		if (bottom == 0) {
			// Bottom represent a breve (double-whole note).
			// Convert it to a semi-breve since MIDI cannot handle it.
			bottom = 1;
			top *= 2;
		} else {
			cticks = cticks * 4.0 / bottom;
		}
		if ((top != 3) && (top % 3 == 0)) {
			outfile.addTimeSignature(0, ontick, top, bottom, cticks * 3);
		} else {
			outfile.addTimeSignature(0, ontick, top, bottom, cticks);
		}
	} else {
		// denominator is not a power of 2, so cannot represent
		// in MIDI...
	}

}



//////////////////////////////
//
// getTitle -- return the title of the work.  If OPR is present, then
//    include that first, then OTL
//

void getTitle(string& title, HumdrumFile& infile) {
	title.resize(1000);
	title.resize(0);
	int opr = -1;
	int otl = -1;
	int i;
	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isBibliographic()) {
			continue;
		}
		if ((opr < 0) && (strncmp(infile[i][0], "!!!OPR", 6) == 0)) {
			opr = i;
		}
		if ((otl < 0) && (strncmp(infile[i][0], "!!!OTL", 6) == 0)) {
			otl = i;
		}
	}

	char bufferopr[512] = {0};
	char bufferotl[512] = {0};
	if (opr >= 0) {
		infile[opr].getBibValue(bufferopr);
	}
	if (otl >= 0) {
		infile[otl].getBibValue(bufferotl);
	}
	char buffer[1024] = {0};
	strcat(buffer, bufferopr);
	if (otl >= 0) {
		if (opr >= 0) {
			strcat(buffer, "  ");
		}
	}
	strcat(buffer, bufferotl);
	int len = strlen(buffer);
	if (len == 0) {
		return;
	}
	title = buffer;
}



//////////////////////////////
//
// checkForBend --
//

void checkForBend(smf::MidiFile& outfile, int notetick, int channel,
		HumdrumFile& infile, int row, int col, double scalefactor) {
	double bendvalue = 0;

	int i;
	for (i=col+1; i<infile[row].getFieldCount(); i++) {
		if (strcmp(infile[row].getExInterp(i), "**Dcent") != 0) {
			continue;
		}
		if (strcmp(infile[row][i], ".") == 0) {
			// consider in the future to resolve "." to previous value
			// in case the **Dcent value is specified before a note attack
			break;
		}
		if (sscanf(infile[row][i], "%lf", &bendvalue)) {
			bendvalue = bendvalue / scalefactor;
			// store one tick before the expected note location
			// but this can cause an audible pitch aberation...
			//notetick = notetick - 1;
			//if (notetick < 0) {
			//   notetick = 0;
			// }
			outfile.addPitchBend(track, notetick, channel, bendvalue);
		}
		break;
	}
}



//////////////////////////////
//
// storeFreeNote -- store a midi note in the freenotestate array.
//

void storeFreeNote(vector<vector<int> >& array, int ptrack, int midinote) {
	int i;
	int loc = -1;
	for (i=0; i<(int)array[ptrack].size(); i++) {
		if (array[ptrack][i] < 0) {
			loc = i;
			break;
		}
	}

	if (loc >= 0) {
		array[ptrack][loc] = midinote;
	} else {
		array[ptrack].push_back(midinote);
	}
}



//////////////////////////////
//
// reviseInstrumentMidiNumbers --
//

void reviseInstrumentMidiNumbers(const char* string) {
	const char* spaces = " \t\n,:;";
	char* buffer = new char[strlen(string) + 1];
	strcpy(buffer, string);
	HumdrumInstrument x;

	char* pointer = buffer;
	pointer = strtok(buffer, spaces);
	int instnum = 0;
	while (pointer != NULL) {
		char* humdrumname = pointer;
		pointer = strtok(NULL, spaces);
		if (pointer == NULL) {
			break;
		}
		instnum = 0;
		sscanf(pointer, "%d", &instnum);

		if (instnum < 0 || instnum > 127) {
			continue;
		}
		x.setGM(humdrumname, instnum);
		pointer = strtok(NULL, spaces);
	}
}



//////////////////////////////
//
// setMidiPlusVolume -- store slur and enharmonic spelling information.
//    based on definition in the book:
//       Beyond MIDI: The Handbook of Musical Codes. pp. 99-104.
//

int setMidiPlusVolume(const char* kernnote) {
	int output = 1 << 6;

	// check for slurs, but do not worry about separating multiple slurs
	if (strchr(kernnote, '(') != NULL) {
		// start of slur A
		output |= (1 << 2);
	} else if (strchr(kernnote, ')') != NULL) {
		// end of slur A
		output |= (1 << 4);
	}

	// set the accidental marking
	int base40class = Convert::kernToBase40(kernnote) % 40;
	int midinoteclass = Convert::kernToMidiNoteNumber(kernnote) % 12;

	int acheck = 0;
	switch (midinoteclass) {
		case 0:                           // C/Dbb/B#
			switch (base40class) {
				case  4: acheck = 1; break;  // Dbb
				case  0: acheck = 2; break;  // C
				case 36: acheck = 3; break;  // B#
				default: acheck = 0;
			}
			break;
		case 1:                           // C#/Db/B##
			switch (base40class) {
				case  5: acheck = 1; break;  // Db
				case  1: acheck = 2; break;  // C#
				case 37: acheck = 3; break;  // B##
				default: acheck = 0;
			}
			break;
		case 2:                           // D/C##/Ebb
			switch (base40class) {
				case 10: acheck = 1; break;  // Ebb
				case  6: acheck = 2; break;  // D
				case  2: acheck = 3; break;  // C##
				default: acheck = 0;
			}
			break;
		case 3:                           // D#/Eb/Fbb
			switch (base40class) {
				case 15: acheck = 1; break;  // Fbb
				case 11: acheck = 2; break;  // Eb
				case  7: acheck = 3; break;  // D#
				default: acheck = 0;
			}
			break;
		case 4:                           // E/Fb/D##
			switch (base40class) {
				case 16: acheck = 1; break;  // Fb
				case 12: acheck = 2; break;  // E
				case  8: acheck = 3; break;  // D##
				default: acheck = 0;
			}
			break;
		case 5:                           // F/E#/Gbb
			switch (base40class) {
				case 21: acheck = 1; break;  // Gbb
				case 17: acheck = 2; break;  // F
				case 13: acheck = 3; break;  // E#
				default: acheck = 0;
			}
			break;
		case 6:                           // F#/Gb/E##
			switch (base40class) {
				case 22: acheck = 1; break;  // Gb
				case 18: acheck = 2; break;  // F#
				case 14: acheck = 3; break;  // E##
				default: acheck = 0;
			}
			break;
		case 7:                           // G/Abb/F##
			switch (base40class) {
				case 27: acheck = 1; break;  // Abb
				case 23: acheck = 2; break;  // G
				case 19: acheck = 3; break;  // F##
				default: acheck = 0;
			}
			break;
		case 8:                           // G#/Ab/F###
			switch (base40class) {
				case 28: acheck = 1; break;  // Ab
				case 24: acheck = 2; break;  // G#
				default: acheck = 0;         // F###
			}
			break;
		case 9:                           // A/Bbb/G##
			switch (base40class) {
				case 33: acheck = 1; break;  // Bbb
				case 29: acheck = 2; break;  // A
				case 25: acheck = 3; break;  // G##
				default: acheck = 0;
			}
			break;
		case 10:                           // Bb/A#/Cbb
			switch (base40class) {
				case 38: acheck = 1; break;  // Cbb
				case 34: acheck = 2; break;  // Bb
				case 30: acheck = 3; break;  // A#
				default: acheck = 0;
			}
			break;
		case 11:                           // B/Cf/A##
			switch (base40class) {
				case 39: acheck = 1; break;  // Cb
				case 35: acheck = 2; break;  // B
				case 31: acheck = 3; break;  // A##
				default: acheck = 0;
			}
			break;
	}

	output |= acheck;
	return output;
}



//////////////////////////////
//
// storePan --
//

void storePan(int ontime, smf::MidiFile& outfile, HumdrumFile& infile,
	int row, int column) {
	double value = 0.5;
	int mvalue = 64;
	sscanf(infile[row][column], "*pan=%lf", &value);
	if (value <= 1.0) {
		mvalue = int (value * 128.0);
	} else {
		mvalue = int(value + 0.5);
	}
	if (mvalue > 127) {
		mvalue = 127;
	} else if (mvalue < 0) {
		mvalue = 0;
	}

	int track = Rtracks[infile[row].getPrimaryTrack(column)];
	int channel = 0x0f & trackchannel[track];

	//ontime = ontime - 1;
	//if (ontime < 0) {
	//   ontime = 0;
	//}

	vector<uchar> mididata;
	mididata.resize(3);
	mididata[0] = 0xb0 | channel;
	mididata[1] = 10;
	mididata[2] = mvalue;
	outfile.addEvent(track, ontime, mididata);
}


//////////////////////////////
//
// autoPan -- presuming multi-track MIDI file.
//

void autoPan(smf::MidiFile& outfile, HumdrumFile& infile) {

	vector<int> kerntracks;
	getKernTracks(kerntracks, infile);
	reverse(kerntracks.begin(), kerntracks.end());

	double value = 0.0;
	int    mval  = 0;
	// vector<int> trackchannel;    // channel of each track

	vector<uchar> mididata;
	mididata.resize(3);

	long ontime = 0;
	int i;
	int channel;
	int track;
	for (i=0; i<(int)kerntracks.size(); i++) {
		// track = kerntracks[i];  Check later if reversed
		track = i;
		channel = trackchannel[track];
		value = 127.0 * i/(kerntracks.size()-1);
		if (value < 0.0) { value = 0.0; }
		if (value > 127.0) { value = 127.0; }
		mval = (int)value;

		mididata[0] = 0xb0 | channel;
		mididata[1] = 10;
		mididata[2] = (char)mval;
		outfile.addEvent(track, ontime, mididata);
	}

}



//////////////////////////////
//
// storeInstrument --
//

void storeInstrument(int ontick, smf::MidiFile& mfile, HumdrumFile& infile,
		int line, int col, int pcQ) {

	PerlRegularExpression pre;
	PerlRegularExpression pre2;

	int i;
	if (timbresQ && !forcedQ) {

		if (!pre.search(infile[line][col], "^\\*I\"\\s*(.*)\\s*", "")) {
			return;
		}
		string targetname;
		targetname = pre.getSubmatch(1);
		int pc = -1;
		for (i=0; i<(int)TimbreName.size(); i++) {
			if (pre2.search(targetname, TimbreName[i], "i")) {
				pc = TimbreValue[i];
				break;
			}
		}
		if (pc < 0) {
			for (i=0; i<(int)TimbreName.size(); i++) {
				if (pre2.search("DEFAULT", TimbreName[i], "i")) {
					pc = TimbreValue[i];
					break;
				}
			}
		}
		if (pc >= 0) {
			int track = -1;
			track = Rtracks[infile[line].getPrimaryTrack(col)];
			int channel = 0x0f & trackchannel[track];

			vector<uchar> mididata;
			mididata.resize(2);
			mididata[0] = 0xc0 | channel;
			mididata[1] = (uchar)pc;
			mfile.addEvent(track, ontick + Offset, mididata);
		}

	} else {
		static HumdrumInstrument huminst;
		int track = Rtracks[infile[line].getPrimaryTrack(col)];

		// const char* trackname = huminst.getName(infile[line][col]);
		string trackname = getInstrumentName(infile,
				Rtracks[infile[line].getPrimaryTrack(col)]);
		int pc = huminst.getGM(infile[line][col]);
		if (forcedQ) {
			pc = instrumentnumber;
		}
		int channel = 0x0f & trackchannel[track];

		// store the program change if requested:
		vector<uchar> mididata;
		mididata.resize(2);
		mididata[0] = 0xc0 | channel;
		mididata[1] = (uchar)pc;
		if (pcQ && pc >= 0 && !forcedQ) {
			mfile.addEvent(track, ontick + Offset, mididata);
		}

		if (!tracknamed[track]) {
			tracknamed[track] = 1;
			storeMetaText(mfile, track, trackname, 0, 3);    // Track Name
		}
		// storeMetaText(mfile, track, trackname, ontick + Offset, 4);  // Inst. Name
	}

}



//////////////////////////////
//
// getInstrumentName --
//

string getInstrumentName(HumdrumFile& infile, int ptrack) {
	int track;
	int i, j;
	PerlRegularExpression pre;
	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].isData()) {
			return "";
		}
		if (!infile[i].isInterpretation()) {
			continue;
		}
		for (j=0; j<infile[i].getFieldCount(); j++) {
			track = Rtracks[infile[i].getPrimaryTrack(j)];
			if (track != ptrack) {
				continue;
			}
			if (pre.search(infile[i][j], "^\\*I\"(.*)")) {
				return pre.getSubmatch(1);
			}
		}
	}
	return "";
}



//////////////////////////////
//
// usage --
//

void usage(const char* command) {

}


//////////////////////////////
//
// getDynamics --
//

void getDynamics(HumdrumFile& infile, vector<string>& dynamics,
		int defaultdynamic) {
	int maxtracks = infile.getMaxTracks();
	vector<int> currentdynamic;
	currentdynamic.resize(maxtracks);
	fill(currentdynamic.begin(), currentdynamic.end(), defaultdynamic);

	vector<int> metlev;
	infile.analyzeMetricLevel(metlev);

	vector<string> crescendos;  // -1=decrescendo, +1=crescendo, 0=none
	crescendos.resize(maxtracks);

	vector<string> accentuation;  // v = sf, sfz, fz, sffz
	accentuation.resize(maxtracks);

	dynamics.resize(maxtracks);

	int i;
	int j;
	for (i=0; i<(int)dynamics.size(); i++) {
		dynamics[i].resize(infile.getNumLines());
		crescendos[i].resize(infile.getNumLines());
		fill(crescendos[i].begin(), crescendos[i].end(), 0);
		accentuation[i].resize(infile.getNumLines());
		fill(accentuation[i].begin(), accentuation[i].end(), 0);
	}

	vector<int> assignments;  // dynamic data which controls kern data
	getDynamicAssignments(infile, assignments);


	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].getType() == E_humrec_data) {
			getNewDynamics(currentdynamic, assignments, infile, i, crescendos,
					accentuation);
		}
		for (j=0; j<(int)currentdynamic.size(); j++) {
			dynamics[j][i] = currentdynamic[j];
			// remove new dynamic marker:
			if (currentdynamic[j] < 0) {
				currentdynamic[j] = -currentdynamic[j];
			}
		}
	}

	processCrescDecresc(infile, dynamics, crescendos);

	// convert negative dynamics back into positive ones.
	if (dynamicsQ > 1) {
		cout << "----------------------------------------------" << endl;
		cout << "Dynamics profile of file:" << endl;
	}
	for (i=0; i<(int)dynamics[0].size(); i++) {
		for (j=0; j<(int)dynamics.size(); j++) {
			if (dynamics[j][i] < 0) {
				dynamics[j][i] = -dynamics[j][i];
			}
			if (humanvolumeQ) {
				dynamics[j][i] = adjustVolumeHuman(dynamics[j][i], humanvolumeQ);
			}
			if (metricvolumeQ) {
				dynamics[j][i] = adjustVolumeMetric(dynamics[j][i],
						metricvolumeQ, -metlev[i]);
			}
			if (dynamicsQ) {
				// probably should change accentuation of this type
				// into a continuous controller....
				dynamics[j][i] = applyAccentuation(dynamics[j][i],
						accentuation[j][i]);
			}
			if (dynamicsQ > 1) {
				cout << (int)dynamics[j][i] << "\t";
			}
		}
		if (dynamicsQ > 1) {
			cout << endl;
		}
	}
	if (dynamicsQ > 1) {
		cout << "----------------------------------------------" << endl;
	}

}



//////////////////////////////
//
// applyAccentuation --
//

char applyAccentuation(int dynamic, int accent) {
	switch (accent) {
		case 'v':
			dynamic += sforzando;
			break;
	}

	if (dynamic > 127) {
		dynamic = 127;
	} else if (dynamic <= 0) {
		dynamic = 1;
	}

	return (char)dynamic;
}



///////////////////////////////
//
// adjustVolumeMetric -- adjust the attack volume based on the
//  metric position of the note (on the beat, off the beat, on
//  a metrically strong beat).
//

char adjustVolumeMetric(int startvol, int delta, double metricpos) {
	if (metricpos > 0.0) {
		startvol += delta;
	} else if (metricpos < 0) {
		startvol -= delta;
	}

	if (startvol <= 0) {
		startvol = 1;
	} else if (startvol > 127) {
		startvol = 127;
	}

	return (char)startvol;
}



////////////////////////////////
//
// adjustVolumeHuman -- add a randomness to the volume
//

char adjustVolumeHuman(int startvol, int delta) {
	int randval;
	#ifndef VISUAL
		randval = lrand48() % (delta * 2 + 1) - delta;
	#else
		randval = rand() % (delta * 2 + 1) - delta;
	#endif

	startvol += randval;
	if (startvol <= 0) {
		startvol = 1;
	}
	if (startvol > 127) {
		startvol = 127;
	}

	return (char)startvol;
}



//////////////////////////////
//
// processCrescDecresc -- adjust attack velocities based on the
//    crescendos and decrescendos found in the file.
//

void processCrescDecresc(HumdrumFile& infile, vector<string>& dynamics,
	vector<string>& crescendos) {

	int i;
	for (i=0; i<(int)dynamics.size(); i++) {
		interpolateDynamics(infile, dynamics[i], crescendos[i]);
	}
}



//////////////////////////////
//
// interpolateDynamics --
//

void interpolateDynamics(HumdrumFile& infile, string& dyn,
		string& cresc) {
	int direction = 0;
	int i;
	int ii;

	for (i=0; i<(int)dyn.size(); i++) {
		if (cresc[i] != 0) {
			if (cresc[i] > 0) {
				direction = +1;
			} else if (cresc[i] < 0) {
				direction = -1;
			} else {
				direction = 0;
			}
			ii = findtermination(dyn, cresc, i);
			generateInterpolation(infile, dyn, cresc, i, ii, direction);

			// skip over calm water:
			i = ii - 1;
		}

	}
}



//////////////////////////////
//
// generateInterpolation -- do the actual interpolation work.
//

void generateInterpolation(HumdrumFile& infile, string& dyn,
		string& cresc, int startline, int stopline, int direction) {

	double startbeat = infile[startline].getAbsBeat();
	double stopbeat  = infile[stopline].getAbsBeat();

	if (startbeat == stopbeat) {
		// nothing to do
		return;
	}

	int startvel = dyn[startline];
	int stopvel  = dyn[stopline];
	if (startvel < 0)   startvel = -startvel;
	if (stopvel  < 0)   stopvel  = -stopvel;

	if (stopvel == startvel) {
		if (direction > 0) {
			stopvel += 10;
		} else {
			stopvel -= 10;
		}
	} else if (stopvel>startvel && direction<0) {
		stopvel = startvel - 10;
	} else if (stopvel<startvel && direction>0) {
		stopvel = startvel + 10;
	}

	int i;
	double slope = (double)(stopvel-startvel)/(double)(stopbeat-startbeat);
	double currvel = 0.0;
	double currbeat = 0.0;
	for (i=startline+1; i<stopline && i<(int)dyn.size(); i++) {
		currbeat = infile[i].getAbsBeat();
		currvel  = slope * (currbeat - startbeat) + startvel;
		if (currvel > 127.0) {
			currvel = 127.0;
		}
		if (currvel < 0.0) {
			currvel = 0.0;
		}
		dyn[i] = (char)(currvel+0.5);
	}

}



//////////////////////////////
//
// findtermination --
//

int findtermination(string& dyn, string& cresc, int start) {
	int i;
	for (i=start+1; i<(int)dyn.size(); i++) {
		if (cresc[i] != 0) {
			break;
		} else if (dyn[i] < 0) {
			break;
		}
	}

	if (i>=(int)dyn.size()) {
		i = dyn.size()-1;
	}
	return i;
}



//////////////////////////////
//
// getNewDynamics --
//

void getNewDynamics(vector<int>& currentdynamic, vector<int>& assignments,
		HumdrumFile& infile, int line, vector<string>& crescendos,
		vector<string>& accentuation) {

	if (infile[line].getType() != E_humrec_data) {
		return;
	}

	int i;
	int j;
	int k;
	int length;
	int track = -1;
	int dval = -1;
	int accent = 0;
	int cresval = 0;
	char buffer[2048] = {0};
	for (i=0; i<infile[line].getFieldCount(); i++) {
		dval = -1;
		cresval = 0;
		accent = 0;
		if (strcmp(infile[line][i], ".") == 0) {
			continue;
		}
		if (strcmp(infile[line].getExInterp(i), "**dynam") != 0) {
			continue;
		}

		// copy the **dynam value, removing any X information:
		length = strlen(infile[line][i]);
		k = 0;
		for (j=0; j<length && j<1024; j++) {
			if (infile[line][i][j] == 'm' ||
				infile[line][i][j] == 'p' ||
				infile[line][i][j] == 'f'
				) {
				buffer[k++] = infile[line][i][j];
			} else {
				// do not store character in dynamic string
			}
		}
		buffer[k] = '\0';

		track = Rtracks[infile[line].getPrimaryTrack(i)] - 1;

		if (strstr(buffer, "ffff") != NULL) {
			dval = DYN_FFFF;
		} else if (strcmp(buffer, "fff") == 0) {
			dval = DYN_FFF;
		} else if (strcmp(buffer, "ff") == 0) {
			dval = DYN_FF;
		} else if (strcmp(buffer, "f") == 0) {
			dval = DYN_F;
		} else if (strcmp(buffer, "mf") == 0) {
			dval = DYN_MF;
		} else if (strcmp(buffer, "mp") == 0) {
			dval = DYN_MP;
		} else if (strcmp(buffer, "p") == 0) {
			dval = DYN_P;
		} else if (strcmp(buffer, "pp") == 0) {
			dval = DYN_PP;
		} else if (strcmp(buffer, "ppp") == 0) {
			dval = DYN_PPP;
		} else if (strstr(buffer, "pppp") != NULL) {
			dval = DYN_PPPP;
		}

		// cannot have both > and < on the same line.
		if (strchr(infile[line][i], '<') != NULL) {
			cresval = +1;
		} else if (strchr(infile[line][i], '>') != NULL) {
			cresval = -1;
		}

		// look for accentuatnon
		if (strchr(infile[line][i], 'v') != NULL) {
			accent = 'v';
		}

		if (dval > 0 || cresval !=0) {
			for (j=0; j<(int)assignments.size(); j++) {
				if (assignments[j] == track) {
					// mark new dynamics with a minus sign
					// which will be removed after cresc/decresc processing
					if (dval > 0) {
						currentdynamic[j] = -dval;
					}
					if (cresval != 0) {
						crescendos[j][line] = cresval;
					}
					if (accent != 0) {
						accentuation[j][line] = accent;
					}
				}
			}
		}
	}

}



//////////////////////////////
//
// getDynamicAssignments --
//

void getDynamicAssignments(HumdrumFile& infile, vector<int>& assignments) {
	assignments.resize(infile.getMaxTracks());
	fill(assignments.begin(), assignments.end(), -1);

	// *staff assignment assumed to be all on one line, and before
	// any note data.

	int staffline = -1;
	int exinterp = -1;
	int i;
	int j;
	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].getType() == E_humrec_data) {
			break;
		}
		if (infile[i].getType() == E_humrec_interpretation) {
			if (strncmp(infile[i][0], "**", 2) == 0) {
				exinterp = i;
				continue;
			}
			if (strstr(infile[i][0], "staff") != NULL) {
				staffline = i;
				break;
			}
		}
	}

	// first assume that there are no *staff assignments and setup
	// the default values.

	int currdyn = -1;
	for (i=infile[exinterp].getFieldCount()-1; i>=0; i--) {
		if (strcmp(infile[exinterp][i], "**dynam") == 0) {
			currdyn = i;
		}
		if (currdyn >= 0) {
			assignments[i] = infile[exinterp].getPrimaryTrack(currdyn)-1;
		} else {
			assignments[i] = -1;
		}
	}

	if (staffline == -1) {
		// there is no staff assignments in the file, so attach any
		// dynamics to the first **kern spine on the left.

		return;
	}

	// there is a *staff assignment line, so follow the directions found there

	vector<vector<int> > staffvalues;
	getStaffValues(infile, staffline, staffvalues);

	int k;
	// assignments.resize(infile.getMaxTracks());
	for (i=0; i<infile[exinterp].getFieldCount(); i++) {
		if (strcmp(infile[exinterp][i], "**kern") != 0) {
			continue;
		}
		for (j=0; j<infile[exinterp].getFieldCount(); j++) {
			if (strcmp(infile[exinterp][j], "**dynam") != 0) {
				continue;
			}
			for (k=0; k<(int)staffvalues[j].size(); k++) {
				if (staffvalues[i][0] == staffvalues[j][k]) {
					assignments[infile[exinterp].getPrimaryTrack(i)-1] =
						infile[exinterp].getPrimaryTrack(j)-1;
				}
			}
		}
	}

	// for (i=0;i<assignments.size();i++) {
	//    cout << "ASSIGNMENT " << i << " = " << assignments[i] << endl;
	// }

}




//////////////////////////////
//
// getStaffValues --
//

void getStaffValues(HumdrumFile& infile, int staffline,
		vector<vector<int> >& staffvalues) {

	staffvalues.resize(infile[staffline].getFieldCount());

	int i;
	for (i=0; i<(int)staffvalues.size(); i++) {
		staffvalues[i].resize(0);
	}

	int value;
	const char* cptr;
	char* newcptr;
	for (i=0; i<infile[staffline].getFieldCount(); i++) {
		if (strncmp(infile[staffline][i], "*staff", 6) == 0) {
			if (strchr(infile[staffline][i], '/') == NULL) {
				if (sscanf(infile[staffline][i], "*staff%d", &value) == 1) {
					staffvalues[i].push_back(value);
				}
			} else {
				// more than one number in the staff assignment
				cptr = &(infile[staffline][i][6]);
				while (strlen(cptr) > 0) {
					if (!std::isdigit(cptr[0])) {
						break;
					}
					value = strtol(cptr, &newcptr, 10);
					staffvalues[i].push_back(value);
					cptr = newcptr;
					if (cptr[0] != '/') {
						break;
					} else {
						cptr = cptr + 1;
					}
				}
			}
		}
	}
}



//////////////////////////////
//
// getMillisecondTime -- return the time in milliseconds found
//    on the current line in the file.
//

int getMillisecondTime(HumdrumFile& infile, int line) {
	double output = -100;
	int i;

	while ((line < infile.getNumLines()) &&
		 (infile[line].getType() != E_humrec_data)) {
		line++;
	}
	if (line >= infile.getNumLines() - 1) {
		return getFileDurationInMilliseconds(infile);
	}

	for (i=0; i<infile[line].getFieldCount(); i++) {
		if (strcmp(infile[line].getExInterp(i), "**time") == 0) {
			if (strcmp(infile[line][i], ".") == 0) {
				cout << "Error on line " << line + 1 << ": no time value" << endl;
				exit(1);
			}
			//if (strcmp(infile[line][i], ".") == 0) {
			//   output = -1.0;
			//} else {
			//   flag = sscanf(infile[line][i], "%lf", &output);
			//}
			sscanf(infile[line][i], "%lf", &output);
			break;
		}
	}

	if (timeinsecQ) {
		output *= 1000.0;
	}

	if (output < 0.0) {
		return -1;
	} else {
		return (int)(output + 0.5);
	}
}



//////////////////////////////
//
// getMillisecondDuration --
//

int getMillisecondDuration(HumdrumFile& infile, int row, int col, int subcol) {
	char buffer1[1024] = {0};

	int startime = getMillisecondTime(infile, row);

	double output = -100.0;

	double duration = 0.0;

	if (strchr(buffer1, '[')) {
		// total tied note durations
		duration = infile.getTiedDuration(row, col, subcol);
	} else {
		infile[row].getToken(buffer1, col, subcol);
		duration = rhysc * Convert::kernToDuration(buffer1);
	}

	double stopbeat = duration + infile[row].getAbsBeat();

	int i, j;
	for (i=row+1; i<infile.getNumLines(); i++) {
		if (infile[i].getType() != E_humrec_data) {
			continue;
		}
		if (infile[i].getAbsBeat() >= (stopbeat-0.0002)) {
			for (j=0; j<infile[i].getFieldCount(); j++) {
				if (strcmp(infile[i].getExInterp(j), "**time") == 0) {
					sscanf(infile[i][j], "%lf", &output);
					break;
				}
			}
			break;
		}
	}

	if (timeinsecQ) {
		output *= 1000.0;
	}


	if (output - startime < 0.0) {
		return 0;
	} else {
		return (int)(output - startime + 0.5);
	}
}



//////////////////////////////
//
// getFileDurationInMilliseconds --
//

int getFileDurationInMilliseconds(HumdrumFile& infile) {

	int lastdataline = infile.getNumLines() - 1;
	while ((lastdataline > 0) &&
		 (infile[lastdataline].getType() != E_humrec_data)) {
		lastdataline--;
	}

	double ctime = getMillisecondTime(infile, lastdataline);
	double cbeat = infile[lastdataline].getAbsBeat();
	double nbeat = infile[infile.getNumLines()-1].getAbsBeat();

	int preindex = -1;

	int i;
	for (i=lastdataline-1; i>=0; i--) {
		if (infile[i].getType() != E_humrec_data) {
			continue;
		}

		preindex = i;
		break;
	}

	if (preindex < 0) {
		return -1;
	}

	double pbeat = infile[preindex].getAbsBeat();
	double ptime = -1.0;

	for (i=0; i<infile[preindex].getFieldCount(); i++) {
		if (strcmp("**time", infile[preindex].getExInterp(i)) != 0) {
			continue;
		}

		sscanf(infile[preindex][i], "%lf", &ptime);
		break;
	}

	if (ptime < 0.0) {
		return -1;
	}

	if (timeinsecQ) {
		ptime *= 1000.0;
	}

	double db2 = nbeat - cbeat;
	double db1 = cbeat - pbeat;
	double dt1 = ctime - ptime;

	// cout << "<< DB1 = " << db1 << " >> ";
	// cout << "<< DB2 = " << db2 << " >> ";
	// cout << "<< DT1 = " << dt1 << " >> ";

	double result = ctime + db2 * dt1 / db1;

	return (int)(result + 0.5);
}


///////////////////////////////////////////////////////////////////////////
//
// PerfViz related functions
//

ostream& operator<<(ostream& out, PerfVizNote& note) {

	int anchor        = ++note.sid;
	int measure       = note.bar;
	int beat          = note.beat;
	int velocity      = note.vel;
	int onset         = note.ontick;   // tick on time for note
	int offset        = note.offtick;  // tick off time for note
	int adjoffset     = offset;        // not taking pedalling into account
	double absbeaton  = note.absbeat;
	double absbeatoff = note.absbeat + note.scoredur;
	char pitchname[1024] = {0};
	Convert::base40ToPerfViz(pitchname, note.pitch);
	double soffset    = note.beatfrac;
	double dur        = note.beatdur;

	out << "snote(n"
		 << anchor     << ","
		 << pitchname  << ","
		 << measure    << ":"
		 << beat       << ",";

	printRational(out, soffset);
	out << ",";
	printRational(out, dur);
	out << ",";

	out << absbeaton   << ","
		 << absbeatoff  << ","
		 << "[])-";
	out << "note("
		 << anchor      << ","
		 << pitchname   << ","
		 << onset       << ","
		 << offset      << ","
		 << adjoffset   << ","
		 << velocity
		 << ").\n";

	return out;
}



//////////////////////////////
//
// printRational --
//

void printRational(ostream& out, double value) {
	if (value <= 0.0002) {
		out << "0";
		return;
	}


	if (fabs(value-0.5) < 0.0002) {
		out << "1/2";
	} else if (fabs(value - 0.25) < 0.0002) {
		out << "1/16";
	} else if (fabs(value - 1.5) < 0.0002) {
		out << "3/2";
	} else if (fabs(value - 2.5) < 0.0002) {
		out << "5/2";
	} else if (fabs(value - 0.75) < 0.0002) {
		out << "3/16";
	} else if (fabs(value - 0.16667) < 0.0002) {
		out << "1/8/3";
	} else if (fabs(value - int(value)) <= 0.0002) {
		out << int(value);
	} else if (fabs(value - int(value)) >= 0.9998) {
		out << int(value)+1;
	} else if (value == 1.0) {
		out << "1";
	} else if (fabs(value - 0.3333) < 0.0002) {
		out << "1/4/3";
	} else if (fabs(value - 0.6667) < 0.0002) {
		out << "2/4/3";
	} else {
		out << "{{" << value << "}}";
	}

}



//////////////////////////////
//
// writePerfVizMatchFile --
//

void writePerfVizMatchFile(const string& filename, stringstream& contents) {
	ofstream outputfile(filename.c_str());
	contents   << ends;
	outputfile << contents.str();
	outputfile.close();
}


//////////////////////////////
//
// printPerfVizKey --
//

void printPerfVizKey(int key) {
	int octave = key / 40;
	int diatonic = Convert::base40ToDiatonic(key) % 7;
	int accidental = Convert::base40ToAccidental(key);

	if (PVIZ == NULL) {
		return;
	}

	PVIZ[0] << "info(keySignature,[";
	switch (diatonic) {
		case 0:  PVIZ[0] << "c"; break;
		case 1:  PVIZ[0] << "d"; break;
		case 2:  PVIZ[0] << "e"; break;
		case 3:  PVIZ[0] << "f"; break;
		case 4:  PVIZ[0] << "g"; break;
		case 5:  PVIZ[0] << "a"; break;
		case 6:  PVIZ[0] << "b"; break;
		default: PVIZ[0] << "X";
	}
	switch (accidental) {
		case -2: PVIZ[0] << "bb"; break;
		case -1: PVIZ[0] << "b"; break;
		case  0: PVIZ[0] << "n"; break;
		case  1: PVIZ[0] << "#"; break;
		case  2: PVIZ[0] << "##"; break;
	}
	PVIZ[0] << ",";
	if (octave == 3) {
		PVIZ[0] << "major";
	} else if (octave == 4) {
		PVIZ[0] << "minor";
	} else {
		PVIZ[0] << "locrian";
	}
	PVIZ[0] << "]).\n";
}



//////////////////////////////
//
// printPerfVizTimeSig --
//

void printPerfVizTimeSig(int tstop, int tsbottom) {
	if (PVIZ == NULL) {
		return;
	}
	PVIZ[0] << "info(timeSignature," << tstop << "/" << tsbottom << ").\n";
}



//////////////////////////////
//
// printPerfVizTempo --
//

void printPerfVizTempo(double approxtempo) {
	if (PVIZ == NULL) {
		return;
	}

	PVIZ[0] << "info(approximateTempo," << approxtempo << ").\n";

}



//////////////////////////////
//
// storeTimbres --
//

void storeTimbres(vector<string>& name, vector<int>& value,
		vector<int>& volumes, const string& input) {
	PerlRegularExpression pre;
	vector<string> tokens;
	pre.getTokens(tokens, "\\s*;\\s*", input);
	name.resize(tokens.size());
	value.resize(tokens.size());
	volumes.resize(tokens.size());
	name.resize(0);
	value.resize(0);
	volumes.resize(0);

	string tempstr;
	int temp;
	int i;
	for (i=0; i<(int)tokens.size(); i++) {
		if (pre.search(tokens[i], "(.*)\\s*:\\s*i?(\\d+),v(\\d+)", "")) {
			temp = atoi(pre.getSubmatch(2));
			value.push_back(temp);
			temp = atoi(pre.getSubmatch(3));
			volumes.push_back(temp);
			tempstr = pre.getSubmatch(1);
			name.push_back(tempstr);
		} else if (pre.search(tokens[i], "(.*)\\s*:\\s*(\\d+)", "")) {
			temp = atoi(pre.getSubmatch(2));
			value.push_back(temp);
			tempstr = pre.getSubmatch(1);
			name.push_back(tempstr);
		}
	}
}



//////////////////////////////
//
// getKernTracks --  Return a list of the **kern primary tracks found
//     in the Humdrum data.  Currently all tracks are independent parts.
//     No grand staff parts are considered if the staves are separated
//     into two separate spines.
//
//

void getKernTracks(vector<int>& tracks, HumdrumFile& infile) {
	tracks.resize(infile.getMaxTracks());
	tracks.resize(0);
	int i;
	for (i=1; i<=infile.getMaxTracks(); i++) {
		if (infile.getTrackExInterp(i) == "**kern") {
			tracks.push_back(i);
		}
	}
}



