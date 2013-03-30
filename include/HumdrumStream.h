//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Dec 11 16:03:43 PST 2012
// Last Modified: Tue Dec 11 16:03:46 PST 2012
// Filename:      ...sig/include/sigInfo/HumdrumStream.h
// Web Address:   http://sig.sapp.org/include/sigInfo/HumdrumStream.h
// Syntax:        C++ 
//
// Description:   Multi-movement manager for Humdrum files.  The class
//                will accept files, standard input, URLs, URIs which
//                have more than one data start/stop sequence.  This usually
//                indicates multiple movements if stored in one file, or
//                multiple works if coming in from standard input.
//

#ifndef _HUMDRUMSTREAM_H_INCLUDED
#define _HUMDRUMSTREAM_H_INCLUDED

#include "HumdrumFile.h"
#include "Array.h"

#ifndef OLDCPP
   #include <iostream>
   #include <fstream>
   #include <sstream>
   #define SSTREAM stringstream
   #define CSTRING str().c_str()
   using namespace std;
#else
   #include <iostream.h>
   #include <fstream.h>
   #ifdef VISUAL
      #include <strstrea.h>
   #else
      #include <strstream.h>
   #endif
   #define SSTREAM strstream
   #define CSTRING str()
#endif

// the following define is for compiling/not compiling the automatic
// downloading of data from the web inside of the read() functions.
// Comment out the define if your OS doesn't understand the primarily
// GCC/linux based method of downloading with a socket.
#define USING_URI

#ifdef USING_URI
   #include <sys/types.h>   /* socket, connect */
   #include <sys/socket.h>  /* socket, connect */
   #include <netinet/in.h>  /* htons           */
   #include <netdb.h>       /* gethostbyname   */
   #include <unistd.h>      /* read, write     */
   #include <string.h>      /* memcpy          */
#endif


class HumdrumStream {
   public:
                      HumdrumStream      (void);
                      HumdrumStream      (char** list);

   int                setFileList        (char** list);

   int                is_open            (void);
   int                hasData            (void);
   void               clear              (void);
   
   int                getFile            (HumdrumFile& infile);
   int                read               (HumdrumFile& infile);

   protected:
      ifstream            instream;      // used to read from list of files.
      SSTREAM             urlbuffer;     // used to read data over internet.
      Array<char>         newfilebuffer; // used to keep track of !!!!segment: 
                                         // records.

      Array<Array<char> > filelist;      // used when not using cin
      int                 curfile;       // index into filelist

      Array<Array<char> > universals;    // storage for universal comments

      // automatic URI downloading of data in read()
      #ifdef USING_URI
      void     fillUrlBuffer            (SSTREAM& uribuffer, 
                                         const char* uriname);
      int      getChunk                 (int socket_id, SSTREAM& inputdata, 
                                         char* buffer, int bufsize);
      int      getFixedDataSize         (int socket_id, int datalength, 
                                         SSTREAM& inputdata, char* buffer, 
                                         int bufsize);
      int      open_network_socket      (const char *hostname, 
                                         unsigned short int port);
      void     prepare_address          (struct sockaddr_in *addr, 
                                         const char *hostname,
                                         unsigned short int port);
      #endif

};

#endif /* _HUMDRUMSTREAM_H_INCLUDED */



// md5sum: 8e155fdb7b2d0af7bbfa1d92cd7ccd85 HumdrumStream.h [20050403]
