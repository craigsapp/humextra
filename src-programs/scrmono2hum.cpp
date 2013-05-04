//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Mar 26 09:12:54 PST 2002
// Last Modified: Sat Mar 30 14:24:32 PST 2002
// Filename:      ...sig/examples/all/scorepitch.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/score/scorepitch.cpp
// Syntax:        C++; museinfo
//
// Description:   Extract pitch information from a SCORE data file.
//
// Todo:          generate thru directive, beam markings, stem directions
// Done:          metronome pitch, measure numbers, Slurs, ties
//                first & second endings, segmentation for thru command
//

#include "humdrum.h"
#include "ScorePageSimple.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#ifndef OLDCPP
   #include <fstream>
#else
   #include <fstream.h>
#endif

// P1_Note 		= 1,
// P1_Rest			= 2,
// P1_Clef			= 3,
// P1_Line			= 4,
// P1_Slur			= 5,
// P1_Beam			= 6,
// P1_Trill		= 7,
// P1_Staff		= 8,
// P1_Symbol		= 9,
// P1_Number		= 10,
// P1_User			= 11,
// P1_Special		= 12,
// P1_BadLuck		= 13,
// P1_Barline		= 14,
// P1_ImportedEPSGraphic	= 15,
// P1_Text			= 16,
// P1_KeySignature		= 17,
// P1_MeterSignature	= 18



class Thing {
   public:
      int type;        // type of data: note, rest, barline, key, or meter
      int loc;         // staff number of location in score data
      int index;       // index number of item in staff
      int pitch;       // the pitch or negative if a rest
      double dur;      // duration;
      double ndur;     // duration of following measure of music;
      double nndur;    // duration of next following measure of measure;
      int clef;        // clef type
      int control;     // 1 = controlling barline
      int slstart;     // start of slur
      int slend;       // end of slur marker
      int slstart2;    // start of slur 2
      int slend2;      // end of slur marker 2
      int tiestart;    // start of tie marker
      int tiecont;     // continuation of tie
      int tieend;      // end of tie
      int measure;     // measure number, if a measure
      int mstyle;      // measure style, -1 = use P5 value
      int metronome;   // metronome marking on first note.
      int finalbar;    // 1 = final barline;
      double meterdur; // the proper duration of a measure of music
      int segment;     // thru section control marker
      int ending;      // ending marker for multiple endings
      int repeattype;  // 1 = forward repeat, 2 = end repeat, 3 = both
      double absbeat;  // absolute beat position in music (quarter note = beat)
      
      Thing(void) { clear(); }
      void clear(void) {
         type = 0; loc = -1; dur = 0.0; clef = 0; index = 0; finalbar = 0;
         slstart = 0; slend = 0; tiestart = 0; tiecont = 0; tieend = 0;
         meterdur = 0.0; control = 0; ndur = 0.0; nndur = 0.0; pitch = 0;
         measure = -1000; segment = 0; slstart2 = 0; slend2 = 0;
         metronome = 0; absbeat = 0.0; mstyle = -1; ending = 0;
         repeattype = 0;
      }
};


class char1024 { public: char c[1024]; char1024(void) { c[0] = '\0'; } };


class Extra {
   public:
      char     text[1024];
      double   duration;
      int      measure;
      double   measurebeat;
      int      precedence;
      int      mark;
    
      Extra(void) { clear(); }
      void clear(void) {
         text[0] = '\0'; duration = -1; measure = -1; measurebeat = 1.0;
         precedence = 'b'; mark = 0;
      }
};


class Thru {
   public: 
      int seg; 
      int ending;
   Thru(void) { clear(); }
   void clear(void) { seg = ending = 0; }
   int operator==(Thru& b) { return (seg == b.seg) && (ending == b.ending); }
};


// function declarations:
void  extractPitches     (ScorePageSimple& score);
void  printKern          (ostream& out, ScorePageSimple& score, Array<Thing>& things, 
                          Array<char1024>& header, Array<char1024>& trailer,
                          Array<Extra>& extras, Array<Thru>& thruinfo);
void  printClef          (ostream& out, int clef);
void  applySlurs         (ScorePageSimple& score, Array<Thing>& things);
void  findMetronome      (ScorePageSimple& score, Array<Thing>& things);
void  assignMeasure      (ScorePageSimple& score, Array<Thing>& things);
void  storeItem          (ScoreRecord& record, int& clef, int& key, 
                          Array<Thing>& things, int location, int index);
void  cleanupThings      (ScorePageSimple& score, Array<Thing>& things, 
                          Array<Extra>& extras, Array<Thru>& thruinfo);
void  getThings          (ScorePageSimple& score, Array<Thing>& things);
void  printKey           (ostream& out, int key);
int   findNearestNote    (ScorePageSimple& score, float pos, int staff);
int   findIndex          (Array<Thing>& things, int staff, int index);
void  checkForTie        (Array<Thing>& things, int id, int start);
char* getTitle           (char* buffer, ScorePageSimple& score);
void  getHeaderAndTrailer(const char* globalfilename, const char* localfilename,
                          Array<char1024>& header, Array<char1024>& trailer,
                          Array<Extra>& extras);
void  readExtraData      (Extra& extra, const char* string);
void  printExtras        (Array<Thing>& things, Array<Extra>& extras, int index,
                          ostream& out);
void  generateThruInfo   (Array<Thing>& things, Array<Thru>& thruinfo);
int   findNearestMeasure (ScorePageSimple& score, float pos, int staff);
double   findDuration    (Array<Thing>& things, int measure);
ostream& operator<<      (ostream& out, Thru& thru);
void     printNoRep      (Array<Thru>& thruinfo, ostream& out);


// interface variables:
Options options;
int     verboseQ  = 0;    // Display debugging information
int     rhythmQ   = 1;    // used with the -r option
int     titleQ    = 0;    // used with the -t option
int     checksumQ = 0;    // used with the -c option


