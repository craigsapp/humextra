//
// Copyright 2002,2009 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Feb 11 19:48:50 PST 2002
// Last Modified: Sun Mar 24 12:10:00 PST 2002 (small changes for visual c++)
// Last Modified: Sat Mar 21 14:28:55 PST 2009
// Filename:      ...sig/src/sigInfo/ScoreRecordSimple.cpp
// Web Address:   http://sig.sapp.org/include/sigInfo/ScoreRecordSimple.cpp
// Syntax:        C++ 
//
// Description:   A SCORE item parameter class
//

#include "ScoreRecordSimple.h"
#include "Convert.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

#ifndef OLDCPP
   #include <iostream>
   #include <iomanip>
   #include <sstream>
   #define SSTREAM stringstream
   #define CSTRING str().c_str()
   using namespace std;
#else
   #include <iostream.h>
   #include <iomanip.h>
   #ifdef VISUAL
      #include <strstrea.h>
   #else
      #include <strstream.h>
   #endif
   #define SSTREAM strstream
   #define CSTRING str()
#endif
   

//////////////////////////////
//
// ScoreRecordSimple::ScoreRecordSimple --
//

ScoreRecordSimple::ScoreRecordSimple(void) { 
   textdata.setSize(1);
   textdata.allowGrowth(0);
   textdata[0] = '\0';
  
   textfont.setSize(1);
   textfont.allowGrowth(0);
   textfont[0] = '\0';

   analysisData.setSize(3);
   analysisData.allowGrowth(0);
   analysisData.zero();
}


ScoreRecordSimple::ScoreRecordSimple(ScoreRecordSimple& a) {
   serialnumber = a.serialnumber;
   textdata     = a.textdata;
   textfont     = a.textfont;
   analysisData = a.analysisData;
   pi = a.pi;
}



//////////////////////////////
//
// ScoreRecordSimple::~ScoreRecordSimple --
//

ScoreRecordSimple::~ScoreRecordSimple(void) { 
   // do nothing
   textdata.setSize(0);
   textfont.setSize(0);
}



//////////////////////////////
//
// ScoreRecordSimple::clear --
//

void ScoreRecordSimple::clear(void) {
   textdata.setSize(1);
   textdata.allowGrowth(0);
   textdata[0] = '\0';

   textfont.setSize(1);
   textfont.allowGrowth(0);
   textfont[0] = '\0';

   analysisData.setSize(3); 
   analysisData.allowGrowth(0);
   analysisData.zero();

   pi.clear();
}



//////////////////////////////
//
// ScoreRecordSimple::printAscii -- print the parameter data in ASCII text
//     format.
//     default value: roundQ   = 1
//     default value: verboseQ = 0
//

