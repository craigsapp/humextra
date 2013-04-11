//
// Copyright 1998-2001 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Fri Mar 29 14:49:35 PDT 2013
// Last Modified: Fri Mar 29 14:49:39 PDT 2013
// Filename:      ...sig/include/sigInfo/HumdrumFileSet.h
// Web Address:   http://sig.sapp.org/include/sigInfo/HumdrumFileSet.h
// Syntax:        C++ 
//
// Description:   Collection of one or more Humdrum data sequences started
//                with an exclusive interpretation and ending with *-.
//

#ifndef _HUMDRUMFILESET_H_INCLUDED
#define _HUMDRUMFILESET_H_INCLUDED


// the following define is for compiling/not compiling the automatic
// downloading of data from the web inside of the read() functions.
// Comment out the define if your OS doesn't understand the primarily
// GCC/linux based method of downloading with a socket.
#define USING_URI

#include "HumdrumFile.h"

#ifndef OLDCPP
   #include <iostream>
   #include <istream>
   using namespace std;
#else
   #include <iostream.h>
   #include <istream.h>
#endif


///////////////////////////////////////////////////////////////////////////

class HumdrumFileSet {
   public:
                             HumdrumFileSet   (void);
                            ~HumdrumFileSet   ();

      void                   clear            (void);
      int                    getSize          (void);
      int                    getCount         (void) { return getSize(); }
      HumdrumFile&           operator[]       (int index);
      int                    read             (const char* filename);
      int                    read             (istream& inStream);
      int                    readAppend       (const char* filename);
      int                    readAppend       (istream& inStream);

   protected:
      Array<HumdrumFile*>    data;

      void                   appendHumdrumFileContent(const char* filename, 
                                               SSTREAM& inbuffer);

      #ifdef USING_URI
      void                   readAppendFromHumdrumURI(SSTREAM& inputstream,
                                                const char* humdrumaddress);
      void                   readAppendFromJrpURI(SSTREAM& inputstream, 
                                                const char* jrpaddress);
      void                   readAppendFromHttpURI(SSTREAM& inputstream,
                                                const char* webaddress);
      int                    getChunk          (int socket_id, 
                                                SSTREAM& inputstream, 
                                                char* buffer, int bufsize);
      void                   prepare_address   (struct sockaddr_in *address, 
                                                const char *hostname, 
                                                unsigned short int port);
      int                    open_network_socket(const char *hostname, 
                                                unsigned short int port);
      int                    getFixedDataSize  (int socket_id, int datalength, 
                                                SSTREAM& inputstream, 
                                                char* buffer, int bufsize);
      #endif

};

#endif /* _HUMDRUMFILESET_H_INCLUDED */

// md5sum: 8e155fdb7b2d0af7bbfa1d92cd7ccd85 HumdrumFileSet.h [20050403]