int main(int argc, char** argv) {
   options.define("v|verbose=b",      "print debugging information");
   options.define("t|title=b",        "extract title from SCORE data");
   options.define("c|checksum=b",     "Append !!!VTS record to end of output");
   options.define("g|global-ref=s",   "global reference record file");
   options.define("l|local-ref=s",    "local reference record file");
   options.define("R|no-rhythm=b",    "don't display rhythm information");
   options.process(argc, argv);
   checksumQ =  options.getBoolean("checksum");
   verboseQ  =  options.getBoolean("verbose");
   rhythmQ   = !options.getBoolean("no-rhythm");
   titleQ    =  options.getBoolean("title");
   const char* globalfilename = "";
   const char* localfilename  = "";
   if (options.getBoolean("global-ref")) {
      globalfilename = options.getString("global-ref");
   }
   if (options.getBoolean("local-ref")) {
      localfilename = options.getString("local-ref");
   }
   Array<char1024> header;    // header bibliographic records
   Array<char1024> trailer;   // trailer bibliographic records
   Array<Extra> extras;       // extra comments to interleave with data
   extras.setSize(100);
   extras.allowGrowth(100);
   extras.setSize(0);
   header.setSize(0);
   trailer.setSize(0);
   getHeaderAndTrailer(globalfilename, localfilename, header, trailer, extras);

   if (options.getArgCount() == 0) {
      cout << "Usage: " << argv[0] << " input.mus " << endl;
      exit(1);
   }

   ScorePageSimple score;
   Array<Thing> things;
   Array<Thru> thruinfo;
   int i;
   for (i=1; i<=options.getArgCount(); i++) {
      score.clear();
      score.readFile(options.getArg(i), verboseQ); 
      getThings(score, things);
      assignMeasure(score, things);
      applySlurs(score, things);
      findMetronome(score, things);
      cleanupThings(score, things, extras, thruinfo);

      printKern(cout, score, things, header, trailer, extras, thruinfo);
#ifndef VISUAL
      if (checksumQ) {
         fstream outfile("/tmp/scrmono2hum-temp", ios::out);
         printKern(outfile, score, things, header, trailer, extras, thruinfo);
         outfile.close();
         system("chmod 0666 /tmp/scrmono2hum-temp");
         system("cksum /tmp/scrmono2hum-temp | sed 's/ .*//' > /tmp/scrmono2hum-temp2");
         system("chmod 0666 /tmp/scrmono2hum-temp2");
         #ifndef OLDCPP
            fstream infile("/tmp/scrmono2hum-temp2", ios::in);
         #else
            fstream infile("/tmp/scrmono2hum-temp2", ios::in | ios::nocreate);
         #endif
         unsigned long value;
         infile >> value; 
         cout << "!!!VTS: " << value << endl;
         infile.close();
         system("rm /tmp/scrmono2hum-temp2");
         system("rm /tmp/scrmono2hum-temp");
      }
#endif /* VISUAL */
   }

   cout << flush;
   return 0;
}


//////////////////////////////////////////////////////////////////////////



//////////////////////////////
//
// findMetronome -- look for metronome markings and apply to 
//   things.
//

void findMetronome(ScorePageSimple& score, Array<Thing>& things) {
   int i, j;
   int tempo = 0;
   int sindex;
   int tindex;
   const char* pos;
   for (i=0; i<=score.getMaxStaff(); i++) {
      for (j=0; j<score.getStaffSize(i); j++) {
         tempo = 0;
         if (score.getStaff(i, j).isTextItem()) {
            if ((pos = strstr(score.getStaff(i, j).getTextData(), "[ = "))) {
               sscanf(pos, "[ = %d", &tempo);
            } else if ((pos = strstr(score.getStaff(i, j).getTextData(), "[= "))) {
               sscanf(pos, "[= %d", &tempo);
            } else if ((pos = strstr(score.getStaff(i, j).getTextData(), "[ ="))) {
               sscanf(pos, "[ =%d", &tempo);
            } else if ((pos = strstr(score.getStaff(i, j).getTextData(), "[="))) {
               sscanf(pos, "[=%d", &tempo);
            }
            if (tempo > 0) {
               sindex = findNearestNote(score, score.getStaff(i, j).getPValue(3), i);
               tindex = findIndex(things, i, sindex);
               if (tindex >= 0) {
                  things[tindex].metronome = tempo;
               }
            }
         } 
      }
   }


}



//////////////////////////////
//
// cleanupThings -- do finalization of items before printing in **kern format.
//