void ScoreRecordSimple::printAscii(ostream& out, int roundQ, int verboseQ) { 
   if (getFixedSize() < 1) {
      return;
   }

   shrink();

   union { float num; unsigned char ch[4]; } u;
   static char buffer[128] = {0};

   double num;
   int i, j;
   int tcount = 0;
   int tcorr  = 0;
   int bufflen;
   for (i=0; i<getFixedSize(); i++) {
      if (isTextItem() && (i==2) && (fabs(getValue(i)) < 0.001)) {
         sprintf(buffer, "%f", 0.001);
         // there is a bug in WinScore with text at position 0.0000...
      } else {
         num = getValue(i);
         if (roundQ) {
            if (i == 2) {
               num = int(num * 1000.0 + 0.5)/1000.0;
            } else if ((i == 5) && 
                  ( (int(getValue(0)) == 4) ||
                    (int(getValue(0)) == 5) ||
                    (int(getValue(0)) == 6)
	          ) ) {
               // round ending of lines, slurs and beams to nearest 1/1000th
               num = int(num * 1000.0 + 0.5)/1000.0;
            }
         }
	        
         sprintf(buffer, "%f", num);

         u.num = (float)atof(buffer);
         if (u.num != num) {
            SSTREAM stream;
            stream.precision(8);
            stream << num;
            stream << ends;
            strcpy(buffer, stream.CSTRING);
         } 
         u.num = (float)atof(buffer);
      }
      if (verboseQ) {
         if (u.num != getValue(i)) {
            cout << "XXX:";
         }
      }
      adjustBuffer(buffer);
      if (strchr(buffer, 'e') != NULL) {
         strcpy(buffer, "0.00000");
      }

      bufflen = (int)strlen(buffer);

      switch (i) {
         case P1: 
            if ((int)getValue(0) == 16) {
               out << "t    ";
            } else {
               out << buffer;
	       if (bufflen == 2) {
                  out << ' ';
               }
               out << "  ";
		         
            }
            if (i < getFixedSize()-1) {
               out << ' ';
            }
            break;
         case P2: 
            tcount = 0;
            if (getValue(i) >= 0) {
	       tcount++;	
               if (tcorr > 0) { tcorr--; } else { out << ' '; }
            }
            if (getValue(i) < 10) {
	       tcount++;	
               if (tcorr > 0) { tcorr--; } else { out << ' '; }
            }
            out << buffer;
            if (i < getFixedSize()-1) {
               out << ' ';
            }
            if (i < getFixedSize()-1) {
               for (j=0; j<5 - bufflen - tcount; j++) {
                  out << ' ';
               }
               out << ' ';
            }
	    if (5 - bufflen - tcount < 0) {
               tcorr = -(5-bufflen-tcount);
            }
            break;
         case P3: 
	    tcount = 0;
            cout << setiosflags(ios::left);
            if (getValue(i) < 10.0) {
               out << ' ';
	       tcount++;
            }
            if (getValue(i) < 100.0) {
               out << ' ';
	       tcount++;
            }
            out << buffer;
            if (i<getFixedSize()-1) {
               for (j=0; j<11 - bufflen - tcount; j++) {
                  out << ' ';
               }
               out << ' ';
            }
	    if (11 - bufflen - tcount < 0) {
               tcorr = -(11-bufflen - tcount);
            }
            break;
         default: 
	    tcount = 0;
            if (getValue(i) >= 0) {
	       tcount++;	
               if (tcorr > 0) { tcorr--; } else { out << ' '; }
            }
            if (fabs(getValue(i)) < 10) {
	       tcount++;	
               if (tcorr > 0) { tcorr--; } else { out << ' '; }
            }
            if (fabs(getValue(i)) < 100) {
	       tcount++;	
               if (tcorr > 0) { tcorr--; } else { out << ' '; }
            }
            out << buffer;
            if (i<getFixedSize()-1) {
               for (j=0; j<11 - bufflen - tcount; j++) {
                  out << ' ';
               }
               out << ' ';
            }
	    if (11 - bufflen - tcount < 0) {
               tcorr = -(11-bufflen-tcount);
            }
      }
   }

   if (isTextItem()) {
      out << "\n" << getTextFont() << getTextData();
   }

   if (isNoteItem() && verboseQ && getPitch() != 0) {
      char buffer[32];
      out << "\n; pitch=" << getPitch() << " ("
          << Convert::base40ToKern(buffer, getPitch()) << ")";
   }


   // leave printing of final newline to the calling function
}


void ScoreRecordSimple::printAscii(char* buffer1, char* buffer2, int max, 
      int roundQ, int verboseQ) {

   SSTREAM stream;
   printAscii(stream, roundQ, verboseQ);
   stream << ends;

   int length = (int)strlen(stream.CSTRING);
   if (length > max) {
      cerr << "ERROR: Score Record too long" << endl;
      exit(1);
   }
   strcpy(buffer1, stream.CSTRING);
   char* ptr = strchr(buffer1, '\n');
   if (ptr != NULL) {
      strcpy(buffer2, ptr+1);
      *ptr = '\0';
   } else {
      buffer2[0] = '\0';
   }

   //length = (int)strlen(buffer1);
   //if (!isprint(buffer1[length-1])) {
   //   buffer1[length-1] = '\0';
   //}
   //buffer1[length-1] = '\0';
   //buffer1[length-2] = '\0';

}



//////////////////////////////
//
// ScoreRecordSimple::writeBinary -- print the parameter data in Binary
//    format, as found in a SCORE .mus file where the number of numbers
//    to be written is given first.
//

int ScoreRecordSimple::writeBinary(ostream& out) { 
   if (getFixedSize() < 1) {
      return 0;
   }

   shrink();

   // WinScore can't understand data fields smaller than 3
   if (getFixedSize() < 3) {
      setPValue(3, 0.0);
   }

   int i;
   if (getValue(0) == P1_Text) {
      // set the length of the text in P12
      int textlen = (int)strlen(getTextData());
      int fontlen = (int)strlen(getTextFont());
      int totaltextlen = textlen + fontlen;

      setPValue(12, totaltextlen);
      if (getPValue(13) == 0.0) {
         setPValue(13, 0.0); // needed for .MUS files to parse in WinScore
      }
      int spaces = totaltextlen % 4;
      int charbytes = totaltextlen / 4;
      if (spaces != 0) {
         spaces = 4 - spaces;
         charbytes++;
      }

      writeLittleEndian(out, (float)(getFixedSize() + charbytes));
      for (i=0; i<getFixedSize(); i++) {
         writeLittleEndian(out, getValue(i));
      }

      const char* ptr = getTextFont();
      for (i=0; i<fontlen; i++) {
         out << ptr[i];
      }

      ptr = getTextData();
      for (i=0; i<textlen; i++) {
         out << ptr[i];
      }

      for (i=0; i<spaces; i++) {
         out << ' ';
      }

      return getFixedSize() + charbytes + 1;
   } else {
      writeLittleEndian(out, (float)getFixedSize());
      for (i=0; i<getFixedSize(); i++) {
         writeLittleEndian(out, getValue(i));
      }
      return getFixedSize() + 1;
   }

}



