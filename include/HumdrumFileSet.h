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
#include "Options.h"

#include <iostream>
#include <istream>
#include <string>
#include <vector>

using namespace std;


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
      int                    read             (const string& filename);
      int                    read             (istream& inStream);
      int                    read             (Options& options);
      int                    readAppend       (const char* filename);
      int                    readAppend       (const string& filename);
      int                    readAppend       (istream& inStream, 
                                               const char* filename = "");

   protected:
      vector<HumdrumFile*>   data;

      void                   appendHumdrumFileContent(const char* filename, 
                                               stringstream& inbuffer);

      #ifdef USING_URI
      void                   readAppendFromHumdrumURI(stringstream& inputstream,
                                                const char* humdrumaddress);
      void                   readAppendFromJrpURI(stringstream& inputstream, 
                                                const char* jrpaddress);
      void                   readAppendFromHttpURI(stringstream& inputstream,
                                                const char* webaddress,
                                                const char* filename = NULL);
      int                    getChunk          (int socket_id, 
                                                stringstream& inputstream, 
                                                char* buffer, int bufsize);
      void                   prepare_address   (struct sockaddr_in *address, 
                                                const char *hostname, 
                                                unsigned short int port);
      int                    open_network_socket(const char *hostname, 
                                                unsigned short int port);
      int                    getFixedDataSize  (int socket_id, int datalength, 
                                                stringstream& inputstream, 
                                                char* buffer, int bufsize);
      #endif

};

#endif /* _HUMDRUMFILESET_H_INCLUDED */

// md5sum: 8e155fdb7b2d0af7bbfa1d92cd7ccd85 HumdrumFileSet.h [20050403]