void cleanupThings(ScorePageSimple& score, Array<Thing>& things, 
      Array<Extra>& extras, Array<Thru>& thruinfo) {
   int i;

   // mark start of verse endings as segment
   for (i=0; i<things.getSize(); i++) {
      if (things[i].type == P1_Barline && things[i].ending != 0) {
         things[i].segment = 1;
      }
   }

   // check for segment indicators
   int segfound = 0;
   for (i=0; i<things.getSize(); i++) {
      if (things[i].segment) {
         segfound = 1;
         break;
      }
   }

   // add a starting segment marker if there is a segment
   // marker present and no segment marker already before the
   // first note
   if (segfound) {
      for (i=0; i<things.getSize(); i++) {
         if (things[i].type == P1_Note || things[i].type == P1_Rest) {
            things[i].segment = 1;
            break;
         } else if (things[i].segment) {
            // already a segment indicate before the first note.
            // so don't put in another one.
            break;
         }
      }
   }

   // adjust segment numbers
   int segment = 0;
   if (segfound) {
      for (i=0; i<things.getSize(); i++) {
         if (things[i].ending) {
            things[i].segment = segment;
         } else if (things[i].segment && !things[i].finalbar) {
            things[i].segment = ++segment;
         }
      }
   }

   // generate segment traversal array
   if (segfound) {
      generateThruInfo(things, thruinfo);
   } else {
      thruinfo.setSize(0);
   }

   // calculate absbeat values
   double absbeat = 0.0;
   for (i=0; i<things.getSize(); i++) {
      things[i].absbeat = absbeat;
      absbeat += things[i].dur;
   }

   // convert extra things with beats greater than 1.0 to
   // duration extras
   for (i=0; i<extras.getSize(); i++) {
      if (extras[i].measure > 0 && extras[i].measurebeat > 1.0) {
         extras[i].duration = findDuration(things, extras[i].measure) +
               extras[i].measurebeat + 2.0;
         if (extras[i].duration >= 0) {
            extras[i].measure = -1;
            extras[i].measurebeat = 1.0;
         }
      }
   }

   // remove consecutive barline markers due to repeats
   // at the starts of lines.
   for (i=0; i<things.getSize(); i++) {
      if (i > 0 && things[i].type == P1_Barline && 
          things[i-1].type == P1_Barline) {
         things[i-1].mstyle = (int)score.getStaff(things[i].loc,
               things[i].index).getPValue(5);
         if (things[i-1].measure < 0) {
            things[i-1].measure = things[i].measure;
         }
         things[i].type = -P1_Barline;
      } else if (i > 1 && things[i].type == P1_Barline &&
          things[i-1].type == P1_MeterSignature &&
          things[i-2].type == P1_Barline) {
         // catch cases where there is a new time signature
         // just before the repeat sign at the start of a line
         things[i-2].mstyle = (int)score.getStaff(things[i].loc,
               things[i].index).getPValue(5);
         if (things[i-2].measure < 0) {
            things[i-2].measure = things[i].measure;
         }
         things[i].type = -P1_Barline;
      }
   }

}



//////////////////////////////
//
// assignMeasures -- place measure numbers.
//

void assignMeasure(ScorePageSimple& score, Array<Thing>& things) {
   int i;

   // identify the final barline of the piece
   for (i=things.getSize()-1; i>=0; i--) {
      if (things[i].type == P1_Barline) {
         things[i].finalbar = 1;
         break;
      }
   }

   // measure the duration of each measure:
   double duration = 0.0;
   double meterdur = 0.0;
   int top;
   int bottom;
   for (i=0; i<things.getSize(); i++) {
      switch (things[i].type) {
         case P1_Note:
         case P1_Rest:
            duration += things[i].dur;
            break;
         case P1_Barline:
            things[i].dur = duration;
            things[i].meterdur = meterdur;
            duration = 0.0;
            break;
         case P1_MeterSignature:
            // composite time signature are not yet handled.
            top = (int)score.getStaff(things[i].loc, 
                  things[i].index).getPValue(5);
            bottom = (int)score.getStaff(things[i].loc, 
                  things[i].index).getPValue(6);
            if (top == 0) {
               top = bottom;
               bottom = 4;
            }
            meterdur = top * 4.0 / bottom;
            break;
      }
   }


   // mark controlling measures
   double olddur = 0.0;
   double oldolddur = 0.0;
   int control = 0;
   for (i=things.getSize()-1; i>=0; i--) {
      if (things[i].type == P1_Barline) {
         if (fabs(things[i].meterdur - things[i].dur) < 0.01) {
            things[i].control = control;
            control = 1;
         } else {
            things[i].control = control;
            control = 0;
         }
         things[i].ndur  = olddur;
         things[i].nndur = oldolddur;
         oldolddur       = olddur;
         olddur          = things[i].dur;
      }
   }


   // the penultimate measure is controlling even if 
   // the last measure does not have a complete duration
   int count = 0;
   for (i=things.getSize()-1; i>=0; i--) {
      if (things[i].type == P1_Barline) {
         count++;
         if (count == 2) {
            things[i].control = 1;
            break;
         }
      }
   }


   // shift meter durations back one measure
   double lastmeterdur = -1.0;
   double tempmeterdur = 0.0;
   for (i=things.getSize()-1; i>=0; i--) {
      if (things[i].type == P1_Barline) {
         if (lastmeterdur < 0.0) {
            lastmeterdur = things[i].meterdur;
         }
         tempmeterdur = things[i].meterdur;
         things[i].meterdur = lastmeterdur;
         lastmeterdur = tempmeterdur;
      } 
   }


   // look for non-controlling barlines.
   int setcontrol = 0;
   int marker = 0;
   for (i=0; i<things.getSize(); i++) {
      if (things[i].type == P1_Barline) {
         if (setcontrol == 1) {
            marker = 1;
            setcontrol = 0;
         }
         if (things[i].control == 0) {
            if (fabs(things[i].ndur + things[i].nndur - things[i].meterdur) < 
                  0.01) {
               things[i].control = 1;
               setcontrol = 1;
            }
         }
         if (marker) {
            things[i].control = -1;
            marker = 0;
         }
      }
   }


   // determine if the first measure is a pick-up measure or a
   // complete measure.
   int pickupQ = 1;
   for (i=0; i<things.getSize(); i++) {
      if (things[i].type == P1_Barline) {
         if (things[i].dur != 0.0 && 
               fabs(things[i].dur - things[i].meterdur) < 0.01) {
            pickupQ = 0; 
            break;
         } else if (things[i].dur != 0.0) {
            pickupQ = 1;
            break;
         } else {
            pickupQ = 1;
            break;
         }
      }
   }


   // now ready to number the measures
   int measurenum = 2;
   if (pickupQ) {
      measurenum = 1;
   }
   for (i=0; i<things.getSize(); i++) {
      if (things[i].type == P1_Barline) {
         if (things[i].control == 1) {
            things[i].measure = measurenum++;
         }
      }
   }


}



//////////////////////////////
//
// applySlurs -- associate slurs and ties with the correct notes.
//   elided slurs are not handled although id mechanism is in place
//   to be able to do so.
//