//////////////////////////////
//
// ScoreRecordSimple::readBinary -- input parameter values from the given
//     input stream.
//

void ScoreRecordSimple::readBinary(istream& instream, int pcount) {
   int i;
   setFixedSize(pcount);
   if (pcount < 1) {
      cout << "Error: invalid parameter count: " << pcount << endl;
      exit(1);
   }
   setValue(0, readLittleEndian(instream));

   if ((int)getValue(0) == 16) {
      setFixedSize(13);
      for (i=1; i<13; i++) {
         setValue(i, readLittleEndian(instream));
      }
      int count = (int)getPValue(12);
      // char buffer[count+1+16];  // can't be done in MS Visual C++
      static char buffer[1024] = {0}; 
      instream.read(buffer, count);
      buffer[count] = '\0';
      setTextData(buffer);
      int extra = count % 4;
      if (extra > 0) {
         instream.read(buffer, 4-extra);
         for (i=0; i<4-extra; i++) {
            if (buffer[i] != 0x20) {
              cout << "Error: non space character found" << endl;
              exit(1);
            }
         }
      }
   } else {
      // non-text data parameters
      for (i=1; i<pcount; i++) {
         setValue(i, readLittleEndian(instream));
      }
   }
}



//////////////////////////////
//
// ScoreRecordSimple::shrink --
//

void ScoreRecordSimple::shrink(void) {
   int i;
   for (i=getFixedSize()-1; i>0; i--) {
      if (getValue(i) != 0.0) {
         break;
      } else {
         setFixedSize(i);
      }
   }
}


//////////////////////////////////////////////////////////////////////////
//
// private functions
//


///////////////////////////////
//
// ScoreRecordSimple::writeLittleEndian --
//

void ScoreRecordSimple::writeLittleEndian(ostream& out, float number) {
   union { float f; unsigned int i; } num;
   num.f = number;
   char byteinfo[4];
   byteinfo[0] = (char)( num.i        & 0xff);
   byteinfo[1] = (char)((num.i >> 8)  & 0xff);
   byteinfo[2] = (char)((num.i >> 16) & 0xff);
   byteinfo[3] = (char)((num.i >> 24) & 0xff);
   out.write(byteinfo, 4);
}



///////////////////////////////
//
// ScoreRecordSimple::readLittleEndian --
//

float ScoreRecordSimple::readLittleEndian(istream& instream) {
   unsigned char byteinfo[4];
   instream.read((char*)byteinfo, 4);
   union { float f; unsigned int i; } num;
   num.i = 0;
   num.i = byteinfo[3];
   num.i = (num.i << 8) | byteinfo[2];
   num.i = (num.i << 8) | byteinfo[1];
   num.i = (num.i << 8) | byteinfo[0];

   return num.f;
}



//////////////////////////////
//
// ScoreRecordSimple::adjustBuffer -- remove all right hand zeros
//

void ScoreRecordSimple::adjustBuffer(char *buffer) {
   int len = (int)strlen(buffer);
   int i;
   for (i=len-1; i>0; i--) {
      if (buffer[i] == '0') {
         buffer[i] = '\0';
         len--;
      } else {
         break;
      }
   }

   // remove trailing decimal point as well for integers:
   if (buffer[len-1] == '.') {
      buffer[len-1] = ' ';
   }
}



//////////////////////////////
//
// ScoreRecordSimple::getTextData -- return the Code items text data or an
//     empty string.
//

const char* ScoreRecordSimple::getTextData(void) {
   if (textdata.getSize() <= 0) {
      return "";
   } else {
      return textdata.getBase();
   }
}



//////////////////////////////
//
// ScoreRecordSimple::setTextData -- set the Code items text data field.
//

void ScoreRecordSimple::setTextData(const char* buffer) {
   int length = (int)strlen(buffer);
   textdata.setSize(length+1);
   strcpy(textdata.getBase(), buffer);
}



//////////////////////////////
//
// ScoreRecordSimple::getTextFont -- return the Code items text data or an
//     empty string.
//

const char* ScoreRecordSimple::getTextFont(void) {
   if (textfont.getSize() <= 0) {
      return "";
   } else {
      return textfont.getBase();
   }
}



//////////////////////////////
//
// ScoreRecordSimple::setTextFont -- set the Code items text data field.
//