void applySlurs(ScorePageSimple& score, Array<Thing>& things) {
   int i;
   int staff;
   float startpos;
   float endpos;
   float pos;
   int nearstart;
   int nearend;
   int slurid = 1;
   int index;

   for (i=0; i<score.getSize(); i++) {
      if ((score[i].getPValue(1) == P1_Slur) && (score[i].getPValue(8) < 0)) {
         // if a real slur
         staff     = (int)score[i].getPValue(2);
         startpos  = score[i].getPValue(3);
         endpos    = score[i].getPValue(6);
         nearstart = findNearestNote(score, startpos, staff);
         nearend   = findNearestNote(score, endpos,   staff);
         if (nearstart < 0 || nearend < 0) {
            continue;
         }
         if (nearstart == nearend) {
            // choose the end of the slur which is closest to the note
            pos = score.getStaff(staff, nearstart).getPValue(3);
            if (fabs(pos - startpos) <= fabs(pos - endpos)) {
               index = findIndex(things, staff, nearstart);
               if (things[index].slstart) {
                  things[index].slstart2 = slurid;
               } else {
                  things[index].slstart = slurid;
               }
            } else {
               index = findIndex(things, staff, nearend);
               if (things[index].slend) {
                  things[index].slend2 = slurid;
               } else {
                  things[index].slend = slurid;
               }
            }
            slurid++;
         } else {
            index = findIndex(things, staff, nearstart);
            if (things[index].slstart) {
               things[index].slstart2 = slurid;
            } else {
               things[index].slstart = slurid;
            }
            index = findIndex(things, staff, nearend);
            if (things[index].slend) {
               things[index].slend2 = slurid;
            } else {
               things[index].slend = slurid;
            }
            slurid++;
         }
      } else if ((score[i].getPValue(1) == P1_Slur) && 
              (score[i].getPValue(8) > 0)       &&
              (score[i].getPValue(9) >= 0)      ) {
         // a verse ending marker
         int endingstart = findNearestMeasure(score, score[i].getPValue(3),
            (int)score[i].getPValue(2));
         index = findIndex(things, (int)score[i].getPValue(2), endingstart);
         if (score[i].getPValue(9) == 0) {
            things[index].ending = 1;
         } else {
            things[index].ending = (int)score[i].getPValue(9);
         }
      }
   }


   // identify slurs which are actually ties
   for (i=0; i<things.getSize(); i++) {
      if (things[i].type == P1_Note || things[i].type == P1_Rest) {
         if (things[i].slstart != 0) {
            checkForTie(things, things[i].slstart, i);
         }
         if (things[i].slstart2 != 0) {
            checkForTie(things, things[i].slstart2, i);
         }
      }
   }


   // collapse tie start/stops
   for (i=0; i<things.getSize(); i++) {
      if (things[i].tiestart != 0 && things[i].tieend != 0) {
         things[i].tiecont  = 1;
         things[i].tiestart = 0;
         things[i].tieend   = 0;
      }
   }

   // adjust slur set2s into slur set1s for printing
   for (i=0; i<things.getSize(); i++) {
      if (things[i].slstart2 != 0 && things[i].slstart == 0) {
         things[i].slstart = things[i].slstart2;
         things[i].slstart2 = 0;
      }
      if (things[i].slend2 != 0 && things[i].slend == 0) {
         things[i].slend = things[i].slend2;
         things[i].slend2 = 0;
      }

   }


}


//////////////////////////////
//
// checkForTie -- look to see if slur is actually a tie and fix it 
//   if necessary.
//

void checkForTie(Array<Thing>& things, int id, int start) {
   int intervene = 0;
   int tieQ = 0;
   int i;
   for (i=start+1; i<things.getSize(); i++) {
      if (things[i].type == P1_Note || things[i].type == P1_Rest) {
         intervene++;
         if (intervene != 1) {
            break;
         }
         if (things[i].pitch != things[start].pitch) {
            break;
         }

         if (things[i].slend == id || things[i].slend == id - 1) {
            things[i].tieend = id;
            things[i].slend  = 0;
            tieQ = 1;
            break;
         } else if (things[i].slend2 == id || things[i].slend2 == id - 1) {
            things[i].tieend = id;
            things[i].slend2 = 0;
            tieQ = 1;
            break;
         } 
      }
   }

   if (tieQ) {
      if (things[start].slstart == id) {
         things[start].tiestart = id;
         things[start].slstart  = 0;
      } else if (things[start].slstart2 == id) {
         things[start].tiestart = id;
         things[start].slstart2 = 0;
      }
   }

}



//////////////////////////////
//
// findIndex -- find the item with the specified position in the
//    score.  Returns -1 if not found in list.
//

int findIndex(Array<Thing>& things, int staff, int index) {
   int i;
   int output = -1;
   for (i=0; i<things.getSize(); i++) {
      if (things[i].loc == staff && things[i].index == index) {
         output = i;
         return output;
      }
   }

   return output;
}



//////////////////////////////
//
// findNearestNote -- return the index on the staff for the nearest
//     note OR rest.
//

int findNearestNote(ScorePageSimple& score, float pos, int staff) {
   int i;
   int bestindex = -1;
   float bestdistance = 1000.0;
   float testdistance;
   for (i=0; i<score.getStaffSize(staff); i++) {
      if (score.getStaff(staff, i).hasDurationQ()) {
         testdistance = fabs(score.getStaff(staff, i).getPValue(3) - pos);
         if (testdistance < bestdistance) {
            bestdistance = testdistance;
            bestindex = i;
         }
      }
   }

   return bestindex;
}



//////////////////////////////
//
// findNearestMeasure -- return the index on the staff for the nearest
//     note OR rest.
//

int findNearestMeasure(ScorePageSimple& score, float pos, int staff) {
   int i;
   int bestindex = -1;
   float bestdistance = 1000.0;
   float testdistance;
   for (i=0; i<score.getStaffSize(staff); i++) {
      if (score.getStaff(staff, i).isBarlineItem()) {
         testdistance = fabs(score.getStaff(staff, i).getPValue(3) - pos);
         if (testdistance < bestdistance) {
            bestdistance = testdistance;
            bestindex = i;
         }
      }
   }

   return bestindex;
}



//////////////////////////////
//
// printKern -- print the things in the Thing array in **kern format.
//

void printKern(ostream& out, ScorePageSimple& score, Array<Thing>& things, 
      Array<char1024>& header, Array<char1024>& trailer, 
      Array<Extra>& extras, Array<Thru>& thruinfo) {
   int i, j;
   char pbuffer[128] = {0};
   char dbuffer[128] = {0};

   // clear the already-printed marker
   for (i=0; i<extras.getSize(); i++) {
      extras[i].mark = 0;
   }

   // print all of the bibliographic header
   for (j=0; j<header.getSize(); j++) {
      out << header[j].c << "\n";
   }

   char buffer[1024] = {0};
   if (titleQ) {
      getTitle(buffer, score);
      if (strlen(buffer) > 0) {
         out << "!!!OTL: " << buffer << endl;
      }
   }

   int thruQ = 0;
   out << "**kern\n";
   int bartype = 0;
   for (i=0; i<things.getSize(); i++) {
      if (things[i].segment > 0 && things[i].segment <= 26 &&
          !things[i].finalbar) {
         if (thruQ == 0 && thruinfo.getSize()) {
            thruQ = 1;
            out << "*>[";
            for (j=0; j<thruinfo.getSize(); j++) {
               out << thruinfo[j];
               if (j < thruinfo.getSize()-1) {
                  out << ",";
               }
            }
            out << "]\n";
            printNoRep(thruinfo, out);
         }
         out << "*>" << (char)('A' + things[i].segment - 1);
         if (things[i].ending) {
            out << things[i].ending;
         }
         out << "\n";
      }

      printExtras(things, extras, i, out);

      switch (things[i].type) {

      case P1_Note:
      case P1_Rest:
         if (things[i].metronome > 0) {
            out << "*MM" << things[i].metronome << "\n";
         }
         if (things[i].slstart) {
            out << "(";
         }
         if (things[i].tiestart) {
            out << "[";
         }
         out << Convert::durationToKernRhythm(dbuffer, things[i].dur);
         if (things[i].type == P1_Note) {
            out << Convert::base40ToKern(pbuffer, 
                  score.getStaff(things[i].loc, things[i].index).getPitch());
         } else {
            out << "r";
         }
         if (things[i].tiecont) {
            out << "_";
         } else if (things[i].tieend) {
            out << "]";
         }
         if (things[i].slend) {
            out << ")";
         }
         out << "\n";
         break;

      case P1_Barline:
         out << "=";
         if (things[i].finalbar) {
            out << "=";
         } else if ((int)score.getStaff(things[i].loc,
               things[i].index).getPValue(5) == 2) {
            out << "=";
            things[i].finalbar = 1;
         }
         if (things[i].measure != -1000 && things[i].finalbar != 1) {
            out << things[i].measure;
         }
         if (things[i].mstyle >= 0) {
            bartype = things[i].mstyle;
         } else {
            bartype = (int)score.getStaff(things[i].loc, 
                  things[i].index).getPValue(5);
         }
         switch (bartype) {
            case 1:    out << "||"; break;
            // case 2: out << "="; break;
            case 3:    out << ":|!"; break;
            case 4:    out << "!|:"; break;
            case 5:    out << ":|!|:"; break;
            case 6:    out << ":!!:"; break;
         }
         if (verboseQ) {
            out << "(" << things[i].dur << ":" << things[i].ndur 
                                         << ":" << things[i].nndur << ")";
            out << "[" << things[i].meterdur << "]";
            out << "{" << things[i].control << "}";
         }
         out << "\n";

         // print which ending is starting, if applicable
         if (things[i].ending != 0) {
            cout << "!! ending " << things[i].ending << "\n";
         }
        
         break;
      case P1_MeterSignature:
         out << "*M" 
              << (int)score.getStaff(things[i].loc,things[i].index).getPValue(5)
              << "/" 
              <<(int)score.getStaff(things[i].loc,things[i].index).getPValue(6);
         out << "\n";
         break;
      case P1_KeySignature:
         printKey(out, (int)score.getStaff(things[i].loc,things[i].index).getPValue(5));
         out << "\n";
         break;
      case P1_Clef:
         printClef(out, things[i].clef);
         out << "\n";
         break;

      }
   }
   out << "*-" << endl;
   for (j=0; j<trailer.getSize(); j++) {
      out << trailer[j].c << "\n";
   }
}



//////////////////////////////
//
// getThings --
//

void getThings(ScorePageSimple& score, Array<Thing>& things) {
   things.setSize(score.getSize());
   things.setSize(0);
   things.allowGrowth(1);

   score.analyzePitch();
   int i, j;
   int clef = -1000;
   int key  = -1000;
   for (i=score.getMaxStaff(); i>=0; i--) {
      if (verboseQ) {
         cout << "Staff: " << i << "\titems: " << score.getStaffSize(i) << endl;
      }
      for (j=0; j<score.getStaffSize(i); j++) {
         storeItem(score.getStaff(i, j), clef, key, things, i, j);
      }
   }

}



//////////////////////////////
//
// storeItem -- store the SCORE item for later processing.
//