void ScoreRecordSimple::setTextFont(const char* buffer) {
   int length = (int)strlen(buffer);
   textfont.setSize(length+1);
   strcpy(textfont.getBase(), buffer);
}



//////////////////////////////
//
// ScoreRecordSimple::getPitch --
//

int ScoreRecordSimple::getPitch(void) { 
   return (int)analysisData[0];
}



//////////////////////////////
//
// ScoreRecordSimple::setPitch --
//

void ScoreRecordSimple::setPitch(int aPitch) { 
   analysisData[0] = (float)aPitch;
}



//////////////////////////////
//
// ScoreRecordSimple::getStartOffset -- get the starting time from the start
//     of the staff.
//

float ScoreRecordSimple::getStartOffset(void) { 
   return analysisData[1];
}



//////////////////////////////
//
// ScoreRecordSimple::setStartOffset -- set the starting time from the start
//     of the staff
//

void ScoreRecordSimple::setStartOffset(float aDuration) { 
   analysisData[1] = aDuration;
}



//////////////////////////////
//
// ScoreRecordSimple::getVoice --
//

int ScoreRecordSimple::getVoice(void) { 
   return (int)analysisData[2];

}



//////////////////////////////
//
// ScoreRecordSimple::setVoice --
//

void ScoreRecordSimple::setVoice(int aVoice) { 
   analysisData[2] = (float)aVoice;
}



//////////////////////////////
//
// ScoreRecordSimple::operator= --
//

ScoreRecordSimple& ScoreRecordSimple::operator=(ScoreRecordSimple& a) {
   if (&a== this) {
      return *this;
   }

   serialnumber = a.serialnumber;


   textdata.setSize(a.textdata.getSize());
   textfont.setSize(a.textfont.getSize());

   int i;
   for (i=0; i<textdata.getSize(); i++) {
      textdata[i] = a.textdata[i];
   }
   for (i=0; i<textfont.getSize(); i++) {
      textfont[i] = a.textfont[i];
   }

   for (i=0; i<a.getFixedSize(); i++) {
      setValue(i, a.getValue(i));
   }
   // do something with key parameters later...
   clearKeyParams();
        
   analysisData = a.analysisData;
   return *this;
}



//////////////////////////////
//
// ScoreRecordSimple::is... -- boolean for code Item values.
//

int ScoreRecordSimple::isNoteItem(void)       
   { return (int)getPValue(1) == P1_Note ? 1 : 0;}

int ScoreRecordSimple::isRestItem(void)       
   { return (int)getPValue(1) == P1_Rest ? 1 : 0;}

int ScoreRecordSimple::hasDurationQ(void)     
   { return isNoteItem() || isRestItem();}

int ScoreRecordSimple::isClefItem(void)       
   { return (int)getPValue(1) == P1_Clef ? 1 : 0;}

int ScoreRecordSimple::isLineItem(void)       
   { return (int)getPValue(1) == P1_Line ? 1 : 0;}

int ScoreRecordSimple::isSlurItem(void)       
   { return (int)getPValue(1) == P1_Slur ? 1 : 0;}

int ScoreRecordSimple::isBeamItem(void)       
   { return (int)getPValue(1) == P1_Beam ? 1 : 0;}

int ScoreRecordSimple::isTrillItem(void)      
   { return (int)getPValue(1) == P1_Trill ? 1 : 0;}

int ScoreRecordSimple::isStaffItem(void)      
   { return (int)getPValue(1) == P1_Staff ? 1 : 0;}

int ScoreRecordSimple::isSymbolItem(void)     
   { return (int)getPValue(1) == P1_Symbol ? 1 : 0;}

int ScoreRecordSimple::isNumberItem(void)     
   { return (int)getPValue(1) == P1_Number? 1 : 0;}

int ScoreRecordSimple::isUserItem(void)       
   { return (int)getPValue(1) == P1_User? 1 : 0;}

int ScoreRecordSimple::isShapeItem(void)      
   { return (int)getPValue(1) == P1_Special? 1 : 0;}

int ScoreRecordSimple::isBarlineItem(void)    
   { return (int)getPValue(1) == P1_Barline? 1 : 0;}

int ScoreRecordSimple::isPostscriptItem(void) 
   { return (int)getPValue(1) == P1_ImportedEPSGraphic? 1 : 0;}

int ScoreRecordSimple::isTextItem(void)       
   { return (int)getPValue(1) == P1_Text? 1 : 0;}

int ScoreRecordSimple::isKeysigItem(void)     
   { return (int)getPValue(1) == P1_KeySignature? 1 : 0;}

int ScoreRecordSimple::isTimesigItem(void)    
   { return (int)getPValue(1) == P1_MeterSignature? 1 : 0;}



// md5sum: 9316e26ed432152361bd0ff02f6e024c ScoreRecordSimple.cpp [20050403]