void storeItem(ScoreRecord& record, int& clef, int& key, 
      Array<Thing>& things, int staffno, int index) {
   Thing tempthing;
   int tempkey;
   int tempclef;
   int bartype;

   switch ((int)record.getPValue(1)) {
      case P1_Note:
         tempthing.type = P1_Note;
         tempthing.loc = staffno;
         tempthing.index = index;
         tempthing.pitch = record.getPitch();
         if (record.getPValue(7) == 0.0 || record.getPValue(7) > 64.0) {
            tempthing.dur = 0.0;
         } else {
            tempthing.dur = record.getPValue(7);
         }
         things.append(tempthing);
         break;
      case P1_Rest:
         tempthing.type = P1_Rest;
         tempthing.dur = record.getPValue(7);
         tempthing.loc = staffno;
         tempthing.index = index;
         tempthing.pitch = -1000;
         things.append(tempthing);
         break;
      case P1_Barline:
         tempthing.type = P1_Barline;
         tempthing.loc = staffno;
         tempthing.index = index;
         tempthing.dur = 0.0;
         bartype = (int)record.getPValue(5);
         if (bartype >= 3 && bartype <= 6) {
            tempthing.segment = 1;
            switch (bartype) {
               case 3:
                  tempthing.repeattype = 2;
                  break;
               case 4:
                  tempthing.repeattype = 1;
                  break;
               case 5: case 6:
                  tempthing.repeattype = 3;
                  break;
            }
         }
         things.append(tempthing);
         break;
      case P1_MeterSignature:
         tempthing.type = P1_MeterSignature;
         tempthing.loc = staffno;
         tempthing.index = index;
         tempthing.dur = 0.0;
         things.append(tempthing);
         break;
      case P1_KeySignature:
         tempkey = (int)record.getPValue(5);
         if (key < -10 || key != tempkey) {
            key = tempkey;
            tempthing.type = P1_KeySignature;
            tempthing.dur = 0.0;
            tempthing.loc = staffno;
            tempthing.index = index;
            things.append(tempthing);
         }
         break;
      case P1_Clef:
         tempclef = (int)record.getPValue(5);
         if (clef < -10 || clef != tempclef) {
            clef = tempclef;
            tempthing.type  = P1_Clef;
            tempthing.clef  = clef;
            tempthing.dur   = 0.0;
            tempthing.loc   = staffno;
            tempthing.index = index;
            things.append(tempthing);
         }

   }
}



//////////////////////////////
//
// printClef -- print the **kern clef token.
//   Ignoring Soprano, Baritone, and Mezzo-soprano clefs for now.
//

void printClef(ostream& out, int clef) {
   switch (clef) {
      case  0:  out << "*clefG2";    break;   // treble
      case  1:  out << "*clefF4";    break;   // bass
      case  2:  out << "*clefC3";    break;   // alto
      case  3:  out << "*clefC4";    break;   // tenor
      case  4:  out << "*clefX";     break;   // percussion
   }
}



//////////////////////////////
//
// printKey -- print the **kern key token.
//

void printKey(ostream& out, int key) {
   switch (key) {
      case -7:  out << "*k[b-e-a-d-g-c-f-]";  break;
      case -6:  out << "*k[b-e-a-d-g-c-]";    break;
      case -5:  out << "*k[b-e-a-d-g-]";      break;
      case -4:  out << "*k[b-e-a-d-]";        break;
      case -3:  out << "*k[b-e-a-]";          break;
      case -2:  out << "*k[b-e-]";            break;
      case -1:  out << "*k[b-]";              break;
      case  0:  out << "*k[]";                break;
      case  1:  out << "*k[f#]";              break;
      case  2:  out << "*k[f#c#]";            break;
      case  3:  out << "*k[f#c#g#]";          break;
      case  4:  out << "*k[f#c#g#d#]";        break;
      case  5:  out << "*k[f#c#g#d#a#]";      break;
      case  6:  out << "*k[f#c#g#d#a#e#]";    break;
      case  7:  out << "*k[f#c#g#d#a#e#b#]";  break;
   }
}


//////////////////////////////
//
// getTitle -- A Highly interpreted value which may not be
//    appropriate for all data files.
//

char* getTitle(char* buffer, ScorePageSimple& score) {
   buffer[0] = '\0';
   int i;
   int staff = score.getMaxStaff();
   for (i=0; i<score.getStaffSize(staff); i++) {
      if (score.getStaff(staff, i).isTextItem()) {
         ScoreRecord& record = score.getStaff(staff, i);
         if (record.getPValue(4) >= 20) {
            if (record.getTextData()[0] == '_') {
               strcpy(buffer, &record.getTextData()[3]);
            } else {
               strcpy(buffer, record.getTextData());
            }
            break;
         }
      }
   }

   return buffer;
}



//////////////////////////////
//
// getHeaderAndTrailer --  Read the bibliographic information the given
//     files contain.
//

void getHeaderAndTrailer(const char* globalfilename, const char* localfilename,
      Array<char1024>& header, Array<char1024>& trailer, 
      Array<Extra>& extras) {
   char1024 buffer;
   Array<char1024> gfile;
   Array<char1024> lfile;
   gfile.setSize(100);
   gfile.setGrowth(100);
   gfile.setSize(0);
   lfile.setSize(100);
   lfile.setGrowth(100);
   lfile.setSize(0);

   if (strlen(globalfilename) > 0) {
      #ifndef OLDCPP
         fstream ginfile(globalfilename, ios::in);
      #else
         fstream ginfile(globalfilename, ios::in | ios::nocreate);
      #endif
      if (!ginfile.is_open()) {
         cout << "Error: cannot open file " << globalfilename 
              << " for reading" << endl;
         exit(1);
      }
      while (!ginfile.eof()) {
         ginfile.getline(buffer.c, 1000, '\n');
         if (buffer.c[0] == '@' || buffer.c[0] == '!') {
            gfile.append(buffer);
         }
      }
      ginfile.close();
   }

   if (strlen(localfilename) > 0) {
      #ifndef OLDCPP
         fstream linfile(localfilename, ios::in);
      #else
         fstream linfile(localfilename, ios::in | ios::nocreate);
      #endif
      if (!linfile.is_open()) {
         cout << "Error: cannot open file " << localfilename 
              << " for reading" << endl;
         exit(1);
      }
      while (!linfile.eof()) {
         linfile.getline(buffer.c, 1000, '\n');
         if (buffer.c[0] == '@' || buffer.c[0] == '!' ||
             buffer.c[0] == 'a' || buffer.c[0] == 'b' ||
             buffer.c[0] == '=' || 
             buffer.c[0] == 'A' || buffer.c[0] == 'B') {
            lfile.append(buffer);
         }
      }
      linfile.close();
   }

   int i;
   Array<int> gloc;
   Array<int> lloc;
   gloc.setSize(gfile.getSize());
   lloc.setSize(lfile.getSize());
   int state = 0;  // 1 = header, 2 = trailer, -1 read lheader, -2 read ltrailer

   for (i=0; i<gfile.getSize(); i++) {
      if (strncmp(gfile[i].c, "@TOP", 4) == 0) {
         state = 1;
         gloc[i] = 0;
         continue;
      } else if (strncmp(gfile[i].c, "@MOVEMENT-TOP", 13) == 0) {
         gloc[i] = -1;
         continue;
      } else if (strncmp(gfile[i].c, "@MOVEMENT-BOTTOM", 16) == 0) {
         gloc[i] = -2;
         continue;
      } else if (strncmp(gfile[i].c, "@BOTTOM", 7) == 0) {
         state = 2;
         gloc[i] = 0;
         continue;
      } else if (strncmp(gfile[i].c, "@DATA", 5) == 0) {
         state = 3;
         gloc[i] = 0;
         continue;
      } else if (state != 3 && strncmp(gfile[i].c, "!", 1) != 0) {
         gloc[i] = 0;
         continue;
      }
      gloc[i] = state;
   }

   for (i=0; i<lfile.getSize(); i++) {
      if (strncmp(lfile[i].c, "@TOP", 4) == 0) {
         state = 1;
         lloc[i] = 0;
         continue;
      } else if (strncmp(lfile[i].c, "@BOTTOM", 7) == 0) {
         state = 2;
         lloc[i] = 0;
         continue;
      } else if (strncmp(lfile[i].c, "@DATA", 5) == 0) {
         state = 3;
         lloc[i] = 0;
         continue;
      } else if (state != 3 && strncmp(lfile[i].c, "!", 1) != 0) {
         lloc[i] = 0;
         continue;
      }
 
      lloc[i] = state;
   }

   // build the header and trailer
   int j;
   header.setSize(lloc.getSize() + gloc.getSize());
   header.setSize(0);
   trailer.setSize(lloc.getSize() + gloc.getSize());
   trailer.setSize(0);
   for (i=0; i<gfile.getSize(); i++) {
      switch (gloc[i]) {
         case 1:
            header.append(gfile[i]);
            break;
         case 2:
            trailer.append(gfile[i]);
            break;
         case -1:
            for (j=0; j<lfile.getSize(); j++) {
               if (lloc[j] == 1) {
                  header.append(lfile[j]);
               }
            }
            break;
         case -2:
            for (j=0; j<lfile.getSize(); j++) {
               if (lloc[j] == 2) {
                  trailer.append(lfile[j]);
               }
            }
            break;
      }
   }

   // build the extras array
   Extra tempextra;
   for (i=0; i<gfile.getSize(); i++) {
      if (gloc[i] == 3) {
         readExtraData(tempextra, gfile[i].c);
         extras.append(tempextra);
      }
   }
   for (i=0; i<lfile.getSize(); i++) {
      if (lloc[i] == 3) {
         readExtraData(tempextra, lfile[i].c);
         extras.append(tempextra);
      } else {
      }
   }

}



//////////////////////////////
//
// readExtraData -- read the external data records which will
//   be placed into the automatically generated Humdrum data 
//   read from the SCORE file.
//

void readExtraData(Extra& extra, const char* string) {
   if (strchr(string, '\t') == NULL) {
      cout << "Error in input for data extras: " << string << endl;
      exit(1);
   }
   extra.clear();
   char1024 c;
   strcpy(c.c, string);
   char* ptr1;
   char* ptr2;
   ptr1 = strtok(c.c, "\t");
   ptr2 = strtok(NULL, "\t");
   if (ptr2 == NULL || strlen(ptr2) == 0) {
      return;
   }

   int count;
   switch (tolower(ptr1[0])) {
      case 'a':
      case 'b':
         extra.precedence = tolower(ptr1[0]);
         extra.duration = 0.0;
         extra.duration = strtod(&ptr1[1], NULL);
         strcpy(extra.text, ptr2);
         break;

      case '=':
         if (strlen(ptr1) < 3) {
            return;
         }
         strcpy(extra.text, ptr2);
         extra.precedence = tolower(ptr1[1]);
         if (extra.precedence != 'a' && extra.precedence != 'b') {
            cout << "Error: invalid data extra line: " << string << endl;
            exit(1);
         }
         count = sscanf(&ptr1[2], "%d,%lf", &extra.measure, &extra.measurebeat);
         if (count != 2) {
            count = sscanf(&ptr1[2], "%d", &extra.measure);
            extra.measurebeat = 1.0;
            if (count != 1) {
               cout << "Error 2: invalid data extra line: " << string << endl;
               exit(1);
            }
         }
         break;
   }

}



//////////////////////////////
//
// printExtras -- 
//

void printExtras(Array<Thing>& things, Array<Extra>& extras, int index, 
      ostream& out) {

   double currentloc = things[index].absbeat;
   double nextloc = 0.0;
   double lastloc = 0.0;
   if (index < things.getSize()-1) {
      nextloc = things[index+1].absbeat;
   } else {
      nextloc = 999999.0;
   }
   if (index > 0) {
      lastloc = things[index-1].absbeat;
   } else {
      lastloc = -999999.0;
   }

   double lastdiff;
   double currdiff;

   int i;
   for (i=0; i<extras.getSize(); i++) {
      if (extras[i].mark != 0) {
         continue;
      }

      if (extras[i].duration >= 0.0) {
         currdiff = fabs(currentloc - extras[i].duration);
         lastdiff = fabs(lastloc - extras[i].duration);

         switch (extras[i].precedence) {
            case 'b':   // place before events
               if (extras[i].duration > lastloc && 
                   (extras[i].duration <= currentloc ||
                    currdiff < 0.01)) {
                   out << extras[i].text << "\n";
                   extras[i].mark = 1;
               }
               break;
   
            case 'a':   // place after events
               if (extras[i].duration < currentloc && 
                   (extras[i].duration >= lastloc ||
                    lastdiff < 0.01)) {
                   out << extras[i].text << "\n";
                   extras[i].mark = 1;
               }
               break;
         }
      } else {
         // measure anchored data
         switch (extras[i].precedence) {
            case 'b':
               if (things[index].type == P1_Barline &&
                  things[index].measure == extras[i].measure) {
                  out << extras[i].text << "\n";
                  extras[i].mark = 1;
               }
               break;
            case 'a':
               if (index > 0 && things[index-1].type == P1_Barline &&
                  things[index-1].measure == extras[i].measure) {
                  out << extras[i].text << "\n";
                  extras[i].mark = 1;
               }
               break;
         }
      }

   }
}



//////////////////////////////
//
// findDuration -- find the duration of the given measure in the music.
//

double findDuration(Array<Thing>& things, int measure) {
   int i;
   double output = -1.0;
   for (i=0; i<things.getSize(); i++) {
      if (things[i].type == P1_Barline && things[i].measure == measure) {
         output = things[i].absbeat;   
         break;
      }
   }

   return output;
}



//////////////////////////////
//
// generateThruInfo -- find the correct thru command directive.
//    This function will probably have to be rewritten.
//

void generateThruInfo(Array<Thing>& things, Array<Thru>& thruinfo) {
   thruinfo.setSize(0);
   
   int back = 0;
   int i = 0;
   int done = 0;
   int ending = 1;
   Thru tempthru;
   while (!done) {
      if (things[i].segment) {
         switch (things[i].repeattype) {
            case 1:
               ending = 1;
sillytag:
               tempthru.seg = things[i].segment;
               tempthru.ending = things[i].ending;
               if (things[i].segment) {
                  thruinfo.append(tempthru);
               }
               back = i;
               break;
            case 2:
               if (ending == 1) {
                  i = back;
                  ending++;
                  goto sillytag;
               }
               ending++;
               if (ending != 1 && !things[i].finalbar) {
                  tempthru.seg = things[i].segment;
                  tempthru.ending = things[i].ending;
                  thruinfo.append(tempthru);
               }
               break;
            case 3:
               if (ending == 1) {
                  i = back;
                  ending++;
                  goto sillytag;
               } else {
                  tempthru.seg = things[i].segment;
                  tempthru.ending = things[i].ending;
                  thruinfo.append(tempthru);
                  back = i + 1;
                  ending = 1;
               }
               break;
            default:
               if (things[i].ending == ending) {
                  tempthru.seg = things[i].segment;
                  tempthru.ending = things[i].ending;
                  thruinfo.append(tempthru);
               } else if (things[i].ending == 0) {
                  tempthru.seg = things[i].segment;
                  tempthru.ending = things[i].ending;
                  thruinfo.append(tempthru);
               } 
         }
      }
      i++;
      if (i >= things.getSize()) {
         done = 1;
      }
   }

}


//////////////////////////////
//
// ostream<<(Thru) -- how to print out a thru marker
//

ostream& operator<<(ostream& out, Thru& thru) {
   out << (char)('A' + thru.seg - 1.0);
   if (thru.ending) {
      out << thru.ending;
   }
   return out;
}



//////////////////////////////
//
// printNoRep -- print the thru information taking only one repeat,
//   and all last endings.
//

void printNoRep(Array<Thru>& thruinfo, ostream& out) {
   Array<Thru> copy;
   copy = thruinfo;

   int i;
   // get rid of second repeats
   for (i=1; i<copy.getSize(); i++) {
       if ((copy[i-1] == copy[i]) && (copy[i].ending == 0)) {
          copy[i].seg = 0;
          copy[i].ending = 0;
       }
   }

   // get rid of repeat of earlier endings
   for (i=0; i<copy.getSize()-3; i++) {
      if (copy[i].seg == 0) {
         // already deleted
         continue;
      }
      if (copy[i+1].seg == 0) {
         // already deleted
         continue;
      }
      if ( (copy[i] == copy[i+2]) && 
           (copy[i+1].seg == copy[i+3].seg) &&
           (copy[i+1].ending == copy[i+3].ending - 1) ) {
         copy[i].seg = 0;
         copy[i+1].seg = 0;
         copy[i].ending = 0;
         copy[i+1].ending = 0;
      }
   }

   Array<Thru> newdata;
   newdata.setSize(copy.getSize());
   newdata.setSize(0);

   for (i=0; i<copy.getSize(); i++) {
      if (copy[i].seg != 0) {
         newdata.append(copy[i]);
      }
   }

   if (newdata.getSize() == 0) {
      return;
   }

   out << "*>norep[";
   for (i=0; i<newdata.getSize(); i++) {
      out << newdata[i];
      if (i < newdata.getSize() - 1) {
         out << ",";
      }
   }
   out << "]\n";

}



// md5sum: b4159c3bc8ec1a20926e9b8bb58fb625 scrmono2hum.cpp [20050403]
